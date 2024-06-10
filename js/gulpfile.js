//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

/* eslint no-sync: "off" */
/* eslint no-process-env: "off" */
/* eslint no-process-exit: "off" */

import del from "del";
import extReplace from "gulp-ext-replace";
import fs from "fs";
import gulp from "gulp";
import iceBuilder from "gulp-ice-builder";
import path from "path";
import paths from "vinyl-paths";
import pump from "pump";
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

const sliceDir = path.resolve(__dirname, '..', 'slice');

const iceBinDist = (process.env.ICE_BIN_DIST || "").split(" ");
const useBinDist = iceBinDist.find(v => v == "js" || v == "all") !== undefined;

function parseArg(argv, key) {
    for (let i = 0; i < argv.length; ++i) {
        const e = argv[i];
        if (e == key) {
            return argv[i + 1];
        }
        else if (e.indexOf(key + "=") === 0) {
            return e.substr(key.length + 1);
        }
    }
}

const platform = parseArg(process.argv, "--cppPlatform") || process.env.CPP_PLATFORM;
const configuration = parseArg(process.argv, "--cppConfiguration") || process.env.CPP_CONFIGURATION;

function slice2js(options) {
    const defaults = {};
    const opts = options || {};
    if (!useBinDist) {
        if (process.platform == "win32") {
            if (!platform || (platform.toLowerCase() != "win32" && platform.toLowerCase() != "x64")) {
                console.log("Error: --cppPlatform must be set to `Win32' or `x64', in order to locate slice2js.exe");
                process.exit(1);
            }

            if (!configuration || (configuration.toLowerCase() != "debug" && configuration.toLowerCase() != "release")) {
                console.log("Error: --cppConfiguration must be set to `Debug' or `Release', in order to locate slice2js.exe");
                process.exit(1);
            }
            defaults.iceToolsPath = path.resolve("../cpp/bin", platform, configuration);
        }
        defaults.iceHome = path.resolve(__dirname, '..');
    }
    else if (process.env.ICE_HOME) {
        defaults.iceHome = process.env.ICE_HOME;
    }
    defaults.include = opts.include || [];
    defaults.args = opts.args || [];
    defaults.jsbundle = opts.jsbundle;
    defaults.tsbundle = opts.tsbundle;
    defaults.jsbundleFormat = opts.jsbundleFormat;
    return iceBuilder(defaults);
}

//
// Tasks to build IceJS Distribution
//
const root = path.resolve(__dirname);
const libs = ["Ice", "Glacier2", "IceStorm", "IceGrid"];

const generateTask = name => name.toLowerCase() + ":generate";

const srcDir = name => path.join(root, "src", name);
const libCleanTask = lib => lib + ":clean";

function libFiles(name) {
    return [
        path.join(root, "lib", name + ".js")];
}

function libGeneratedFiles(lib, sources) {
    const tsSliceSources = sources.typescriptSlice || sources.slice;

    return sources.slice.map(f => path.join(srcDir(lib), path.basename(f, ".ice") + ".js"))
        .concat(tsSliceSources.map(f => path.join(srcDir(lib), path.basename(f, ".ice") + ".d.ts")))
        .concat(libFiles(lib))
        .concat([path.join(srcDir(lib), ".depend", "*")]);
}

const sliceFile = f => path.join(sliceDir, f);

for (const lib of libs) {
    const sources = JSON.parse(fs.readFileSync(path.join(srcDir(lib), "sources.json"), { encoding: "utf8" }));

    gulp.task(generateTask(lib),
        cb => {
            pump([gulp.src(sources.slice.map(sliceFile)),
            slice2js(
                {
                    jsbundle: false,
                    tsbundle: false,
                    args: ["--typescript"]
                }),
            gulp.dest(srcDir(lib))], cb);
        });

    gulp.task(libCleanTask(lib),
        cb => {
            del(libGeneratedFiles(lib, sources));
            cb();
        });
}

if (useBinDist) {
    gulp.task("ice:module", cb => cb());
    gulp.task("ice:module:clean", cb => cb());
    gulp.task("dist", cb => cb());
    gulp.task("dist:clean", cb => cb());
}
else {
    gulp.task("dist", gulp.series(gulp.parallel(libs.map(generateTask))));

    gulp.task("dist:clean", gulp.parallel(libs.map(libCleanTask)));

    gulp.task("ice:module:package",
        () => gulp.src(['package.json']).pipe(gulp.dest(path.join("node_modules", "ice"))));

    gulp.task("ice:module",
        gulp.series("ice:module:package",
            cb => {
                pump([
                    gulp.src([path.join(root, 'src/**/*')]),
                    gulp.dest(path.join(root, "node_modules", "ice", "src"))], cb);
            }));

    gulp.task("ice:module:clean", () => gulp.src(['node_modules/ice'], { allowEmpty: true }).pipe(paths(del)));
}

const tests = [
    "test/Ice/adapterDeactivation",
    "test/Ice/ami",
    "test/Ice/binding",
    "test/Ice/defaultValue",
    "test/Ice/enums",
    "test/Ice/exceptions",
    "test/Ice/facets",
    "test/Ice/hold",
    "test/Ice/info",
    "test/Ice/inheritance",
    "test/Ice/location",
    "test/Ice/objects",
    "test/Ice/operations",
    "test/Ice/optional",
    "test/Ice/promise",
    "test/Ice/properties",
    "test/Ice/proxy",
    "test/Ice/retry",
    "test/Ice/servantLocator",
    "test/Ice/slicing/exceptions",
    "test/Ice/slicing/objects",
    "test/Ice/stream",
    "test/Ice/timeout",
    "test/Ice/number",
    "test/Ice/scope",
    "test/Glacier2/router",
    "test/Slice/escape",
    "test/Slice/macros"
];

gulp.task("test:common:generate",
    cb => {
        pump([gulp.src(["../scripts/Controller.ice"]),
        slice2js(),
        gulp.dest("test/Common")], cb);
    });

gulp.task("test:common:clean",
    cb => {
        del(["test/Common/Controller.js",
            "test/Common/.depend"]);
        cb();
    });

const testTask = name => name.replace(/\//g, "_");
const testCleanTask = name => testTask(name) + ":clean";
const testBuildTask = name => testTask(name) + ":build";

for (const name of tests) {
    gulp.task(testBuildTask(name),
        cb => {
            const outdir = path.join(root, name);
            pump([gulp.src(path.join(outdir, "*.ice")),
            slice2js(
                {
                    include: [outdir]
                }),
            gulp.dest(outdir)], cb);
        });

    gulp.task(testCleanTask(name),
        cb => {
            pump([gulp.src(path.join(name, "*.ice")),
            extReplace(".js"),
            gulp.src(path.join(name, ".depend"), { allowEmpty: true }),
            paths(del)], cb);
        });
}

gulp.task(
    "test",
    gulp.series("test:common:generate",
        gulp.parallel(tests.map(testBuildTask))));

gulp.task(
    "test:clean",
    gulp.parallel("test:common:clean", tests.map(testCleanTask)));

gulp.task("build", gulp.series("dist", "ice:module", "test"));
gulp.task("clean", gulp.series("dist:clean", "ice:module:clean", "test:clean"));
gulp.task("default", gulp.series("build"));
