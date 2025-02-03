# Copyright (c) ZeroC, Inc.

import os
import sys
import runpy
import getopt
import traceback
import types
import threading
import time
import re
import itertools
import random
import subprocess
import shutil
import copy
import uuid
import xml.sax.saxutils
from platform import machine as platform_machine
from pathlib import Path

from io import StringIO

from collections import OrderedDict
import Expect

toplevel = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))


def run(cmd, cwd=None, err=False, stdout=False, stdin=None, stdinRepeat=True):
    p = subprocess.Popen(
        cmd,
        shell=True,
        stdin=subprocess.PIPE,
        stdout=None if stdout else subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=cwd,
    )
    try:
        if stdin:
            try:
                while True:
                    p.stdin.write(stdin)
                    p.stdin.flush()
                    if not stdinRepeat:
                        break
                    time.sleep(1)
            except Exception:
                pass

        out = (p.stderr if stdout else p.stdout).read().decode("UTF-8").strip()
        if (not err and p.wait() != 0) or (err and p.wait() == 0):
            raise RuntimeError(cmd + " failed:\n" + out)
    finally:
        #
        # Without this we get warnings when running with python_d on Windows
        #
        # ResourceWarning: unclosed file <_io.TextIOWrapper name=3 encoding='cp1252'>
        #
        (p.stderr if stdout else p.stdout).close()
        try:
            p.stdin.close()
        except Exception:
            pass
    return out


def val(v, quoteValue=True):
    if isinstance(v, bool):
        return "1" if v else "0"
    elif isinstance(v, str):
        if not quoteValue or v.find(" ") < 0:
            return v
        v = v.replace("\\", "\\\\").replace('"', '\\"')
        return '"{0}"'.format(v)
    else:
        return str(v)


illegalXMLChars = re.compile("[\x00-\x08\x0b\x0c\x0e-\x1f\ud800-\udfff\ufffe\uffff]")


def escapeXml(s, attribute=False):
    # Remove backspace characters from the output (they aren't accepted by Jenkins XML parser)
    s = "".join(ch for ch in s if ch != "\u0008")
    s = illegalXMLChars.sub("?", s)  # Strip invalid XML characters
    return xml.sax.saxutils.quoteattr(s) if attribute else xml.sax.saxutils.escape(s)


class Component(object):
    """
    Component abstract class. The driver and mapping classes rely on the component
    class to provide component specific information.
    """

    def __init__(self):
        self.nugetVersion = {}

    def getInstallDir(self, mapping, current):
        """
        Returns the component installation directory if using a binary distribution
        or the mapping directory if using a source distribution.
        """
        raise Exception("must be overridden")

    def getSourceDir(self):
        return toplevel

    def getTestDir(self, mapping):
        if isinstance(mapping, JavaMapping):
            return os.path.join(
                mapping.getPath(), "test", "src", "main", "java", "test"
            )
        return os.path.join(mapping.getPath(), "test")

    def getScriptDir(self):
        return os.path.join(self.getSourceDir(), "scripts", "tests")

    def getPhpExtension(self, mapping, current):
        raise RuntimeError("must be overridden if component provides php mapping")

    def getNugetPackage(self, mapping):
        return "zeroc.{0}.{1}".format(
            self.__class__.__name__.lower(),
            "net"
            if isinstance(mapping, CSharpMapping)
            else platform.getPlatformToolset(),
        )

    def getNugetPackageVersion(self, mapping):
        if mapping not in self.nugetVersion:
            file = self.getNugetPackageVersionFile(mapping)
            if file.endswith(".nuspec"):
                expr = "<version>(.*)</version>"
            elif file.endswith("packages.config"):
                expr = 'id="{0}" version="(.*)" target'.format(
                    self.getNugetPackage(mapping)
                )
            if expr:
                with open(file, "r") as config:
                    m = re.search(expr, config.read())
                    if m:
                        self.nugetVersion[mapping] = m.group(1)
        if mapping not in self.nugetVersion:
            raise RuntimeError(
                "couldn't figure out the nuget version from `{0}'".format(file)
            )
        return self.nugetVersion[mapping]

    def getNugetPackageVersionFile(self, mapping):
        raise RuntimeError(
            "must be overridden if component provides C++ or C# nuget packages"
        )

    def getFilters(self, mapping, config):
        return ([], [])

    def canRun(self, testId, mapping, current):
        return True

    def isMainThreadOnly(self, testId):
        return True

    def getDefaultProcesses(self, mapping, processType, testId):
        return None

    def getDefaultExe(self, mapping, processType):
        return None

    def getDefaultSource(self, mapping, processType):
        return None

    def getOptions(self, testcase, current):
        return None

    def getRunOrder(self):
        return []

    def getEnv(self, process, current):
        return {}

    def getProps(self, process, current):
        return {}

    def overrideConfig(self, mapping, config):
        return config

    def isCross(self, testId):
        return False

    def getSliceDir(self, mapping, current):
        installDir = self.getInstallDir(mapping, current)
        if installDir.endswith(mapping.name):
            installDir = installDir[0 : len(installDir) - len(mapping.name) - 1]
        if platform.getInstallDir() and installDir == platform.getInstallDir():
            if sys.platform == "darwin":
                return os.path.join(installDir, "opt", "ice", "share", "ice", "slice")
            else:
                return os.path.join(installDir, "share", "ice", "slice")
        else:
            return os.path.join(installDir, "slice")

    def getBinDir(self, process, mapping, current):
        return platform._getBinDir(self, process, mapping, current)

    def getLibDir(self, process, mapping, current):
        return platform._getLibDir(self, process, mapping, current)

    def getPhpIncludePath(self, mapping, current):
        return "{0}/{1}".format(
            self.getInstallDir(mapping, current),
            "lib",
        )

    def _getInstallDir(self, mapping, current, envHomeName):
        if mapping:
            return mapping.getPath()
        else:
            return self.getSourceDir()


class Platform(object):
    def __init__(self):
        try:
            self.nugetPackageCache = re.search(
                "global-packages: (.*)",
                run("dotnet nuget locals --list global-packages"),
            ).groups(1)[0]
        except Exception:
            self.nugetPackageCache = None

        self._hasNodeJS = None
        self._hasSwift = None

    def init(self, component):
        self.parseBuildVariables(
            component,
            {
                "supported-platforms": ("supportedPlatforms", lambda s: s.split(" ")),
                "supported-configs": ("supportedConfigs", lambda s: s.split(" ")),
            },
        )

    def hasDotNet(self):
        return self.nugetPackageCache is not None

    def hasNodeJS(self):
        if self._hasNodeJS is None:
            try:
                run("node --version")
                self._hasNodeJS = True
            except Exception:
                self._hasNodeJS = False
        return self._hasNodeJS

    def hasSwift(self, version):
        if self._hasSwift is None:
            try:
                m = re.search(
                    r"Apple Swift version ([0-9]+\.[0-9]+)", run("swift --version")
                )
                if m and m.group(1):
                    self._hasSwift = (
                        tuple([int(n) for n in m.group(1).split(".")]) >= version
                    )
                else:
                    self.hasSwift = False
            except Exception:
                self._hasSwift = False
        return self._hasSwift

    def parseBuildVariables(self, component, variables):
        # Run make to get the values of the given variables
        if os.path.exists(
            os.path.join(component.getSourceDir(), "Makefile")
        ):  # Top level makefile
            cwd = component.getSourceDir()
        elif Mapping.getByName("cpp"):
            cwd = Mapping.getByName("cpp").getPath()

        output = run('make print V="{0}"'.format(" ".join(variables.keys())), cwd=cwd)
        for line in output.split("\n"):
            match = re.match(r"^.*:.*: (.*) = (.*)", line)
            if match and match.group(1):
                if match.group(1) in variables:
                    (varname, valuefn) = variables[match.group(1).strip()]
                    value = match.group(2).strip() or ""
                    setattr(self, varname, valuefn(value) if valuefn else value)

    def getDefaultBuildPlatform(self):
        return self.supportedPlatforms[0]

    def getDefaultBuildConfig(self):
        return self.supportedConfigs[0]

    def _getBinDir(self, component, process, mapping, current):
        installDir = component.getInstallDir(mapping, current)
        if isinstance(mapping, CSharpMapping):
            return os.path.join(
                installDir, "bin", mapping.getTargetFramework(current)
            )
        return os.path.join(installDir, "bin")

    def _getLibDir(self, component, process, mapping, current):
        installDir = component.getInstallDir(mapping, current)
        if isinstance(mapping, CSharpMapping):
            return os.path.join(installDir, "lib", "net8.0")
        return os.path.join(installDir, "lib")

    def getBuildSubDir(self, mapping, name, current):
        # Return the build sub-directory, to be overridden by specializations
        buildPlatform = current.driver.configs[mapping].buildPlatform
        buildConfig = current.driver.configs[mapping].buildConfig
        return os.path.join("build", buildPlatform, buildConfig)

    def getLdPathEnvName(self):
        return "LD_LIBRARY_PATH"

    def getInstallDir(self):
        return "/usr"

    def getNugetPackageDir(self, component, mapping, current):
        if not self.nugetPackageCache:
            return None
        return os.path.join(
            self.nugetPackageCache,
            component.getNugetPackage(mapping),
            component.getNugetPackageVersion(mapping),
        )

    def hasOpenSSL(self):
        # This is used by the IceSSL test suite to figure out how to setup certificates
        return False

    def getDotNetExe(self):
        return "dotnet"


class Darwin(Platform):
    def getDefaultBuildPlatform(self):
        return "macosx"

    def getLdPathEnvName(self):
        return "DYLD_LIBRARY_PATH"

    def getInstallDir(self):
        return "/opt/homebrew" if platform_machine() == "arm64" else "/usr/local"


class Linux(Platform):
    def __init__(self):
        Platform.__init__(self)
        self.multiArch = {}
        self.linuxId = None
        self.buildPlatform = None
        self.foreignPlatforms = []

    def init(self, component):
        Platform.init(self, component)
        self.parseBuildVariables(
            component,
            {
                "linux_id": ("linuxId", None),
                "build-platform": ("buildPlatform", None),
                "foreign-platforms": (
                    "foreignPlatforms",
                    lambda s: s.split(" ") if s else [],
                ),
            },
        )
        if self.linuxId in ["ubuntu", "debian"]:
            for p in [self.buildPlatform] + self.foreignPlatforms:
                self.multiArch[p] = run(
                    "dpkg-architecture -f -a{0} -qDEB_HOST_MULTIARCH 2> /dev/null".format(
                        p
                    )
                )

    def hasOpenSSL(self):
        return True

    def _getBinDir(self, component, process, mapping, current):
        installDir = component.getInstallDir(mapping, current)
        if isinstance(mapping, CSharpMapping):
            return Platform._getBinDir(self, component, process, mapping, current)

        if self.linuxId in ["ubuntu", "debian"]:
            binDir = os.path.join(
                installDir,
                "bin",
                self.multiArch[current.driver.configs[mapping].buildPlatform],
            )
            if os.path.exists(binDir):
                return binDir
        return os.path.join(installDir, "bin")

    def _getLibDir(self, component, process, mapping, current):
        installDir = component.getInstallDir(mapping, current)
        if isinstance(mapping, CSharpMapping):
            return Platform._getLibDir(self, component, process, mapping, current)

        buildPlatform = current.driver.configs[mapping].buildPlatform

        # PHP module is always installed in the lib directory for the default build platform
        if (
            isinstance(mapping, PhpMapping)
            and buildPlatform == self.getDefaultBuildPlatform()
        ):
            return os.path.join(installDir, "lib")

        if self.linuxId in ["centos", "rhel", "fedora"]:
            return os.path.join(
                installDir, "lib64" if buildPlatform == "x64" else "lib"
            )
        elif self.linuxId in ["ubuntu", "debian"]:
            return os.path.join(installDir, "lib", self.multiArch[buildPlatform])
        return os.path.join(installDir, "lib")

    def getBuildSubDir(self, mapping, name, current):
        buildPlatform = current.driver.configs[mapping].buildPlatform
        buildConfig = current.driver.configs[mapping].buildConfig
        if self.linuxId in ["ubuntu", "debian"]:
            return os.path.join("build", self.multiArch[buildPlatform], buildConfig)
        else:
            return os.path.join("build", buildPlatform, buildConfig)

    def getLinuxId(self):
        return self.linuxId


class Windows(Platform):
    def __init__(self):
        Platform.__init__(self)
        self.compiler = None

    def parseBuildVariables(self, component, variables):
        pass  # Nothing to do, we don't support the make build system on Windows

    def getDefaultBuildPlatform(self):
        return "Win32" if "x86" in os.environ.get("PLATFORM", "") else "x64"

    def getDefaultBuildConfig(self):
        return "Release"

    def getCompiler(self):
        if self.compiler is not None:
            return self.compiler
        if os.environ.get("CPP_COMPILER", "") != "":
            self.compiler = os.environ["CPP_COMPILER"]
        else:
            self.compiler = "v143"
        return self.compiler

    def getPlatformToolset(self):
        return self.getCompiler().replace("VC", "v")

    def _getBinDir(self, component, process, mapping, current):
        installDir = component.getInstallDir(mapping, current)
        platform = current.driver.configs[mapping].buildPlatform
        config = (
            "Debug"
            if current.driver.configs[mapping].buildConfig.find("Debug") >= 0
            else "Release"
        )

        if isinstance(mapping, CSharpMapping):
            return os.path.join(
                installDir, "bin", mapping.getTargetFramework(current)
            )
        elif isinstance(mapping, PhpMapping):
            return os.path.join(
                self.getNugetPackageDir(component, mapping, current),
                "build",
                "native",
                "bin",
                platform,
                config,
            )
        else:
            return os.path.join(installDir, "bin", platform, config)

    def _getLibDir(self, component, process, mapping, current):
        installDir = component.getInstallDir(mapping, current)
        if isinstance(mapping, CSharpMapping):
            return os.path.join(installDir, "lib/net8.0")
        else:
            platform = current.driver.configs[mapping].buildPlatform
            config = (
                "Debug"
                if current.driver.configs[mapping].buildConfig.find("Debug") >= 0
                else "Release"
            )
            if isinstance(mapping, PhpMapping):
                return os.path.join(
                    installDir,
                    "lib",
                    "php-{0}".format(current.config.phpVersion),
                    platform,
                    config,
                )
            if isinstance(mapping, MatlabMapping):
                return os.path.join(installDir, "lib", platform, config)
            else:
                return os.path.join(installDir, "bin", platform, config)

    def getBuildSubDir(self, mapping, name, current):
        buildPlatform = current.driver.configs[mapping].buildPlatform
        buildConfig = current.driver.configs[mapping].buildConfig
        if os.path.exists(
            os.path.join(current.testcase.getPath(current), "msbuild", name)
        ):
            return os.path.join("msbuild", name, buildPlatform, buildConfig)
        else:
            return os.path.join("msbuild", buildPlatform, buildConfig)

    def getLdPathEnvName(self):
        return "PATH"

    def getInstallDir(self):
        return None  # No default installation directory on Windows

    def getNugetPackageDir(self, component, mapping, current):
        if isinstance(mapping, CSharpMapping) and current.config.dotnet:
            return Platform.getNugetPackageDir(self, component, mapping, current)
        else:
            package = "{0}.{1}".format(
                component.getNugetPackage(mapping),
                component.getNugetPackageVersion(mapping),
            )
            # The package directory is either under the msbuild directory or in the mapping directory depending
            # on where the solution is located.
            if os.path.exists(os.path.join(mapping.path, "msbuild", "packages")):
                return os.path.join(mapping.path, "msbuild", "packages", package)
            else:
                return os.path.join(mapping.path, "packages", package)

    def getDotNetExe(self):
        try:
            return run("where dotnet").strip().splitlines()[0]
        except Exception:
            return None


def parseOptions(obj, options, mapped={}):
    # Transform configuration options provided on the command line to
    # object data members. The data members must be already set on the
    # object and with the correct type.
    if not hasattr(obj, "parsedOptions"):
        obj.parsedOptions = []
    remaining = []
    for o, a in options:
        if o.startswith("--"):
            o = o[2:]
        if o.startswith("-"):
            o = o[1:]
        if not a and o.startswith("no-"):
            a = "false"
            o = o[3:]
        if o in mapped:
            o = mapped[o]

        if hasattr(obj, o):
            if isinstance(getattr(obj, o), bool):
                setattr(obj, o, True if not a else (a.lower() in ["yes", "true", "1"]))
            elif isinstance(getattr(obj, o), list):
                argList = getattr(obj, o)
                argList.append(a)
            else:
                if not a and not isinstance(a, str):
                    a = "0"
                setattr(obj, o, type(getattr(obj, o))(a))
            if o not in obj.parsedOptions:
                obj.parsedOptions.append(o)
        else:
            remaining.append((o, a))
    options[:] = remaining


"""
Mapping abstract class. The mapping class provides mapping specific information.
Multiple components can share the same mapping rules as long as the layout is
similar.
"""


class Mapping(object):
    mappings = OrderedDict()
    disabled = OrderedDict()

    class Config(object):
        @classmethod
        def getSupportedArgs(self):
            return (
                "",
                [
                    "config=",
                    "platform=",
                    "protocol=",
                    "target=",
                    "compress",
                    "ipv6",
                    "no-ipv6",
                    "serialize",
                    "mx",
                    "cprops=",
                    "sprops=",
                ],
            )

        @classmethod
        def usage(self):
            pass

        @classmethod
        def commonUsage(self):
            print("")
            print("Mapping options:")
            print("--protocol=<prot>     Run with the given protocol.")
            print("--compress            Run the tests with protocol compression.")
            print("--ipv6                Use IPv6 addresses.")
            print("--serialize           Run with connection serialization.")
            print("--mx                  Run with metrics enabled.")
            print(
                "--cprops=<properties> Specifies a list of additional client properties."
            )
            print(
                "--sprops=<properties> Specifies a list of additional server properties."
            )
            print("--config=<config>     Build configuration for native executables.")
            print("--platform=<platform> Build platform for native executables.")

        def __init__(self, options=[]):
            # Build configuration
            self.parsedOptions = []
            self.buildConfig = os.environ.get("CONFIGS", "").split(" ")[0]
            if self.buildConfig:
                self.parsedOptions.append("buildConfig")
            else:
                self.buildConfig = platform.getDefaultBuildConfig()

            self.buildPlatform = os.environ.get("PLATFORMS", "").split(" ")[0]
            if self.buildPlatform:
                self.parsedOptions.append("buildPlatform")
            else:
                self.buildPlatform = platform.getDefaultBuildPlatform()

            self.pathOverride = ""
            self.protocol = "tcp"
            self.compress = False
            self.serialize = False
            self.ipv6 = False
            self.mx = False
            self.cprops = []
            self.sprops = []

            # Options bellow are not parsed by the base class by still initialized here for convenience (this
            # avoid having to check the configuration type)
            self.openssl = False
            self.browser = ""
            self.worker = False
            self.dotnet = False
            self.framework = ""
            self.android = False
            self.jacoco = ""
            self.device = ""
            self.avd = ""
            self.phpVersion = "7.1"
            self.python = sys.executable

            parseOptions(
                self, options, {"config": "buildConfig", "platform": "buildPlatform"}
            )

        def __str__(self):
            s = []
            for o in self.parsedOptions:
                v = getattr(self, o)
                if v:
                    s.append(o if isinstance(v, bool) else str(v))
            return ",".join(s)

        def getAll(self, current, testcase, rand=False):
            #
            # A generator to generate combinations of options (e.g.: tcp/compress/mx, ssl/ipv6/serialize, etc)
            #
            def gen(supportedOptions):
                if not supportedOptions:
                    yield self
                    return

                supportedOptions = supportedOptions.copy()
                supportedOptions.update(testcase.getMapping().getOptions(current))
                supportedOptions.update(testcase.getTestSuite().getOptions(current))
                supportedOptions.update(testcase.getOptions(current))

                for o in self.parsedOptions:
                    # Remove options which were explicitly set
                    if o in supportedOptions:
                        del supportedOptions[o]

                if len(supportedOptions) == 0:
                    yield self
                    return

                # Find the option with the longest list of values
                length = max([len(v) for v in supportedOptions.values()])

                # Replace the values with a cycle iterator on the values
                for k, v in supportedOptions.items():
                    supportedOptions[k] = itertools.cycle(
                        random.sample(v, len(v)) if rand else v
                    )

                # Now, for the length of the longest array of values, we return
                # an array with the supported option combinations
                for i in range(0, length):
                    options = []
                    for k, v in supportedOptions.items():
                        v = next(v)
                        if v:
                            if isinstance(v, bool):
                                if v:
                                    options.append(("--{0}".format(k), None))
                            else:
                                options.append(("--{0}".format(k), v))

                    # Add parsed options
                    for o in self.parsedOptions:
                        v = getattr(self, o)
                        if isinstance(v, bool):
                            if v:
                                options.append(("--{0}".format(o), None))
                        elif isinstance(v, list):
                            options += [("--{0}".format(o), e) for e in v]
                        else:
                            options.append(("--{0}".format(o), v))

                    yield self.__class__(options)

            return [
                c
                for c in gen(
                    current.driver.filterOptions(
                        current.driver.getComponent().getOptions(testcase, current)
                    )
                )
            ]

        def canRun(self, testId, current):
            if not current.driver.getComponent().canRun(
                testId, current.testcase.getMapping(), current
            ):
                return False

            options = {}
            options.update(current.testcase.getMapping().getOptions(current))
            options.update(current.testcase.getTestSuite().getOptions(current))
            options.update(current.testcase.getOptions(current))

            for k, v in options.items():
                if hasattr(self, k):
                    if getattr(self, k) not in v:
                        return False
                elif hasattr(current.driver, k):
                    if getattr(current.driver, k) not in v:
                        return False
            else:
                return True

        def cloneRunnable(self, current):
            #
            # Clone this configuration and make sure all the options are supported
            #
            options = {}
            options.update(current.testcase.getMapping().getOptions(current))
            options.update(current.testcase.getTestSuite().getOptions(current))
            options.update(current.testcase.getOptions(current))

            clone = copy.copy(self)
            for o in self.parsedOptions:
                if o in options and getattr(self, o) not in options[o]:
                    setattr(clone, o, options[o][0] if len(options[o]) > 0 else None)
            return clone

        def cloneAndOverrideWith(self, current):
            #
            # Clone this configuration and override options with options from the given configuration
            # (the parent configuration). This is useful when running cross-testing. For example, JS
            # tests don't support all the options so we clone the C++ configuration and override the
            # options that are set on the JS configuration.
            #
            clone = copy.copy(self)
            for o in current.config.parsedOptions + ["protocol"]:
                if o not in ["buildConfig", "buildPlatform"]:
                    setattr(clone, o, getattr(current.config, o))
            clone.parsedOptions = current.config.parsedOptions
            return clone

        def getArgs(self, process, current):
            return []

        def getProps(self, process, current):
            props = {}
            if isinstance(process, IceProcess):
                props["Ice.Warn.Connections"] = True
                if self.protocol:
                    props["Ice.Default.Protocol"] = self.protocol
                if self.compress:
                    props["Ice.Override.Compress"] = "1"
                if self.serialize:
                    props["Ice.ThreadPool.Server.Serialize"] = "1"

                if not isinstance(current.testsuite.getMapping(), JavaScriptMapping):
                    # JavaScript does not support the IPv6 properties
                    props["Ice.IPv6"] = self.ipv6
                    if self.ipv6:
                        props["Ice.PreferIPv6Address"] = True
                if self.mx:
                    props["Ice.Admin.Endpoints"] = (
                        'tcp -h "::1"' if self.ipv6 else "tcp -h 127.0.0.1"
                    )
                    props["Ice.Admin.InstanceName"] = (
                        "Server" if isinstance(process, Server) else "Client"
                    )
                    props["IceMX.Metrics.Debug.GroupBy"] = "id"
                    props["IceMX.Metrics.Parent.GroupBy"] = "parent"
                    props["IceMX.Metrics.All.GroupBy"] = "none"

                # Additional properties specified on the command line with --cprops or --sprops
                additionalProps = []
                if self.cprops and isinstance(process, Client):
                    additionalProps = self.cprops
                elif self.sprops and isinstance(process, Server):
                    additionalProps = self.sprops
                for pps in additionalProps:
                    for p in pps.split(" "):
                        if p.find("=") > 0:
                            (k, v) = p.split("=")
                            props[k] = v
                        else:
                            props[p] = True

            return props

    @classmethod
    def getByName(self, name):
        if name not in self.mappings:
            raise RuntimeError(
                "unknown mapping: `{0}', known mappings: `{1}'".format(
                    name, list(self.mappings)
                )
            )
        return self.mappings.get(name)

    @classmethod
    def getByPath(self, path):
        path = os.path.normpath(path)
        for m in self.mappings.values():
            if path.startswith(os.path.normpath(m.getTestDir())):
                return m

    @classmethod
    def getAllByPath(self, path):
        path = os.path.abspath(path)
        mappings = []
        for m in self.mappings.values():
            if path.startswith(m.getPath() + os.sep):
                mappings.append(m)
        return mappings

    @classmethod
    def add(self, name, mapping, component, path=None, enable=True):
        name = name.replace("\\", "/")
        m = mapping.init(name, component, path)
        if enable:
            self.mappings[name] = m
        else:
            self.disabled[name] = m

    @classmethod
    def disable(self, name):
        m = self.mappings[name]
        if m:
            self.disabled[name] = m
            del self.mappings[name]

    @classmethod
    def remove(self, name):
        del self.mappings[name]

    @classmethod
    def getAll(self, driver=None, includeDisabled=False):
        return [
            m
            for m in self.mappings.values()
            if not driver or driver.matchLanguage(str(m))
        ] + (
            [
                m
                for m in self.disabled.values()
                if not driver or driver.matchLanguage(str(m))
            ]
            if includeDisabled
            else []
        )

    def __init__(self, path=None):
        self.name = None
        self.path = os.path.abspath(path) if path else None
        self.testsuites = {}

    def init(self, name, component, path=None):
        self.name = name
        self.component = component
        if not self.path:
            self.path = os.path.normpath(
                os.path.join(self.component.getSourceDir(), path or name)
            )
        return self

    def __str__(self):
        return self.name

    def getTestDir(self):
        return self.component.getTestDir(self)

    def createConfig(self, options):
        config = self.Config(options)
        return self.component.overrideConfig(self, config)

    def filterTestSuite(self, testId, config, filters=[], rfilters=[]):
        if len(filters) > 0:
            for f in filters:
                if f.search(self.name + "/" + testId):
                    break
            else:
                return True

        if len(rfilters) > 0:
            for f in rfilters:
                if f.search(self.name + "/" + testId):
                    return True

        return False

    def loadTestSuites(self, tests, config, filters=[], rfilters=[]):
        global currentMapping
        currentMapping = self
        global currentConfig
        currentConfig = config
        try:
            origsyspath = sys.path
            prefix = os.path.commonprefix([toplevel, self.component.getScriptDir()])
            moduleprefix = (
                self.component.getScriptDir()[len(prefix) + 1 :].replace(os.sep, ".")
                + "."
            )
            sys.path = [prefix] + sys.path
            for test in tests or [""]:
                testDir = self.component.getTestDir(self)
                for root, dirs, files in os.walk(
                    os.path.join(testDir, test.replace("/", os.sep))
                ):
                    testId = root[len(testDir) + 1 :]
                    if os.sep != "/":
                        testId = testId.replace(os.sep, "/")

                    if self.filterTestSuite(testId, config, filters, rfilters):
                        continue

                    #
                    # First check if there's a test.py file in the test directory, if there's one use it.
                    #
                    if "test.py" in files:
                        #
                        # WORKAROUND for Python issue 15230 (fixed in 3.2) where run_path doesn't work correctly.
                        #
                        # runpy.run_path(os.path.join(root, "test.py"))
                        origsyspath = sys.path
                        sys.path = [root] + sys.path
                        runpy.run_module("test", init_globals=globals(), run_name=root)
                        origsyspath = sys.path
                        continue

                    #
                    # If there's no test.py file in the test directory, we check if there's a common
                    # script for the test in scripts/tests. If there's one we use it.
                    #
                    if os.path.isfile(
                        os.path.join(self.component.getScriptDir(), testId + ".py")
                    ):
                        runpy.run_module(
                            moduleprefix + testId.replace("/", "."),
                            init_globals=globals(),
                            run_name=root,
                        )
                        continue

                    #
                    # Finally, we try to "discover/compute" the test by looking up for well-known
                    # files.
                    #
                    testcases = self.computeTestCases(testId, files)
                    if testcases:
                        TestSuite(root, testcases)
        finally:
            currentMapping = None
            sys.path = origsyspath

    def getTestSuites(self, ids=[]):
        if not ids:
            return self.testsuites.values()
        return [
            self.testsuites[testSuiteId]
            for testSuiteId in ids
            if testSuiteId in self.testsuites
        ]

    def addTestSuite(self, testsuite):
        assert len(testsuite.path) > len(self.component.getTestDir(self)) + 1
        testSuiteId = testsuite.path[
            len(self.component.getTestDir(self)) + 1 :
        ].replace("\\", "/")
        self.testsuites[testSuiteId] = testsuite
        return testSuiteId

    def findTestSuite(self, testsuite):
        return self.testsuites.get(
            testsuite if isinstance(testsuite, str) else testsuite.id
        )

    def computeTestCases(self, testId, files):
        # Instantiate a new test suite if the directory contains well-known source files.
        def checkFile(f, m):
            try:
                # If given mapping is same as local mapping, just check the files set, otherwise check
                # with the mapping
                return (
                    (self.getDefaultSource(f) in files)
                    if m == self
                    else m.hasSource(testId, f)
                )
            except KeyError:
                # Expected if the mapping doesn't support the process type
                return False

        def checkClient(f):
            return checkFile(f, self.getClientMapping(testId))

        def checkServer(f):
            return checkFile(f, self.getServerMapping(testId))

        testcases = []
        if checkClient("client") and checkServer("server"):
            testcases.append(ClientServerTestCase())
        if (
            checkClient("client")
            and checkServer("serveramd")
            and self.getServerMapping(testId) == self
        ):
            testcases.append(ClientAMDServerTestCase())
        if checkClient("client") and len(testcases) == 0:
            testcases.append(ClientTestCase())
        if checkClient("collocated"):
            testcases.append(CollocatedTestCase())
        if len(testcases) > 0:
            return testcases

    def hasSource(self, testId, processType):
        try:
            return os.path.exists(
                os.path.join(
                    self.component.getTestDir(self),
                    testId,
                    self.getDefaultSource(processType),
                )
            )
        except KeyError:
            return False

    def getPath(self):
        return self.path

    def getTestCwd(self, process, current):
        return current.testcase.getPath(current)

    def getDefaultSource(self, processType):
        default = self.component.getDefaultSource(self, processType)
        if default:
            return default
        return self._getDefaultSource(processType)

    def getDefaultProcesses(self, processType, testsuite):
        default = self.component.getDefaultProcesses(
            self, processType, testsuite.getId()
        )
        if default:
            return default
        return self._getDefaultProcesses(processType)

    def getDefaultExe(self, processType):
        default = self.component.getDefaultExe(self, processType)
        if default:
            return default
        return self._getDefaultExe(processType)

    def _getDefaultSource(self, processType):
        return processType

    def _getDefaultProcesses(self, processType):
        #
        # If no server or client is explicitly set with a testcase, getDefaultProcess is called
        # to figure out which process class to instantiate.
        #
        name, ext = os.path.splitext(self.getDefaultSource(processType))
        if name in globals():
            return [globals()[name]()]
        return (
            [Server()]
            if processType.startswith("server")
            else [Client()]
            if processType
            else []
        )

    def _getDefaultExe(self, processType):
        return os.path.splitext(self.getDefaultSource(processType))[0]

    def getClientMapping(self, testId=None):
        # The client mapping is always the same as this mapping.
        return self

    def getServerMapping(self, testId=None):
        # Can be overridden for client-only mapping that relies on another mapping for servers
        return self

    def getBuildDir(self, name, current):
        return platform.getBuildSubDir(self, name, current)

    def getCommandLine(self, current, process, exe, args):
        cmd = ""
        if process.isFromBinDir():
            # If it's a process from the bin directory, the location is platform specific
            # so we check with the platform.
            cmd = os.path.join(self.component.getBinDir(process, self, current), exe)
        elif current.testcase:
            # If it's a process from a testcase, the binary is in the test build directory.
            cmd = os.path.join(
                current.testcase.getPath(current), current.getBuildDir(exe), exe
            )
        else:
            cmd = exe

        if isinstance(platform, Windows) and not exe.endswith(".exe"):
            cmd += ".exe"

        return cmd + " " + args if args else cmd

    def getProps(self, process, current):
        props = {}
        if isinstance(process, IceProcess):
            if current.config.protocol in ["bt", "bts"]:
                props["Ice.Plugin.IceBT"] = self.getPluginEntryPoint(
                    "IceBT", process, current
                )
            if current.config.protocol in ["ssl", "wss", "bts", "iaps"]:
                props.update(self.getSSLProps(process, current))
        return props

    def getSSLProps(self, process, current):
        sslProps = {
            "IceSSL.Password": "password",
            "IceSSL.DefaultDir": ""
            if current.config.buildPlatform == "iphoneos"
            else os.path.join(self.component.getSourceDir(), "certs"),
        }

        #
        # If the client doesn't support client certificates, set IceSSL.VerifyPeer to 0
        #
        if isinstance(process, Server):
            if isinstance(current.testsuite.getMapping(), JavaScriptMixin):
                sslProps["IceSSL.VerifyPeer"] = 0

        return sslProps

    def getArgs(self, process, current):
        return []

    def getEnv(self, process, current):
        return {}

    def getOptions(self, current):
        return {}


#
# A Runnable can be used as a "client" for in test cases, it provides
# implements run, setup and teardown methods.
#


class Runnable(object):
    def __init__(self, desc=None):
        self.desc = desc

    def setup(self, current):
        # Only called when ran from testcase
        pass

    def teardown(self, current, success):
        # Only called when ran from testcase
        pass

    def run(self, current):
        pass


#
# A Process describes how to run an executable process.
#


class Process(Runnable):
    processType = None

    def __init__(
        self,
        exe=None,
        outfilters=None,
        quiet=False,
        args=None,
        props=None,
        envs=None,
        desc=None,
        mapping=None,
        preexec_fn=None,
        traceProps=None,
    ):
        Runnable.__init__(self, desc)
        self.exe = exe
        self.outfilters = outfilters or []
        self.quiet = quiet
        self.args = args or []
        self.props = props or {}
        self.traceProps = traceProps or {}
        self.envs = envs or {}
        self.mapping = mapping
        self.preexec_fn = preexec_fn

    def __str__(self):
        if not self.exe:
            return str(self.__class__)
        return self.exe + (" ({0})".format(self.desc) if self.desc else "")

    def getOutput(self, current, encoding="utf-8"):
        assert self in current.processes

        def d(s):
            return s.decode(encoding) if isinstance(s, bytes) else s

        output = d(current.processes[self].getOutput())
        try:
            # Apply outfilters to the output
            if len(self.outfilters) > 0:
                lines = output.split("\n")
                newLines = []
                previous = ""
                for line in [line + "\n" for line in lines]:
                    for f in self.outfilters:
                        if isinstance(f, types.LambdaType) or isinstance(
                            f, types.FunctionType
                        ):
                            line = f(line)
                        elif f.search(line):
                            break
                    else:
                        if line.endswith("\n"):
                            if previous:
                                newLines.append(previous + line)
                                previous = ""
                            else:
                                newLines.append(line)
                        else:
                            previous += line
                output = "".join(newLines)
            output = output.strip()
            return output + "\n" if output else ""
        except Exception as ex:
            print("unexpected exception while filtering process output:\n" + str(ex))
            raise

    def run(self, current, args=[], props={}, exitstatus=0, timeout=None):
        class WatchDog:
            def __init__(self, timeout):
                self.lastProgressTime = time.time()
                self.lock = threading.Lock()

            def reset(self):
                with self.lock:
                    self.lastProgressTime = time.time()

            def timedOut(self, timeout):
                with self.lock:
                    return (time.time() - self.lastProgressTime) >= timeout

        watchDog = WatchDog(timeout)
        self.start(current, args, props, watchDog=watchDog)
        process = current.processes[self]

        if timeout is None:
            # If it's not a local process use a large timeout as the watch dog might not
            # get invoked (TODO: improve remote processes to use the watch dog)
            timeout = 60 if isinstance(process, Expect.Expect) else 480

        if not self.quiet and not current.driver.isWorkerThread():
            # Print out the process output to stdout if we're running the client form the main thread.
            process.trace(self.outfilters)
        try:
            while True:
                try:
                    process.waitSuccess(exitstatus=exitstatus, timeout=30)
                    break
                except KeyboardInterrupt:
                    current.driver.setInterrupt(True)
                    raise
                except Expect.TIMEOUT:
                    if watchDog and watchDog.timedOut(timeout):
                        print(
                            "process {0} is hanging - {1}".format(
                                process, time.strftime("%x %X")
                            )
                        )
                        if current.driver.isInterrupted():
                            self.stop(current, False, exitstatus)
                            raise
        finally:
            self.stop(current, True, exitstatus)

    def getEffectiveArgs(self, current, args):
        allArgs = []
        allArgs += current.driver.getArgs(self, current)
        allArgs += current.config.getArgs(self, current)
        allArgs += self.getMapping(current).getArgs(self, current)
        allArgs += current.testcase.getArgs(self, current)
        allArgs += self.getArgs(current)
        allArgs += self.args(self, current) if callable(self.args) else self.args
        allArgs += args
        allArgs = [str(a) for a in allArgs]
        return allArgs

    def getEffectiveProps(self, current, props):
        allProps = {}
        allProps.update(current.driver.getProps(self, current))
        allProps.update(current.driver.getComponent().getProps(self, current))
        allProps.update(current.config.getProps(self, current))
        allProps.update(self.getMapping(current).getProps(self, current))
        allProps.update(current.testcase.getProps(self, current))
        allProps.update(self.getProps(current))
        allProps.update(
            self.props(self, current) if callable(self.props) else self.props
        )
        allProps.update(props)
        return allProps

    def getEffectiveEnv(self, current):
        def merge(envs, newEnvs):
            if (
                platform.getLdPathEnvName() in newEnvs
                and platform.getLdPathEnvName() in envs
            ):
                newEnvs[platform.getLdPathEnvName()] += (
                    os.pathsep + envs[platform.getLdPathEnvName()]
                )
            envs.update(newEnvs)

        allEnvs = {}
        merge(allEnvs, current.driver.getComponent().getEnv(self, current))
        merge(allEnvs, self.getMapping(current).getEnv(self, current))
        merge(allEnvs, current.testcase.getEnv(self, current))
        merge(allEnvs, self.getEnv(current))
        merge(allEnvs, self.envs(self, current) if callable(self.envs) else self.envs)
        return allEnvs

    def getEffectiveTraceProps(self, current):
        traceProps = {}
        traceProps.update(current.testcase.getTraceProps(self, current))
        traceProps.update(
            self.traceProps(self, current)
            if callable(self.traceProps)
            else self.traceProps
        )
        return traceProps

    def start(self, current, args=[], props={}, watchDog=None):
        allArgs = self.getEffectiveArgs(current, args)
        allProps = self.getEffectiveProps(current, props)
        allEnvs = self.getEffectiveEnv(current)

        processController = current.driver.getProcessController(current, self)
        current.processes[self] = processController.start(
            self, current, allArgs, allProps, allEnvs, watchDog
        )
        try:
            self.waitForStart(current)
        except Exception:
            self.stop(current)
            raise

    def waitForStart(self, current):
        # To be overridden in specialization to wait for a token indicating the process readiness.
        pass

    def stop(self, current, waitSuccess=False, exitstatus=0):
        if self in current.processes:
            process = current.processes[self]
            try:
                # Wait for the process to exit successfully by itself.
                if not process.isTerminated() and waitSuccess:
                    while True:
                        try:
                            process.waitSuccess(exitstatus=exitstatus, timeout=30)
                            break
                        except KeyboardInterrupt:
                            current.driver.setInterrupt(True)
                            raise
                        except Expect.TIMEOUT:
                            print(
                                "process {0} is hanging on shutdown - {1}".format(
                                    process, time.strftime("%x %X")
                                )
                            )
                            if current.driver.isInterrupted():
                                raise
                        except RuntimeError as ex:
                            output = self.getOutput(current)
                            if output:
                                raise RuntimeError(str(ex) + output)
                            else:
                                raise ex
            finally:
                if not process.isTerminated():
                    process.terminate()
                if (
                    not self.quiet
                ):  # Write the output to the test case (but not on stdout)
                    current.write(self.getOutput(current), stdout=False)

    def teardown(self, current, success):
        if self in current.processes:
            current.processes[self].teardown(current, success)

    def expect(self, current, pattern, timeout=60):
        assert self in current.processes and isinstance(
            current.processes[self], Expect.Expect
        )
        return current.processes[self].expect(pattern, timeout)

    def expectall(self, current, pattern, timeout=60):
        assert self in current.processes and isinstance(
            current.processes[self], Expect.Expect
        )
        return current.processes[self].expectall(pattern, timeout)

    def sendline(self, current, data):
        assert self in current.processes and isinstance(
            current.processes[self], Expect.Expect
        )
        return current.processes[self].sendline(data)

    def getMatch(self, current):
        assert self in current.processes and isinstance(
            current.processes[self], Expect.Expect
        )
        return current.processes[self].match

    def isStarted(self, current):
        return self in current.processes and not current.processes[self].isTerminated()

    def isFromBinDir(self):
        return False

    def isReleaseOnly(self):
        return False

    def getArgs(self, current):
        return []

    def getProps(self, current):
        return {}

    def getEnv(self, current):
        return {}

    def getMapping(self, current):
        return self.mapping or current.testcase.getMapping()

    def getExe(self, current):
        processType = self.processType or current.testcase.getProcessType(self)
        return self.exe or self.getMapping(current).getDefaultExe(processType)

    def getCommandLine(self, current, args=""):
        return (
            self.getMapping(current)
            .getCommandLine(current, self, self.getExe(current), args)
            .strip()
        )


#
# A simple client (used to run Slice/IceUtil clients for example)
#


class SimpleClient(Process):
    pass


#
# An IceProcess specialization class. This is used by drivers to figure out if
# the process accepts Ice configuration properties.
#


class IceProcess(Process):
    pass


#
# An Ice server process. It's possible to configure when the server is considered
# ready by setting readyCount or ready. The start method will only return once
# the server is considered "ready". It can also be configure to wait (the default)
# or not wait for shutdown when the stop method is invoked.
#


class Server(IceProcess):
    def __init__(
        self,
        exe=None,
        waitForShutdown=True,
        readyCount=1,
        ready=None,
        startTimeout=300,
        *args,
        **kargs,
    ):
        IceProcess.__init__(self, exe, *args, **kargs)
        self.waitForShutdown = waitForShutdown
        self.readyCount = readyCount
        self.ready = ready
        self.startTimeout = startTimeout

    def getProps(self, current):
        props = IceProcess.getProps(self, current)

        mapping = self.getMapping(current)

        if not isinstance(mapping, JavaScriptMapping):
            props.update(
                {
                    "Ice.ThreadPool.Server.Size": 1,
                    "Ice.ThreadPool.Server.SizeMax": 3,
                    "Ice.ThreadPool.Server.SizeWarn": 0,
                }
            )
        props.update(
            current.driver.getProcessProps(
                current,
                self.ready,
                self.readyCount + (1 if current.config.mx else 0),
                mapping,
            )
        )
        return props

    def waitForStart(self, current):
        # Wait for the process to be ready
        current.processes[self].waitReady(
            self.ready,
            self.readyCount + (1 if current.config.mx else 0),
            self.startTimeout,
        )

        # Filter out remaining ready messages
        self.outfilters.append(re.compile("[^\n]+ ready"))

        # If we are not asked to be quiet and running from the main thread, print the server output
        if not self.quiet and not current.driver.isWorkerThread():
            current.processes[self].trace(self.outfilters)

    def stop(self, current, waitSuccess=False, exitstatus=0):
        IceProcess.stop(self, current, waitSuccess and self.waitForShutdown, exitstatus)


#
# An Ice client process.
#


class Client(IceProcess):
    pass


#
# Executables for processes inheriting this marker class are looked up in the
# Ice distribution bin directory.
#


class ProcessFromBinDir:
    def isFromBinDir(self):
        return True


#
# Executables for processes inheriting this marker class are only provided
# as a Release executable on Windows
#


class ProcessIsReleaseOnly:
    def isReleaseOnly(self):
        return True


class SliceTranslator(ProcessFromBinDir, ProcessIsReleaseOnly, SimpleClient):
    def __init__(self, translator):
        SimpleClient.__init__(
            self, exe=translator, quiet=True, mapping=Mapping.getByName("cpp")
        )

    def getCommandLine(self, current, args=""):
        #
        # Look for slice2py installed by pip if not found in the bin directory
        #
        if self.exe == "slice2py":
            translator = self.getMapping(current).getCommandLine(
                current, self, self.getExe(current), ""
            )
            if not os.path.exists(translator):
                translator = sys.executable + " -m slice2py"
            return (translator + " " + args).strip()
        else:
            return Process.getCommandLine(self, current, args)


class ServerAMD(Server):
    pass


class Collocated(Client):
    pass


class EchoServer(Server):
    def __init__(self):
        Server.__init__(
            self, mapping=Mapping.getByName("cpp"), quiet=True, waitForShutdown=False
        )

    def getProps(self, current):
        props = Server.getProps(self, current)
        props["Ice.MessageSizeMax"] = (
            8192  # Don't limit the amount of data to transmit between client/server
        )
        return props

    def getCommandLine(self, current, args=""):
        current.push(self.mapping.findTestSuite("Ice/echo").findTestCase("server"))
        try:
            return Server.getCommandLine(self, current, args)
        finally:
            current.pop()


#
# A test case is composed of servers and clients. When run, all servers are started
# sequentially. When the servers are ready, the clients are also ran sequentially.
# Once all the clients are terminated, the servers are stopped (which waits for the
# successful completion of the server).
#
# A TestCase is also a "Runnable", like the Process class. In other words, it can be
# used a client to allow nested test cases.
#


class TestCase(Runnable):
    def __init__(
        self,
        name,
        client=None,
        clients=None,
        server=None,
        servers=None,
        args=None,
        props=None,
        envs=None,
        options=None,
        desc=None,
        traceProps=None,
    ):
        Runnable.__init__(self, desc)

        self.name = name
        self.parent = None
        self.mapping = None
        self.testsuite = None
        self.options = options or {}
        self.args = args or []
        self.props = props or {}
        self.traceProps = traceProps or {}
        self.envs = envs or {}

        #
        # Setup client list, "client" can be a string in which case it's assumed to
        # to the client executable name.
        #
        self.clients = clients
        if client:
            client = Client(exe=client) if isinstance(client, str) else client
            self.clients = [client] if not self.clients else self.clients + [client]

        #
        # Setup server list, "server" can be a string in which case it's assumed to
        # to the server executable name.
        #
        self.servers = servers
        if server:
            server = Server(exe=server) if isinstance(server, str) else server
            self.servers = [server] if not self.servers else self.servers + [server]

    def __str__(self):
        return self.name

    def init(self, mapping, testsuite):
        # init is called when the testcase is added to the given testsuite
        self.mapping = mapping
        self.testsuite = testsuite

        #
        # If no clients are explicitly specified, we instantiate one if getClientType()
        # returns the type of client to instantiate (client, collocated, etc)
        #
        testId = self.testsuite.getId()
        if not self.clients:
            if self.getClientType():
                self.clients = self.mapping.getClientMapping(
                    testId
                ).getDefaultProcesses(self.getClientType(), testsuite)
            else:
                self.clients = []

        #
        # If no servers are explicitly specified, we instantiate one if getServerType()
        # returns the type of server to instantiate (server, serveramd, etc)
        #
        if not self.servers:
            if self.getServerType():
                self.servers = self.mapping.getServerMapping(
                    testId
                ).getDefaultProcesses(self.getServerType(), testsuite)
            else:
                self.servers = []

    def getOptions(self, current):
        return self.options(current) if callable(self.options) else self.options

    def canRun(self, current):
        # Can be overridden
        return True

    def setupServerSide(self, current):
        # Can be overridden to perform setup activities before the server side is started
        pass

    def teardownServerSide(self, current, success):
        # Can be overridden to perform terddown after the server side is stopped
        pass

    def setupClientSide(self, current):
        # Can be overridden to perform setup activities before the client side is started
        pass

    def teardownClientSide(self, current, success):
        # Can be overridden to perform terddown after the client side is stopped
        pass

    def startServerSide(self, current):
        for server in self.servers:
            self._startServer(current, server)

    def stopServerSide(self, current, success):
        for server in reversed(self.servers):
            self._stopServer(current, server, success)

    def runClientSide(self, current):
        for client in self.clients:
            self._runClient(current, client)

    def getTestSuite(self):
        return self.testsuite

    def getParent(self):
        return self.parent

    def getName(self):
        return self.name

    def getPath(self, current):
        path = self.testsuite.getPath()
        if current.config.pathOverride:
            return path.replace(toplevel, current.config.pathOverride)
        else:
            return path

    def getMapping(self):
        return self.mapping

    def getArgs(self, process, current):
        return self.args

    def getProps(self, process, current):
        return self.props

    def getTraceProps(self, process, current):
        return self.traceProps

    def getEnv(self, process, current):
        return self.envs

    def getProcessType(self, process):
        if process in self.clients:
            return self.getClientType()
        elif process in self.servers:
            return self.getServerType()
        elif isinstance(process, Server):
            return self.getServerType()
        else:
            return self.getClientType()

    def getClientType(self):
        # Overridden by test case specialization to specify the type of client to instantiate
        # if no client is explicitly provided
        return None

    def getServerType(self):
        # Overridden by test case specialization to specify the type of client to instantiate
        # if no server is explicitly provided
        return None

    def getServerTestCase(self, cross=None):
        testsuite = (
            (cross or self.mapping)
            .getServerMapping(self.testsuite.getId())
            .findTestSuite(self.testsuite)
        )
        return testsuite.findTestCase(self) if testsuite else None

    def getClientTestCase(self):
        testsuite = self.mapping.getClientMapping(self.testsuite.getId()).findTestSuite(
            self.testsuite
        )
        return testsuite.findTestCase(self) if testsuite else None

    def _startServerSide(self, current):
        # Set the host to use for the server side
        current.push(self)
        current.host = current.driver.getProcessController(current).getHost(current)
        self.setupServerSide(current)
        try:
            self.startServerSide(current)
            return current.host
        except Exception:
            self._stopServerSide(current, False)
            raise
        finally:
            current.pop()

    def _stopServerSide(self, current, success):
        current.push(self)
        try:
            self.stopServerSide(current, success)
        finally:
            for server in reversed(self.servers):
                if server.isStarted(current):
                    self._stopServer(current, server, False)
            self.teardownServerSide(current, success)
            current.pop()

    def _startServer(self, current, server):
        if server.desc:
            current.write("starting {0}... ".format(server.desc))
        server.setup(current)
        server.start(current)
        if server.desc:
            current.writeln("ok")

    def _stopServer(self, current, server, success):
        try:
            server.stop(current, success)
        except Exception:
            success = False
            raise
        finally:
            server.teardown(current, success)

    def _runClientSide(self, current, host=None):
        current.push(self, host)
        self.setupClientSide(current)
        success = False
        try:
            self.runClientSide(current)
            success = True
        finally:
            self.teardownClientSide(current, success)
            current.pop()

    def _runClient(self, current, client):
        success = False
        if client.desc:
            current.writeln("running {0}...".format(client.desc))
        client.setup(current)
        try:
            client.run(current)
            success = True
        finally:
            client.teardown(current, success)

    def run(self, current):
        try:
            current.push(self)
            if not self.parent:
                current.result.started(current)
            self.setup(current)
            self.runWithDriver(current)
            self.teardown(current, True)
            if not self.parent:
                current.result.succeeded(current)
        except Exception as ex:
            self.teardown(current, False)
            if not self.parent:
                current.result.failed(
                    current, traceback.format_exc() if current.driver.debug else str(ex)
                )
            raise
        finally:
            current.pop()


class ClientTestCase(TestCase):
    def __init__(self, name="client", *args, **kargs):
        TestCase.__init__(self, name, *args, **kargs)

    def runWithDriver(self, current):
        current.driver.runTestCase(current)

    def getClientType(self):
        return "client"


class ClientServerTestCase(ClientTestCase):
    def __init__(self, name="client/server", *args, **kargs):
        TestCase.__init__(self, name, *args, **kargs)

    def runWithDriver(self, current):
        current.driver.runClientServerTestCase(current)

    def getServerType(self):
        return "server"


class CollocatedTestCase(ClientTestCase):
    def __init__(self, name="collocated", *args, **kargs):
        TestCase.__init__(self, name, *args, **kargs)

    def getClientType(self):
        return "collocated"


class ClientAMDServerTestCase(ClientServerTestCase):
    def __init__(self, name="client/amd server", *args, **kargs):
        ClientServerTestCase.__init__(self, name, *args, **kargs)

    def getServerType(self):
        return "serveramd"


class Result:
    def getKey(self, current):
        return (
            (current.testcase, current.config)
            if isinstance(current, Driver.Current)
            else current
        )

    def getDesc(self, current):
        return current.desc if isinstance(current, Driver.Current) else ""

    def __init__(self, testsuite, writeToStdout):
        self.testsuite = testsuite
        self._failed = {}
        self._skipped = {}
        self._stdout = StringIO()
        self._writeToStdout = writeToStdout
        self._testcases = {}
        self._duration = 0
        self._testCaseDuration = 0

    def start(self):
        self._duration = time.time()

    def finished(self):
        self._duration = time.time() - self._duration

    def started(self, current):
        self._testCaseDuration = time.time()
        self._start = self._stdout.tell()

    def failed(self, current, exception):
        print(exception)
        key = self.getKey(current)
        self._testCaseDuration = time.time() - self._testCaseDuration
        self.writeln("\ntest in {0} failed:\n{1}".format(self.testsuite, exception))
        self._testcases[key] = (
            self._start,
            self._stdout.tell(),
            self._testCaseDuration,
            self.getDesc(current),
        )
        self._failed[key] = exception

        # If ADDRINUSE, dump the current processes
        output = self.getOutput(key)
        for s in ["EADDRINUSE", "Address already in use"]:
            if output.find(s) >= 0:
                if isinstance(platform, Windows):
                    self.writeln(run("netstat -on"))
                    self.writeln(
                        run('powershell.exe "Get-Process | Select id,name,path"')
                    )
                else:
                    self.writeln(run("lsof -n -P -i; ps ax"))

    def succeeded(self, current):
        key = self.getKey(current)
        self._testCaseDuration = time.time() - self._testCaseDuration
        self._testcases[key] = (
            self._start,
            self._stdout.tell(),
            self._testCaseDuration,
            self.getDesc(current),
        )

    def skipped(self, current, reason):
        self.writeln("skipped, " + reason)
        self._skipped[self.getKey(current)] = reason

    def isSuccess(self):
        return len(self._failed) == 0

    def getFailed(self):
        return self._failed

    def getDuration(self):
        return self._duration

    def getOutput(self, key=None):
        if key:
            if key in self._testcases:
                (start, end, duration, desc) = self._testcases[key]
                self._stdout.seek(start)
                try:
                    return self._stdout.read(end - start)
                finally:
                    self._stdout.seek(0, os.SEEK_END)

        return self._stdout.getvalue()

    def write(self, msg, stdout=True):
        if self._writeToStdout and stdout:
            try:
                sys.stdout.write(msg)
            except UnicodeEncodeError:
                #
                # The console doesn't support the encoding of the message, we convert the message
                # to an UTF-8 byte sequence and print out the byte sequence. We replace all the
                # double backslash from the byte sequence string representation to single back
                # slash.
                #
                sys.stdout.write(str(msg.encode("utf-8")).replace("\\\\", "\\"))
            sys.stdout.flush()
        self._stdout.write(msg)

    def writeln(self, msg, stdout=True):
        if self._writeToStdout and stdout:
            try:
                print(msg)
            except UnicodeEncodeError:
                #
                # The console doesn't support the encoding of the message, we convert the message
                # to an UTF-8 byte sequence and print out the byte sequence. We replace all the
                # double backslash from the byte sequence string representation to single back
                # slash.
                #
                print(str(msg.encode("utf-8")).replace("\\\\", "\\"))
        self._stdout.write(msg)
        self._stdout.write("\n")

    def writeAsXml(self, out, hostname=""):
        out.write(
            '  <testsuite tests="{0}" failures="{1}" skipped="{2}" time="{3:.9f}" name="{5}/{4}">\n'.format(
                len(self._testcases) - 2,
                len(self._failed),
                0,
                self._duration,
                self.testsuite,
                self.testsuite.getMapping(),
            )
        )

        for k, v in self._testcases.items():
            if isinstance(k, str):
                # Don't keep track of setup/teardown steps
                continue

            # Don't write skipped tests, this doesn't really provide useful information and clutters
            # the output.
            if k in self._skipped:
                continue

            (tc, cf) = k
            (s, e, d, c) = v
            if c:
                name = "{0} [{1}]".format(tc, c)
            else:
                name = str(tc)
            if hostname:
                name += " on " + hostname
            out.write(
                '    <testcase name="{0}" time="{1:.9f}" classname="{2}.{3}">\n'.format(
                    escapeXml(name),
                    d,
                    self.testsuite.getMapping(),
                    self.testsuite.getId().replace("/", "."),
                )
            )
            if k in self._failed:
                last = self._failed[k].strip().split("\n")
                if len(last) > 0:
                    last = last[len(last) - 1]
                if hostname:
                    last = "Failed on {0}\n{1}".format(hostname, last)
                out.write(
                    "      <failure message={1}>{0}</failure>\n".format(
                        escapeXml(self._failed[k]), escapeXml(last, True)
                    )
                )
            # elif k in self._skipped:
            #     out.write('      <skipped message="{0}"/>\n'.format(escapeXml(self._skipped[k], True)))
            out.write("      <system-out>\n")
            if hostname:
                out.write("Running on {0}\n".format(hostname))
            out.write(escapeXml(self.getOutput(k)))
            out.write("      </system-out>\n")
            out.write("    </testcase>\n")

        out.write("</testsuite>\n")


class TestSuite(object):
    def __init__(
        self,
        path,
        testcases=None,
        options=None,
        libDirs=None,
        runOnMainThread=False,
        chdir=False,
        multihost=True,
        mapping=None,
    ):
        global currentMapping
        self.path = (
            os.path.dirname(path) if os.path.basename(path) == "test.py" else path
        )
        self.mapping = currentMapping or Mapping.getByPath(self.path)
        self.id = self.mapping.addTestSuite(self)
        self.options = options or {}
        self.libDirs = libDirs or []
        self.runOnMainThread = runOnMainThread
        self.chdir = chdir
        self.multihost = multihost
        if self.chdir:
            # Only tests running on main thread can change the current working directory
            self.runOnMainThread = True

        if testcases is None:
            files = [
                f
                for f in os.listdir(self.path)
                if os.path.isfile(os.path.join(self.path, f))
            ]
            testcases = self.mapping.computeTestCases(self.id, files)
        self.testcases = OrderedDict()
        for testcase in testcases if testcases else []:
            testcase.init(self.mapping, self)
            if testcase.name in self.testcases:
                raise RuntimeError(
                    "duplicate testcase {0} in testsuite {1}".format(testcase, self)
                )
            self.testcases[testcase.name] = testcase

    def __str__(self):
        return self.id

    def getId(self):
        return self.id

    def getOptions(self, current):
        return self.options(current) if callable(self.options) else self.options

    def getPath(self):
        return self.path

    def getMapping(self):
        return self.mapping

    def getLibDirs(self):
        return self.libDirs

    def isMainThreadOnly(self, driver):
        if self.runOnMainThread or driver.getComponent().isMainThreadOnly(self.id):
            return True

        if isinstance(self.mapping, MatlabMapping):
            return True

        config = driver.configs[self.mapping]
        if "iphone" in config.buildPlatform or config.browser or config.android:
            return True  # Not supported yet for tests that require a remote process controller
        return False

    def addTestCase(self, testcase):
        if testcase.name in self.testcases:
            raise RuntimeError(
                "duplicate testcase {0} in testsuite {1}".format(testcase, self)
            )
        testcase.init(self.mapping, self)
        self.testcases[testcase.name] = testcase

    def findTestCase(self, testcase):
        return self.testcases.get(
            testcase if isinstance(testcase, str) else testcase.name
        )

    def getTestCases(self):
        return self.testcases.values()

    def setup(self, current):
        pass

    def run(self, current):
        try:
            current.result.start()
            cwd = None
            if self.chdir:
                cwd = os.getcwd()
                os.chdir(self.path)
            current.driver.runTestSuite(current)
        finally:
            if cwd:
                os.chdir(cwd)
            current.result.finished()

    def teardown(self, current, success):
        pass

    def isMultiHost(self):
        return self.multihost


class ProcessController:
    def __init__(self, current):
        pass

    def start(self, process, current, args, props, envs, watchDog):
        raise NotImplementedError()

    def destroy(self, driver):
        pass


class LocalProcessController(ProcessController):
    class LocalProcess(Expect.Expect):
        def __init__(self, traceFile, *args, **kargs):
            Expect.Expect.__init__(self, *args, **kargs)
            self.traceFile = traceFile

        def waitReady(self, ready, readyCount, startTimeout):
            if ready:
                self.expect("%s ready\n" % ready, timeout=startTimeout)
            else:
                while readyCount > 0:
                    self.expect("[^\n]+ ready\n", timeout=startTimeout)
                    readyCount -= 1

        def isTerminated(self):
            return self.p is None

        def teardown(self, current, success):
            if self.traceFile:
                if success or current.driver.isInterrupted():
                    os.remove(self.traceFile)
                else:
                    current.writeln("saved {0}".format(self.traceFile))

    def getHost(self, current):
        return current.driver.getHost(current.config.protocol, current.config.ipv6)

    def start(self, process, current, args, props, envs, watchDog):
        #
        # Props and arguments can use the format parameters set below in the kargs
        # dictionary. It's time to convert them to their values.
        #
        kargs = {
            "process": process,
            "testcase": current.testcase,
            "testdir": current.testsuite.getPath(),
            "builddir": current.getBuildDir(process.getExe(current)),
        }

        traceFile = ""
        if not isinstance(process.getMapping(current), JavaScriptMixin):
            traceProps = process.getEffectiveTraceProps(current)
            if traceProps:
                if "Ice.ProgramName" in props:
                    programName = props["Ice.ProgramName"]
                else:
                    programName = process.exe or current.testcase.getProcessType(
                        process
                    )
                traceFile = os.path.join(
                    current.testsuite.getPath(),
                    "{0}-{1}.log".format(programName, time.strftime("%m%d%y-%H%M")),
                )
                traceProps["Ice.LogFile"] = traceFile
            props.update(traceProps)

        args = ["--{0}={1}".format(k, val(v)) for k, v in props.items()] + [
            val(a) for a in args
        ]
        for k, v in envs.items():
            envs[k] = val(v, quoteValue=False)

        cmd = ""
        if current.driver.valgrind:
            cmd += 'valgrind -q --child-silent-after-fork=yes --leak-check=full --suppressions="{0}" '.format(
                os.path.join(
                    current.driver.getComponent().getSourceDir(),
                    "config",
                    "valgrind.sup",
                )
            )
        exe = process.getCommandLine(current, " ".join(args))
        cmd += exe.format(**kargs)

        if current.driver.debug:
            if len(envs) > 0:
                current.writeln("({0} env={1})".format(cmd, envs))
            else:
                current.writeln("({0})".format(cmd))

        env = os.environ.copy()
        env.update(envs)
        mapping = process.getMapping(current)
        cwd = mapping.getTestCwd(process, current)
        process = LocalProcessController.LocalProcess(
            command=cmd,
            startReader=False,
            env=env,
            cwd=cwd,
            desc=process.desc or exe,
            preexec_fn=process.preexec_fn,
            mapping=str(mapping),
            traceFile=traceFile,
        )
        process.startReader(watchDog)
        return process


class RemoteProcessController(ProcessController):
    class RemoteProcess:
        def __init__(self, exe, proxy):
            self.exe = exe
            self.proxy = proxy
            self.terminated = False
            self.stdout = False
            self.output = ""

        def __str__(self):
            return "{0} proxy={1}".format(self.exe, self.proxy)

        def waitReady(self, ready, readyCount, startTimeout):
            self.proxy.waitReady(startTimeout)

        def waitSuccess(self, exitstatus=0, timeout=60):
            import Ice

            try:
                result = self.proxy.waitSuccess(timeout)
            except Ice.UserException as ex:
                if "timed out" in ex.reason:
                    raise Expect.TIMEOUT("waitSuccess timeout")
                else:
                    raise
            except Ice.LocalException:
                raise

            if exitstatus != result:
                raise RuntimeError(
                    "unexpected exit status: expected: %d, got %d\n"
                    % (exitstatus, result)
                )

        def getOutput(self):
            return self.output

        def trace(self, outfilters):
            self.stdout = True

        def isTerminated(self):
            return self.terminated

        def terminate(self):
            self.output = self.proxy.terminate().strip()
            self.terminated = True
            if self.stdout and self.output:
                print(self.output)

        def teardown(self, current, success):
            pass

    def __init__(self, current, endpoints="tcp"):
        self.processControllerProxies = {}
        self.controllerApps = []
        self.driver = current.driver
        self.cond = threading.Condition()

        comm = current.driver.getCommunicator()
        import Test

        class ProcessControllerRegistryI(Test.Common.ProcessControllerRegistry):
            def __init__(self, remoteProcessController):
                self.remoteProcessController = remoteProcessController

            def setProcessController(self, proxy, current):
                import Test

                proxy = Test.Common.ProcessControllerPrx.uncheckedCast(
                    current.con.createProxy(proxy.ice_getIdentity())
                )
                self.remoteProcessController.setProcessController(proxy)

        import Ice

        comm.getProperties().setProperty("Adapter.AdapterId", str(uuid.uuid4()))
        self.adapter = comm.createObjectAdapterWithEndpoints("Adapter", endpoints)
        self.adapter.add(
            ProcessControllerRegistryI(self),
            Ice.stringToIdentity("Util/ProcessControllerRegistry"),
        )
        self.adapter.activate()

    def __str__(self):
        return "remote controller"

    def getHost(self, current):
        return self.getController(current).getHost(
            current.config.protocol, current.config.ipv6
        )

    def getController(self, current):
        import Ice
        import Test

        ident = self.getControllerIdentity(current)
        if isinstance(ident, str):
            ident = Ice.stringToIdentity(ident)

        proxy = None
        with self.cond:
            if ident in self.processControllerProxies:
                proxy = self.processControllerProxies[ident]
        if proxy:
            try:
                proxy.ice_ping()
                return proxy
            except Ice.NoEndpointException:
                self.clearProcessController(proxy)

        comm = current.driver.getCommunicator()

        if current.driver.controllerApp:
            if ident in self.controllerApps:
                self.restartControllerApp(
                    current, ident
                )  # Controller must have crashed, restart it
            else:
                self.controllerApps.append(ident)
                self.startControllerApp(current, ident)

        # Use well-known proxy and IceDiscovery to discover the process controller object from the app.
        proxy = Test.Common.ProcessControllerPrx.uncheckedCast(
            comm.stringToProxy(comm.identityToString(ident))
        )

        #
        # First try to discover the process controller with IceDiscovery, if this doesn't
        # work we'll wait for 10s for the process controller to register with the registry.
        # If the wait times out, we retry again.
        #
        def callback(future):
            try:
                future.result()
                with self.cond:
                    if ident not in self.processControllerProxies:
                        self.processControllerProxies[proxy.ice_getIdentity()] = proxy
                    self.cond.notifyAll()
            except Exception:
                pass

        nRetry = 0
        while nRetry < 120:
            nRetry += 1

            if self.supportsDiscovery():
                proxy.ice_pingAsync().add_done_callback(callback)

            with self.cond:
                if ident not in self.processControllerProxies:
                    self.cond.wait(5)
                if ident in self.processControllerProxies:
                    return self.processControllerProxies[ident]

            # If the controller isn't up after a while, we restart it. With the iOS simulator,
            # it's not uncommon to get Springboard crashes when starting the controller.
            if nRetry == 50:
                sys.stdout.write("controller application unreachable, restarting... ")
                sys.stdout.flush()
                self.restartControllerApp(current, ident)
                print("ok")

        raise RuntimeError("couldn't reach the remote controller `{0}'".format(ident))

    def setProcessController(self, proxy):
        with self.cond:
            self.processControllerProxies[proxy.ice_getIdentity()] = proxy
            conn = proxy.ice_getConnection()
            if hasattr(conn, "setCloseCallback"):
                proxy.ice_getConnection().setCloseCallback(
                    lambda conn: self.clearProcessController(proxy, conn)
                )
            else:
                import Ice

                class CallbackI(Ice.ConnectionCallback):
                    def __init__(self, registry):
                        self.registry = registry

                    def heartbeath(self, conn):
                        pass

                    def closed(self, conn):
                        self.registry.clearProcessController(proxy, conn)

                proxy.ice_getConnection().setCallback(CallbackI(self))

            self.cond.notifyAll()

    def supportsDiscovery(self):
        return True

    def clearProcessController(self, proxy, conn=None):
        with self.cond:
            if proxy.ice_getIdentity() in self.processControllerProxies:
                if not conn:
                    conn = proxy.ice_getCachedConnection()
                if (
                    conn
                    == self.processControllerProxies[
                        proxy.ice_getIdentity()
                    ].ice_getCachedConnection()
                ):
                    del self.processControllerProxies[proxy.ice_getIdentity()]

    def startControllerApp(self, current, ident):
        pass

    def restartControllerApp(self, current, ident):
        self.stopControllerApp(ident)
        self.startControllerApp(current, ident)

    def stopControllerApp(self, ident):
        pass

    def start(self, process, current, args, props, envs, watchDog):
        # Get the process controller
        processController = self.getController(current)

        # TODO: support envs?

        exe = process.getExe(current)
        args = [
            "--{0}={1}".format(k, val(v, quoteValue=False)) for k, v in props.items()
        ] + [val(a) for a in args]
        if current.driver.debug:
            current.writeln(
                "(executing `{0}/{1}' on `{2}' args = {3})".format(
                    current.testsuite, exe, self, args
                )
            )
        prx = processController.start(str(current.testsuite), exe, args)

        # Create bi-dir proxy in case we're talking to a bi-bir process controller.
        if self.adapter:
            prx = processController.ice_getConnection().createProxy(
                prx.ice_getIdentity()
            )
        import Test

        return RemoteProcessController.RemoteProcess(
            exe, Test.Common.ProcessPrx.uncheckedCast(prx)
        )

    def destroy(self, driver):
        if driver.controllerApp:
            for ident in self.controllerApps:
                self.stopControllerApp(ident)
            self.controllerApps = []
        if self.adapter:
            self.adapter.destroy()


class AndroidProcessController(RemoteProcessController):
    def __init__(self, current):
        endpoint = None
        if current.config.device:
            endpoint = "tcp -h 0.0.0.0 -p 15001"
        elif current.config.avd or not current.config.device:
            endpoint = "tcp -h 127.0.0.1 -p 15001"
        RemoteProcessController.__init__(self, current, endpoint)
        self.device = current.config.device
        self.avd = current.config.avd
        self.emulator = None  # Keep a reference to the android emulator process

    def __str__(self):
        return "Android"

    def supportsDiscovery(self):
        return self.device is not None  # Discovery is only used with devices

    def getControllerIdentity(self, current):
        return "Android/ProcessController"

    def adb(self):
        return "adb -d" if self.device == "usb" else "adb"

    def startEmulator(self, avd):
        #
        # First check if the AVD image is available
        #
        print("starting the emulator... ")
        out = run("emulator -list-avds")

        if avd not in out:
            raise RuntimeError("couldn't find AVD `{}'".format(avd))

        #
        # Find and unused port to run android emulator, between 5554 and 5584
        #
        port = -1
        out = run("adb devices -l")
        for p in range(5554, 5586, 2):
            if "emulator-{}".format(p) not in out:
                port = p

        if port == -1:
            raise RuntimeError(
                "cannot find free port in range 5554-5584, to run android emulator"
            )

        self.device = "emulator-{}".format(port)
        cmd = "emulator -avd {0} -port {1} -no-audio -partition-size 768 -no-snapshot -gpu auto -accel on -no-boot-anim -no-window".format(
            avd, port
        )
        self.emulator = subprocess.Popen(cmd, shell=True)

        if self.emulator.poll():
            raise RuntimeError(
                "failed to start the Android emulator `{}' on port {}".format(avd, port)
            )

        self.avd = avd

        #
        # Wait for the device to be ready
        #
        t = time.time()
        while True:
            try:
                lines = run("{} shell getprop sys.boot_completed".format(self.adb()))
                if len(lines) > 0 and lines[0].strip() == "1":
                    break
            except RuntimeError:
                pass  # expected if device is offline
            #
            # If the emulator doesn't complete boot in 300 seconds give up
            #
            if (time.time() - t) > 300:
                raise RuntimeError(
                    "couldn't start the Android emulator `{}'".format(avd)
                )
            time.sleep(2)

    def startControllerApp(self, current, ident):
        # Stop previous controller app before starting new one
        for ident in self.controllerApps:
            self.stopControllerApp(ident)

        if current.config.avd:
            self.startEmulator(current.config.avd)
        elif not current.config.device:
            # Create Android Virtual Device
            sdk = current.testcase.getMapping().getSDKPackage()
            print("creating virtual device ({0})... ".format(sdk))
            try:
                run("avdmanager -v delete avd -n IceTests")  # Delete the created device
            except Exception:
                pass
            # The SDK is downloaded by test VMs instead of here.
            # run("sdkmanager \"{0}\"".format(sdk), stdout=True, stdin="yes", stdinRepeat=True) # yes to accept licenses
            run(
                'avdmanager -v create avd -k "{0}" -d "Nexus 6" -n IceTests'.format(sdk)
            )
            self.startEmulator("IceTests")
        elif current.config.device != "usb":
            run("adb connect {}".format(current.config.device))

        # First try uninstall in case the controller was left behind from a previous run
        try:
            run("{} shell pm uninstall com.zeroc.testcontroller".format(self.adb()))
        except Exception:
            pass
        run(
            "{} install -t -r {}".format(
                self.adb(), current.testcase.getMapping().getApk(current)
            )
        )
        run(
            '{} shell am start -n "{}" -a android.intent.action.MAIN -c android.intent.category.LAUNCHER'.format(
                self.adb(), current.testcase.getMapping().getActivityName()
            )
        )

    def stopControllerApp(self, ident):
        try:
            run("{} logcat -b crash".format(self.adb()))
        except Exception:
            pass

        try:
            run("{} shell pm uninstall com.zeroc.testcontroller".format(self.adb()))
        except Exception:
            pass

        if self.avd:
            try:
                run("{} emu kill".format(self.adb()))
            except Exception:
                pass

            if self.avd == "IceTests":
                try:
                    run(
                        "avdmanager -v delete avd -n IceTests"
                    )  # Delete the created device
                except Exception:
                    pass

        #
        # Wait for the emulator to shutdown
        #
        if self.emulator:
            sys.stdout.write("Waiting for the emulator to shutdown..")
            sys.stdout.flush()
            while True:
                if self.emulator.poll() is not None:
                    print(" ok")
                    break
                sys.stdout.write(".")
                sys.stdout.flush()
                time.sleep(0.5)

        try:
            run("adb kill-server")
        except Exception:
            pass


class iOSSimulatorProcessController(RemoteProcessController):
    device = "iOSSimulatorProcessController"
    deviceID = "com.apple.CoreSimulator.SimDeviceType.iPhone-15"

    def __init__(self, current):
        RemoteProcessController.__init__(self, current, None)
        self.simulatorID = None
        self.runtimeID = None
        # Pick the last iOS simulator runtime ID in the list of iOS simulators (assumed to be the latest).
        try:
            for r in run("xcrun simctl list runtimes").split("\n"):
                m = re.search(r"iOS .* \(.*\) - (.*)", r)
                if m:
                    self.runtimeID = m.group(1)
        except Exception:
            pass

        if not self.runtimeID:
            self.runtimeID = (
                "com.apple.CoreSimulator.SimRuntime.iOS-17-5"  # Default value
            )

    def __str__(self):
        return "iOS Simulator ({})".format(
            self.runtimeID.replace("com.apple.CoreSimulator.SimRuntime.", "").strip()
        )

    def getControllerIdentity(self, current):
        return current.testcase.getMapping().getIOSControllerIdentity(current)

    def startControllerApp(self, current, ident):
        mapping = current.testcase.getMapping()
        appFullPath = mapping.getIOSAppFullPath(current)

        sys.stdout.write("launching simulator... ")
        sys.stdout.flush()
        try:
            run('xcrun simctl boot "{0}"'.format(self.device))
            run(
                'xcrun simctl bootstatus "{0}"'.format(self.device)
            )  # Wait for the boot to complete
        except Exception as ex:
            if str(ex).find("Booted") >= 0:
                pass
            elif str(ex).find("Invalid device") >= 0 or str(ex).find(
                "Assertion failure in SimDevicePair"
            ):
                #
                # Create the simulator device if it doesn't exist
                #
                self.simulatorID = run(
                    'xcrun simctl create "{0}" {1} {2}'.format(
                        self.device, self.deviceID, self.runtimeID
                    )
                )
                run('xcrun simctl boot "{0}"'.format(self.device))
                run(
                    'xcrun simctl bootstatus "{0}"'.format(self.device)
                )  # Wait for the boot to complete
                #
                # This not longer works on iOS 15 simulator, fails with:
                #   "Could not write domain com.apple.springboard; exiting"
                #
                # We update the watchdog timer scale to prevent issues with the controller app taking too long
                # to start on the simulator. The security validation of the app can take a significant time and
                # causes the watch dog to kick-in leaving the springboard app in a bogus state where it's not
                # possible to terminate and restart the controller
                #
                # run("xcrun simctl spawn \"{0}\" defaults write com.apple.springboard FBLaunchWatchdogScale 20".format(self.device))
                # run("xcrun simctl shutdown \"{0}\"".format(self.device))
                # run("xcrun simctl boot \"{0}\"".format(self.device))
            else:
                raise
        print("ok")

        sys.stdout.write("launching {0}... ".format(os.path.basename(appFullPath)))
        sys.stdout.flush()

        if not os.path.exists(appFullPath):
            raise RuntimeError(
                "couldn't find iOS simulator controller application, did you build it?"
            )
        run('xcrun simctl install "{0}" "{1}"'.format(self.device, appFullPath))
        run('xcrun simctl launch "{0}" {1}'.format(self.device, ident.name))
        print("ok")

    def restartControllerApp(self, current, ident):
        # We reboot the simulator if the controller fails to start. Terminating the controller app
        # with simctl terminate doesn't always work, it can hang if the controller app died because
        # of the springboard watchdog.
        run('xcrun simctl shutdown "{0}"'.format(self.device))
        nRetry = 0
        while nRetry < 20:
            try:
                run('xcrun simctl boot "{0}"'.format(self.device))
                break
            except Exception:
                time.sleep(1.0)
                nRetry += 1
        run('xcrun simctl launch "{0}" {1}'.format(self.device, ident.name))

    def stopControllerApp(self, ident):
        try:
            run('xcrun simctl uninstall "{0}" {1}'.format(self.device, ident.name))
        except Exception:
            pass

    def destroy(self, driver):
        RemoteProcessController.destroy(self, driver)

        sys.stdout.write("shutting down simulator... ")
        sys.stdout.flush()
        try:
            run('xcrun simctl shutdown "{0}"'.format(self.simulatorID))
        except Exception:
            pass
        print("ok")

        if self.simulatorID:
            sys.stdout.write("destroying simulator... ")
            sys.stdout.flush()
            try:
                run('xcrun simctl delete "{0}"'.format(self.simulatorID))
            except Exception:
                pass
            print("ok")


class iOSDeviceProcessController(RemoteProcessController):
    appPath = "cpp/test/ios/controller/build"

    def __init__(self, current):
        RemoteProcessController.__init__(self, current, None)

    def __str__(self):
        return "iOS Device"

    def getControllerIdentity(self, current):
        return current.testcase.getMapping().getIOSControllerIdentity(current)

    def startControllerApp(self, current, ident):
        # TODO: use ios-deploy to deploy and run the application on an attached device?
        pass

    def stopControllerApp(self, ident):
        pass


class BrowserProcessController(RemoteProcessController):
    def __init__(self, current):
        self.host = current.driver.host or "127.0.0.1"
        RemoteProcessController.__init__(
            self, current, "ws -h {0} -p 15002:wss -h {0} -p 15003".format(self.host)
        )
        self.httpServer = None
        self.httpsServer = None
        self.url = None
        self.driver = None
        try:
            httpServerCmd = "node node_modules/http-server/bin/http-server -p 8080 dist"
            cwd = current.testcase.getMapping().getPath()
            self.httpServer = Expect.Expect(httpServerCmd, cwd=cwd)
            self.httpServer.expect("Available on:")

            httpsServerCmd = "node node_modules/http-server/bin/http-server -p 9090 --tls --cert ../certs/server.pem --key ../certs/server_key.pem dist"
            cwd = current.testcase.getMapping().getPath()
            self.httpsServer = Expect.Expect(httpsServerCmd, cwd=cwd)
            self.httpsServer.expect("Available on:")

            if current.config.browser.startswith("Remote:"):
                from selenium import webdriver
                from selenium.webdriver.common.desired_capabilities import (
                    DesiredCapabilities,
                )

                (driver, capabilities, port) = current.config.browser.split(":")
                self.driver = webdriver.Remote(
                    "http://localhost:{0}".format(port),
                    desired_capabilities=getattr(DesiredCapabilities, capabilities),
                    keep_alive=True,
                )
            elif current.config.browser != "Manual":
                from selenium import webdriver

                if current.config.browser.find(":") > 0:
                    (driver, port) = current.config.browser.split(":")
                else:
                    (driver, port) = (current.config.browser, 0)

                if not hasattr(webdriver, driver):
                    raise RuntimeError("unknown browser `{0}'".format(driver))

                if driver == "Firefox":
                    if (
                        isinstance(platform, Linux)
                        and os.environ.get("DISPLAY", "") != ":1"
                        and os.environ.get("USER", "") == "ubuntu"
                    ):
                        current.writeln("error: DISPLAY is unset, setting it to :1")
                        os.environ["DISPLAY"] = ":1"

                    #
                    # We need to specify a profile for Firefox. This profile only provides the cert8.db which
                    # contains our Test CA cert. It should be possible to avoid this by setting the webdriver
                    # acceptInsecureCerts capability but it's only supported by latest Firefox releases.
                    #
                    profilepath = os.path.join(
                        current.driver.getComponent().getSourceDir(),
                        "scripts",
                        "selenium",
                        "firefox",
                    )
                    options = webdriver.FirefoxOptions()
                    options.set_preference("profile", profilepath)
                    self.driver = webdriver.Firefox(options=options)
                elif driver == "Safari" and int(port) > 0:
                    service = webdriver.SafariService(port=port, reuse_service=True)
                    self.driver = webdriver.Safari(service=service)
                else:
                    self.driver = getattr(webdriver, driver)()
        except Exception:
            self.destroy(current.driver)
            raise

    def __str__(self):
        return str(self.driver) if self.driver else "Manual"

    def supportsDiscovery(self):
        return False

    def getControllerIdentity(self, current):
        import Ice

        #
        # Load the controller page each time we're asked for the controller and if we're running
        # another testcase, the controller page will connect to the process controller registry
        # to register itself with this script.
        #
        testsuite = str(current.testsuite)

        if current.config.protocol == "wss":
            protocol = "https"
            port = "9090"
            cport = "15003"
        else:
            protocol = "http"
            port = "8080"
            cport = "15002"
        url = "{0}://{5}:{1}/test/{2}/controller.html?port={3}&worker={4}".format(
            protocol, port, testsuite, cport, current.config.worker, self.host
        )
        if url != self.url:
            self.url = url
            ident = Ice.stringToIdentity("Browser/ProcessController")
            if self.driver:
                # Clear the previous controller connection if it exists. This ensures that the reload
                # of the test controller will use a new connection (with Chrome the connection close
                # callback for the old page is sometime called after the new paged is loaded).
                with self.cond:
                    if ident in self.processControllerProxies:
                        prx = self.processControllerProxies[ident]
                        self.clearProcessController(prx, prx.ice_getCachedConnection())
                self.driver.get(url)
            else:
                # If no process controller is registered, we request the user to load the controller
                # page in the browser. Once loaded, the controller will register and we'll redirect to
                # the correct testsuite page.
                prx = None
                with self.cond:
                    while True:
                        if ident in self.processControllerProxies:
                            prx = self.processControllerProxies[ident]
                            break
                        print("Please load {}".format(url))
                        self.cond.wait(5)

                try:
                    import Test

                    Test.Common.BrowserProcessControllerPrx.uncheckedCast(prx).redirect(
                        url
                    )
                except Exception:
                    pass
                finally:
                    self.clearProcessController(prx, prx.ice_getCachedConnection())

        return "Browser/ProcessController"

    def getController(self, current):
        try:
            return RemoteProcessController.getController(self, current)
        except RuntimeError as ex:
            if self.driver:
                # Print out the client & server console element values
                for element in ["clientConsole", "serverConsole"]:
                    try:
                        console = self.driver.find_element_by_id(element).get_attribute(
                            "value"
                        )
                        if len(console) > 0:
                            print("controller {0} value:\n{1}".format(element, console))
                    except Exception as exc:
                        print(
                            "couldn't get controller {0} value:\n{1}".format(
                                element, exc
                            )
                        )
                        pass
                # Print out the browser log
                try:
                    print("browser log:\n{0}".format(self.driver.get_log("browser")))
                except Exception:
                    pass  # Not all browsers support retrieving the browser console log
            raise ex

    def destroy(self, driver):
        RemoteProcessController.destroy(self, driver)
        if self.httpServer:
            self.httpServer.terminate()
            self.httpServer = None

        if self.httpsServer:
            self.httpsServer.terminate()
            self.httpsServer = None

        try:
            self.driver.quit()
        except Exception:
            pass


class Driver:
    class Current:
        def __init__(self, driver, testsuite, result):
            self.driver = driver
            self.testsuite = testsuite
            self.config = driver.configs[testsuite.getMapping()]
            self.desc = ""
            self.result = result
            self.host = None
            self.testcase = None
            self.testcases = []
            self.processes = {}
            self.dirs = []
            self.files = []

        def getTestEndpoint(self, *args, **kargs):
            return self.driver.getTestEndpoint(*args, **kargs)

        def getBuildDir(self, name):
            return self.testcase.getMapping().getBuildDir(name, self)

        def getPluginEntryPoint(self, plugin, process):
            return self.testcase.getMapping().getPluginEntryPoint(plugin, process, self)

        def write(self, *args, **kargs):
            self.result.write(*args, **kargs)

        def writeln(self, *args, **kargs):
            self.result.writeln(*args, **kargs)

        def push(self, testcase, host=None):
            if not testcase.mapping:
                assert not testcase.parent and not testcase.testsuite
                testcase.mapping = self.testcase.getMapping()
                testcase.testsuite = self.testcase.getTestSuite()
                testcase.parent = self.testcase
            self.testcases.append((self.testcase, self.config, self.host))
            self.testcase = testcase
            self.config = self.driver.configs[
                self.testcase.getMapping()
            ].cloneAndOverrideWith(self)
            self.host = host

        def pop(self):
            assert self.testcase
            testcase = self.testcase
            (self.testcase, self.config, self.host) = self.testcases.pop()
            if testcase.parent and self.testcase != testcase:
                testcase.mapping = None
                testcase.testsuite = None
                testcase.parent = None

        def createFile(self, path, lines, encoding=None):
            path = os.path.join(self.testsuite.getPath(), path)
            with (
                open(path, "w", encoding=encoding)
                if encoding
                else open(path, "w") as file
            ):
                for line in lines:
                    file.write("%s\n" % line)
            self.files.append(path)

        def mkdirs(self, dirs):
            for d in dirs if isinstance(dirs, list) else [dirs]:
                d = os.path.join(self.testsuite.getPath(), d)
                self.dirs.append(d)
                if not os.path.exists(d):
                    os.makedirs(d)

        def destroy(self):
            for d in self.dirs:
                if os.path.exists(d):
                    shutil.rmtree(d)
            for f in self.files:
                if os.path.exists(f):
                    os.unlink(f)

    drivers = {}
    driver = "local"

    @classmethod
    def add(self, name, driver, default=False):
        if default:
            Driver.driver = name
        self.driver = name
        self.drivers[name] = driver

    @classmethod
    def getAll(self):
        return list(self.drivers.values())

    @classmethod
    def create(self, options, component):
        parseOptions(self, options)
        driver = self.drivers.get(self.driver)
        if not driver:
            raise RuntimeError("unknown driver `{0}'".format(self.driver))
        return driver(options, component)

    @classmethod
    def getSupportedArgs(self):
        return (
            "dlrR",
            [
                "debug",
                "driver=",
                "filter=",
                "rfilter=",
                "host=",
                "host-ipv6=",
                "host-bt=",
                "interface=",
                "controller-app",
                "valgrind",
                "languages=",
                "rlanguages=",
            ],
        )

    @classmethod
    def usage(self):
        pass

    @classmethod
    def commonUsage(self):
        print("")
        print("Driver options:")
        print("-d | --debug          Verbose information.")
        print(
            "--driver=<driver>     Use the given driver (local, client, server or remote)."
        )
        print("--filter=<regex>      Run all the tests that match the given regex.")
        print(
            "--rfilter=<regex>     Run all the tests that do not match the given regex."
        )
        print(
            "--languages=l1,l2,... List of comma-separated language mappings to test."
        )
        print(
            "--rlanguages=l1,l2,.. List of comma-separated language mappings to not test."
        )
        print("--host=<addr>         The IPv4 address to use for Ice.Default.Host.")
        print("--host-ipv6=<addr>    The IPv6 address to use for Ice.Default.Host.")
        print(
            "--host-bt=<addr>      The Bluetooth address to use for Ice.Default.Host."
        )
        print(
            "--interface=<IP>      The multicast interface to use to discover controllers."
        )
        print("--controller-app      Start the process controller application.")
        print("--valgrind            Start executables with valgrind.")

    def __init__(self, options, component):
        self.component = component
        self.debug = False
        self.filters = []
        self.rfilters = []
        self.host = ""
        self.hostIPv6 = ""
        self.hostBT = ""
        self.controllerApp = False
        self.valgrind = False
        self.languages = ",".join(os.environ.get("LANGUAGES", "").split(" "))
        self.languages = [self.languages] if self.languages else []
        self.rlanguages = []
        self.failures = []

        parseOptions(
            self,
            options,
            {
                "d": "debug",
                "r": "filters",
                "R": "rfilters",
                "filter": "filters",
                "rfilter": "rfilters",
                "host-ipv6": "hostIPv6",
                "host-bt": "hostBT",
                "controller-app": "controllerApp",
            },
        )

        if self.languages:
            self.languages = [
                i
                for sublist in [lang.split(",") for lang in self.languages]
                for i in sublist
            ]
        if self.rlanguages:
            self.rlanguages = [
                i
                for sublist in [lang.split(",") for lang in self.rlanguages]
                for i in sublist
            ]

        (self.filters, self.rfilters) = (
            [re.compile(a) for a in self.filters],
            [re.compile(a) for a in self.rfilters],
        )

        self.communicator = None
        self.interface = ""
        self.processControllers = {}

    def setConfigs(self, configs):
        self.configs = configs

    def getFilters(self, mapping, config):
        # Return the driver and component filters
        (filters, rfilters) = self.component.getFilters(mapping, config)
        (filters, rfilters) = (
            [re.compile(a) for a in filters],
            [re.compile(a) for a in rfilters],
        )
        return (self.filters + filters, self.rfilters + rfilters)

    def getHost(self, protocol, ipv6):
        if protocol == "bt":
            if not self.hostBT:
                raise RuntimeError("no Bluetooth address set with --host-bt")
            return self.hostBT
        elif ipv6:
            return self.hostIPv6 or "::1"
        else:
            return self.host or "127.0.0.1"

    def getComponent(self):
        return self.component

    def isWorkerThread(self):
        return False

    def getTestEndpoint(self, portnum, protocol="default"):
        return "{0} -p {1}".format(protocol, self.getTestPort(portnum))

    def getTestPort(self, portnum):
        return 12010 + portnum

    def getArgs(self, process, current):
        # Return driver specific arguments
        return []

    def getProps(self, process, current):
        props = {}
        if isinstance(process, IceProcess):
            if not self.host:
                props["Ice.Default.Host"] = (
                    "::1" if current.config.ipv6 else "127.0.0.1"
                )
            else:
                props["Ice.Default.Host"] = self.host
        return props

    def getMappings(self):
        # Return additional mappings to load required by the driver
        return []

    def matchLanguage(self, language):
        if self.languages and language not in self.languages:
            return False
        if self.rlanguages and language in self.rlanguages:
            return False
        return True

    def getCommunicator(self):
        self.initCommunicator()
        return self.communicator

    def initCommunicator(self):
        if self.communicator:
            return

        try:
            import Ice
        except ImportError:
            # Try to add the local Python build to the sys.path
            try:
                pythonMapping = Mapping.getByName("python")
                if pythonMapping:
                    for p in pythonMapping.getPythonDirs(
                        pythonMapping.getPath(), self.configs[pythonMapping]
                    ):
                        sys.path.append(p)
            except RuntimeError:
                print(
                    "couldn't find IcePy, running these tests require it to be installed or built"
                )

        import Ice

        Ice.loadSlice(
            os.path.join(self.component.getSourceDir(), "scripts", "Controller.ice")
        )

        initData = Ice.InitializationData()
        initData.properties = Ice.createProperties()

        # Load IceSSL, this is useful to talk with WSS for JavaScript
        initData.properties.setProperty("Ice.Plugin.IceSSL", "IceSSL:createIceSSL")
        initData.properties.setProperty(
            "IceSSL.DefaultDir", os.path.join(self.component.getSourceDir(), "certs")
        )
        initData.properties.setProperty("IceSSL.CertFile", "server.p12")
        initData.properties.setProperty("IceSSL.Password", "password")
        initData.properties.setProperty("IceSSL.Keychain", "test.keychain")
        initData.properties.setProperty("IceSSL.KeychainPassword", "password")
        initData.properties.setProperty("IceSSL.VerifyPeer", "0")

        initData.properties.setProperty(
            "Ice.Plugin.IceDiscovery", "IceDiscovery:createIceDiscovery"
        )
        initData.properties.setProperty("IceDiscovery.DomainId", "TestController")
        initData.properties.setProperty("IceDiscovery.Interface", self.interface)
        initData.properties.setProperty("Ice.Default.Host", self.interface)
        initData.properties.setProperty("Ice.ThreadPool.Server.Size", "10")
        # initData.properties.setProperty("Ice.Trace.Protocol", "1")
        # initData.properties.setProperty("Ice.Trace.Network", "2")
        # initData.properties.setProperty("Ice.StdErr", "allTests.log")
        self.communicator = Ice.initialize(initData)

    def getProcessController(self, current, process=None):
        processController = None
        if current.config.buildPlatform == "iphonesimulator":
            processController = iOSSimulatorProcessController
        elif current.config.buildPlatform == "iphoneos":
            processController = iOSDeviceProcessController
        elif (
            process
            and current.config.browser
            and isinstance(process.getMapping(current), JavaScriptMixin)
        ):
            processController = BrowserProcessController
        elif process and current.config.android:
            processController = AndroidProcessController
        else:
            processController = LocalProcessController

        if processController in self.processControllers:
            return self.processControllers[processController]

        # Instantiate the controller
        self.processControllers[processController] = processController(current)
        return self.processControllers[processController]

    def getProcessProps(self, current, ready, readyCount, mapping):
        props = {}
        if ready or readyCount > 0:
            if current.config.buildPlatform not in [
                "iphonesimulator",
                "iphoneos",
            ] and not isinstance(mapping, JavaScriptMapping):
                props["Ice.PrintAdapterReady"] = 1
        return props

    def destroy(self):
        for controller in self.processControllers.values():
            controller.destroy(self)

        if self.communicator:
            self.communicator.destroy()


class CppMapping(Mapping):
    class Config(Mapping.Config):
        @classmethod
        def getSupportedArgs(self):
            return ("", ["cpp-config=", "cpp-platform=", "cpp-path=", "openssl"])

        @classmethod
        def usage(self):
            print("")
            print("C++ Mapping options:")
            print(
                "--cpp-path=<path>         Path of alternate source tree for the C++ mapping."
            )
            print(
                "--cpp-config=<config>     C++ build configuration for native executables (overrides --config)."
            )
            print(
                "--cpp-platform=<platform> C++ build platform for native executables (overrides --platform)."
            )
            print(
                "--openssl                 Run SSL tests with OpenSSL instead of the default platform SSL engine."
            )

        def __init__(self, options=[]):
            Mapping.Config.__init__(self, options)

            if self.buildConfig == platform.getDefaultBuildConfig():
                if isinstance(platform, Windows):
                    self.buildConfig = "Release"
                else:
                    self.buildConfig = "shared"

            parseOptions(
                self,
                options,
                {
                    "cpp-config": "buildConfig",
                    "cpp-platform": "buildPlatform",
                    "cpp-path": "pathOverride",
                },
            )

            if self.pathOverride:
                self.pathOverride = os.path.abspath(self.pathOverride)

    def getProps(self, process, current):
        props = Mapping.getProps(self, process, current)
        if isinstance(process, IceProcess):
            props["Ice.PrintStackTraces"] = "1"
        return props

    def getSSLProps(self, process, current):
        props = Mapping.getSSLProps(self, process, current)
        server = isinstance(process, Server)

        props.update(
            {
                "IceSSL.CAs": "cacert.pem",
                "IceSSL.CertFile": "server.p12" if server else "client.p12",
            }
        )
        if isinstance(platform, Darwin):
            props.update(
                {
                    "IceSSL.KeychainPassword": "password",
                    "IceSSL.Keychain": "server.keychain"
                    if server
                    else "client.keychain",
                }
            )
        return props

    def getPluginEntryPoint(self, plugin, process, current):
        return {
            "IceSSL": "IceSSLOpenSSL:createIceSSLOpenSSL"
            if current.config.openssl
            else "IceSSL:createIceSSL",
            "IceBT": "IceBT:createIceBT",
            "IceDiscovery": "IceDiscovery:createIceDiscovery",
            "IceLocatorDiscovery": "IceLocatorDiscovery:createIceLocatorDiscovery",
        }[plugin]

    def getEnv(self, process, current):
        #
        # On Windows, add the testcommon directories to the PATH
        #
        libPaths = []
        if isinstance(platform, Windows):
            testcommon = os.path.join(self.path, "test", "Common")
            if os.path.exists(testcommon):
                libPaths.append(
                    os.path.join(testcommon, self.getBuildDir("testcommon", current))
                )

        #
        # On most platforms, we also need to add the library directory to the library path environment variable.
        #
        if not isinstance(platform, Darwin):
            libPaths.append(self.component.getLibDir(process, self, current))

        #
        # Add the test suite library directories to the platform library path environment variable.
        #
        if current.testcase:
            for d in set(
                [
                    current.getBuildDir(d)
                    for d in current.testcase.getTestSuite().getLibDirs()
                ]
            ):
                libPaths.append(d)

        env = {}
        if len(libPaths) > 0:
            env[platform.getLdPathEnvName()] = os.pathsep.join(libPaths)
        return env

    def _getDefaultSource(self, processType):
        return {
            "client": "Client.cpp",
            "server": "Server.cpp",
            "serveramd": "ServerAMD.cpp",
            "collocated": "Collocated.cpp",
            "subscriber": "Subscriber.cpp",
            "publisher": "Publisher.cpp",
            "reader": "Reader.cpp",
            "writer": "Writer.cpp",
        }[processType]

    def _getDefaultExe(self, processType):
        return Mapping._getDefaultExe(self, processType).lower()

    def getIOSControllerIdentity(self, current):
        category = (
            "iPhoneSimulator"
            if current.config.buildPlatform == "iphonesimulator"
            else "iPhoneOS"
        )
        return "{0}/com.zeroc.Cpp-Test-Controller".format(category)

    def getIOSAppFullPath(self, current):
        cmd = "xcodebuild -project '{0}/cpp/test/ios/controller/C++ Test Controller.xcodeproj' \
                          -scheme 'C++ Test Controller' \
                          -configuration {1} \
                          -sdk {2} \
                          -arch arm64 \
                          -showBuildSettings \
                          ".format(
            toplevel,
            "Release" if os.environ.get("OPTIMIZE", "yes") != "no" else "Debug",
            current.config.buildPlatform,
        )
        targetBuildDir = re.search(r"\sTARGET_BUILD_DIR = (.*)", run(cmd)).groups(1)[0]
        testDriver = os.path.join(targetBuildDir, "C++ Test Controller.app")
        return testDriver


class JavaMapping(Mapping):
    class Config(Mapping.Config):
        @classmethod
        def getSupportedArgs(self):
            return ("", ["device=", "avd=", "android", "jacoco="])

        @classmethod
        def usage(self):
            print("")
            print("Java Mapping options:")
            print("--android                 Run the Android tests.")
            print(
                "--device=<device-id>      ID of the Android emulator or device used to run the tests."
            )
            print("--avd=<name>              Start specific Android Virtual Device.")
            print("--jacoco=<path>             Path to the JaCoCo agent.")

    def getCommandLine(self, current, process, exe, args):
        javaHome = os.getenv("JAVA_HOME", "")
        java = os.path.join(javaHome, "bin", "java") if javaHome else "java"
        javaArgs = self.getJavaArgs(process, current)
        if process.isFromBinDir():
            if javaArgs:
                return "{0} -ea {1} {2} {3}".format(java, " ".join(javaArgs), exe, args)
            else:
                return "{0} -ea {1} {2}".format(java, exe, args)

        testdir = self.component.getTestDir(self)
        assert current.testcase.getPath(current).startswith(testdir)
        package = "test." + current.testcase.getPath(current)[
            len(testdir) + 1 :
        ].replace(os.sep, ".")
        javaArgs = self.getJavaArgs(process, current)

        if current.config.jacoco:
            javaAgent = f"-javaagent:{current.config.jacoco}=destfile={package}.{exe}.jacoco.exec"
            java = f"{java} {javaAgent}"

        if javaArgs:
            return "{0} -ea {1} -Dtest.class={2}.{3} test.TestDriver {4}".format(
                java, " ".join(javaArgs), package, exe, args
            )
        else:
            return "{0} -ea -Dtest.class={1}.{2} test.TestDriver {3}".format(
                java, package, exe, args
            )

    def getJavaArgs(self, process, current):
        # TODO: WORKAROUND for https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=911925
        if isinstance(platform, Linux) and platform.getLinuxId() in [
            "debian",
            "ubuntu",
        ]:
            return ["-Djdk.net.URLClassPath.disableClassPathURLCheck=true"]
        return []

    def getSSLProps(self, process, current):
        props = Mapping.getSSLProps(self, process, current)
        if current.config.android:
            props.update(
                {
                    "IceSSL.KeystoreType": "BKS",
                    "IceSSL.TruststoreType": "BKS",
                    "IceSSL.Keystore": "server.bks"
                    if isinstance(process, Server)
                    else "client.bks",
                }
            )
        else:
            props.update(
                {
                    "IceSSL.Keystore": "server.jks"
                    if isinstance(process, Server)
                    else "client.jks",
                }
            )
        return props

    def getPluginEntryPoint(self, plugin, process, current):
        return {
            "IceSSL": "",
            "IceBT": "com.zeroc.IceBT.PluginFactory",
            "IceDiscovery": "com.zeroc.IceDiscovery.PluginFactory",
            "IceLocatorDiscovery": "com.zeroc.IceLocatorDiscovery.PluginFactory",
        }[plugin]

    def getEnv(self, process, current):
        return {"CLASSPATH": os.path.join(self.path, "lib", "test.jar")}

    def _getDefaultSource(self, processType):
        return {
            "client": "Client.java",
            "server": "Server.java",
            "serveramd": "AMDServer.java",
            "collocated": "Collocated.java",
        }[processType]

    def getSDKPackage(self):
        return "system-images;android-34;google_apis;{}".format(
            "arm64-v8a" if platform_machine() == "arm64" else "x86_64"
        )

    def getApk(self, current):
        return os.path.join(
            self.getPath(),
            "test",
            "android",
            "controller",
            "build",
            "outputs",
            "apk",
            "debug",
            "controller-debug.apk",
        )

    def getActivityName(self):
        return "com.zeroc.testcontroller/.ControllerActivity"


class CSharpMapping(Mapping):
    class Config(Mapping.Config):
        @classmethod
        def getSupportedArgs(self):
            return ("", ["coverage-session="])

        @classmethod
        def usage(self):
            print("")
            print("C# mapping options:")
            print(
                "--coverage-session=<id>      Run tests the dotnet-coverage using the given session ID."
            )

        def __init__(self, options=[]):
            Mapping.Config.__init__(self, options)
            self.dotnetCoverageSession = ""

            parseOptions(
                self,
                options,
                {
                    "coverage-session": "dotnetCoverageSession",
                },
            )

    def getTargetFramework(self, current):
        return "net8.0"

    def getBuildDir(self, name, current):
        return os.path.join("msbuild", name, self.getTargetFramework(current))

    def getSSLProps(self, process, current):
        props = Mapping.getSSLProps(self, process, current)
        props.update(
            {
                "IceSSL.Password": "password",
                "IceSSL.DefaultDir": os.path.join(
                    self.component.getSourceDir(), "certs"
                ),
                "IceSSL.CAs": "cacert.pem",
                "IceSSL.VerifyPeer": "0" if current.config.protocol == "wss" else "2",
                "IceSSL.CertFile": "server.p12"
                if isinstance(process, Server)
                else "client.p12",
            }
        )
        return props

    def getPluginEntryPoint(self, plugin, process, current):
        plugindir = self.component.getLibDir(process, self, current)

        #
        # If the plug-in assembly exists in the test directory, this is a good indication that the
        # test include a reference to the plug-in, in this case we must use the test dir as the
        # plug-in base directory to avoid loading two instances of the same assembly.
        #
        proccessType = current.testcase.getProcessType(process)
        if proccessType:
            testdir = os.path.join(
                current.testcase.getPath(current),
                self.getBuildDir(proccessType, current),
            )
            if os.path.isfile(os.path.join(testdir, plugin + ".dll")):
                plugindir = testdir
        plugindir += os.sep

        return {
            "IceSSL": "",
            "IceDiscovery": plugindir + "IceDiscovery.dll:IceDiscovery.PluginFactory",
            "IceLocatorDiscovery": plugindir
            + "IceLocatorDiscovery.dll:IceLocatorDiscovery.PluginFactory",
        }[plugin]

    def getEnv(self, process, current):
        env = {}
        if isinstance(platform, Windows):
            env["PATH"] = os.path.join(
                self.component.getSourceDir(),
                "cpp",
                "msbuild",
                "packages",
                "bzip2.{0}.1.0.6.10".format(platform.getPlatformToolset()),
                "build",
                "native",
                "bin",
                "x64",
                "Release",
            )
        return env

    def _getDefaultSource(self, processType):
        return {
            "client": "Client.cs",
            "server": "Server.cs",
            "serveramd": "ServerAMD.cs",
            "servertie": "ServerTie.cs",
            "serveramdtie": "ServerAMDTie.cs",
            "collocated": "Collocated.cs",
        }[processType]

    def _getDefaultExe(self, processType):
        return Mapping._getDefaultExe(self, processType).lower()

    def getCommandLine(self, current, process, exe, args):
        if process.isFromBinDir():
            path = self.component.getBinDir(process, self, current)
        else:
            path = os.path.join(
                current.testcase.getPath(current), current.getBuildDir(exe)
            )

        cmd = "dotnet {}.dll {}".format(os.path.join(path, exe), args)

        if current.config.dotnetCoverageSession != "":
            # escapign \" is requried for passing though from dotnet-coverage -> dotnet -> exe
            cmd = cmd.replace('\\"', '\\\\\\"')
            cmd = f"dotnet-coverage connect --nologo {current.config.dotnetCoverageSession} {cmd}"

        return cmd


class CppBasedMapping(Mapping):
    class Config(Mapping.Config):
        @classmethod
        def getSupportedArgs(self):
            return (
                "",
                [
                    self.mappingName + "-config=",
                    self.mappingName + "-platform=",
                    "openssl",
                ],
            )

        @classmethod
        def usage(self):
            print("")
            print(self.mappingDesc + " mapping options:")
            print(
                "--{0}-config=<config>     {1} build configuration for native executables (overrides --config).".format(
                    self.mappingName, self.mappingDesc
                )
            )
            print(
                "--{0}-platform=<platform> {1} build platform for native executables (overrides --platform).".format(
                    self.mappingName, self.mappingDesc
                )
            )
            print(
                "--openssl                 Run SSL tests with OpenSSL instead of the default platform SSL engine."
            )

        def __init__(self, options=[]):
            Mapping.Config.__init__(self, options)
            parseOptions(
                self,
                options,
                {
                    self.mappingName + "-config": "buildConfig",
                    self.mappingName + "-platform": "buildPlatform",
                },
            )

    def getSSLProps(self, process, current):
        return Mapping.getByName("cpp").getSSLProps(process, current)

    def getPluginEntryPoint(self, plugin, process, current):
        return Mapping.getByName("cpp").getPluginEntryPoint(plugin, process, current)

    def getEnv(self, process, current):
        env = Mapping.getEnv(self, process, current)
        if self.component.getInstallDir(self, current) != platform.getInstallDir():
            # If not installed in the default platform installation directory, add
            # the C++ library directory to the library path
            env[platform.getLdPathEnvName()] = self.component.getLibDir(
                process, Mapping.getByName("cpp"), current
            )
        return env


class PythonMapping(CppBasedMapping):
    class Config(CppBasedMapping.Config):
        mappingName = "python"
        mappingDesc = "Python"

        @classmethod
        def getSupportedArgs(self):
            return ("", ["python="])

        @classmethod
        def usage(self):
            print("")
            print("Python mapping options:")
            print(
                "--python=<interpreter>   Choose the interperter used to run python tests"
            )

        def __init__(self, options=[]):
            Mapping.Config.__init__(self, options)
            self.pythonVersion = None

        def getPythonVersion(self):
            if self.pythonVersion is None:
                version = subprocess.check_output(
                    [
                        currentConfig.python,
                        "-c",
                        'import sys; print("{0}.{1}".format(sys.version_info[0], sys.version_info[1]))',
                    ]
                )
                if not isinstance(version, str):
                    version = version.decode("utf-8")
                self.pythonVersion = tuple(int(num) for num in version.split("."))
            return self.pythonVersion

    def getCommandLine(self, current, process, exe, args):
        return '"{0}"  {1} {2} {3}'.format(
            current.config.python,
            os.path.join(self.path, "test", "TestHelper.py"),
            exe,
            args,
        )

    def getEnv(self, process, current):
        env = CppBasedMapping.getEnv(self, process, current)
        dirs = []
        if self.component.getInstallDir(self, current) != platform.getInstallDir():
            # If not installed in the default platform installation directory, add
            # the Ice python directory to PYTHONPATH
            dirs += self.getPythonDirs(
                self.component.getInstallDir(self, current), current.config
            )
        dirs += [current.testcase.getPath(current)]
        env["PYTHONPATH"] = os.pathsep.join(dirs)
        return env

    def getPythonDirs(self, iceDir, config):
        dirs = []
        if isinstance(platform, Windows):
            dirs.append(
                os.path.join(iceDir, "python", config.buildPlatform, config.buildConfig)
            )
        dirs.append(os.path.join(iceDir, "python"))
        return dirs

    def _getDefaultSource(self, processType):
        return {
            "client": "Client.py",
            "server": "Server.py",
            "serveramd": "ServerAMD.py",
            "collocated": "Collocated.py",
        }[processType]


class CppBasedClientMapping(CppBasedMapping):
    def loadTestSuites(self, tests, config, filters, rfilters):
        Mapping.loadTestSuites(self, tests, config, filters, rfilters)
        self.getServerMapping().loadTestSuites(self.testsuites.keys(), config)

    def getServerMapping(self, testId=None):
        return Mapping.getByName(
            "cpp"
        )  # By default, run clients against C++ mapping executables


class RubyMapping(CppBasedClientMapping):
    class Config(CppBasedClientMapping.Config):
        mappingName = "ruby"
        mappingDesc = "Ruby"

    def getCommandLine(self, current, process, exe, args):
        return "ruby  {0} {1} {2}".format(
            os.path.join(self.path, "test", "TestHelper.rb"), exe, args
        )

    def getEnv(self, process, current):
        env = CppBasedMapping.getEnv(self, process, current)
        dirs = []
        if self.component.getInstallDir(self, current) != platform.getInstallDir():
            # If not installed in the default platform installation directory, add
            # the Ice ruby directory to RUBYLIB
            dirs += [os.path.join(self.path, "ruby")]
        dirs += [current.testcase.getPath(current)]
        env["RUBYLIB"] = os.pathsep.join(dirs)
        return env

    def _getDefaultSource(self, processType):
        return {"client": "Client.rb"}[processType]


class PhpMapping(CppBasedClientMapping):
    class Config(CppBasedClientMapping.Config):
        mappingName = "php"
        mappingDesc = "PHP"

        def __init__(self, options=[]):
            CppBasedClientMapping.Config.__init__(self, options)

    def getCommandLine(self, current, process, exe, args):
        phpArgs = []
        php = "php"

        #
        # If Ice is not installed in the system directory, specify its location with PHP
        # configuration arguments.
        #
        if (
            platform.getInstallDir()
            and self.component.getInstallDir(self, current) != platform.getInstallDir()
        ):
            phpArgs += ["-n"]  # Do not load any php.ini files
            phpArgs += [
                "-d",
                "extension_dir='{0}'".format(
                    self.component.getLibDir(process, self, current)
                ),
            ]
            phpArgs += [
                "-d",
                "extension='{0}'".format(self.component.getPhpExtension(self, current)),
            ]
            phpArgs += [
                "-d",
                "include_path='{0}'".format(
                    self.component.getPhpIncludePath(self, current)
                ),
            ]

        if hasattr(process, "getPhpArgs"):
            phpArgs += process.getPhpArgs(current)

        return "{0} {1} -f {2} -- {3} {4}".format(
            php,
            " ".join(phpArgs),
            os.path.join(self.path, "test", "TestHelper.php"),
            exe,
            args,
        )

    def _getDefaultSource(self, processType):
        return {"client": "Client.php"}[processType]


class MatlabMapping(CppBasedClientMapping):
    class Config(CppBasedClientMapping.Config):
        mappingName = "matlab"
        mappingDesc = "MATLAB"

    def getCommandLine(self, current, process, exe, args):
        # The MATLAB_COMMAND environment variable can be used to specify the MATLAB command.
        # This is currently used for GitHub Actions to specify the path to a custom MATLAB executable
        # which is required to run the tests and does not need any additional arguments.
        matlabCmd = os.getenv("MATLAB_COMMAND")

        if not matlabCmd:
            matlabHome = os.getenv("MATLAB_HOME")
            # -wait and -minimize are not available on Linux, -log causes duplicate output to stdout on Linux
            matlabCmd = "{0} -nodesktop -nosplash {1} -r".format(
                "matlab"
                if matlabHome is None
                else os.path.join(matlabHome, "bin", "matlab"),
                " -wait -log -minimize" if isinstance(platform, Windows) else "",
            )

        return "{0} \"cd '{1}', runTest {2} {3} {4}\"".format(
            matlabCmd,
            os.path.abspath(
                os.path.join(os.path.dirname(__file__), "..", "matlab", "test", "lib")
            ),
            self.getTestCwd(process, current),
            current.driver.getComponent().getLibDir(process, self, current),
            args,
        )

    def getServerMapping(self, testId=None):
        return Mapping.getByName("python")  # Run clients against Python mapping servers

    def _getDefaultSource(self, processType):
        return {"client": "Client.m"}[processType]

    def getOptions(self, current):
        #
        # Metrics tests configuration not supported with MATLAB they use the Admin adapter.
        #
        options = CppBasedClientMapping.getOptions(self, current)
        options["mx"] = [False]
        return options


class JavaScriptMixin:
    def loadTestSuites(self, tests, config, filters, rfilters):
        # Exclude typescript directory when the mapping is not typescript otherwise we endup with duplicate entries
        if self.name != "typescript":
            rfilters += [re.compile("typescript/*")]

        Mapping.loadTestSuites(self, tests, config, filters, rfilters)
        self.getServerMapping().loadTestSuites(
            list(self.testsuites.keys()) + ["Ice/echo"], config
        )

    def getServerMapping(self, testId=None):
        if testId and self.hasSource(testId, "server"):
            return self
        else:
            return Mapping.getByName(
                "cpp"
            )  # Run clients against C++ mapping servers if no JS server provided

    def _getDefaultProcesses(self, processType):
        if processType.startswith("server"):
            return [EchoServer(), Server()]
        return Mapping._getDefaultProcesses(self, processType)

    def getCommonDir(self, current):
        return os.path.join(self.getPath(), "test", "Common")

    def getCommandLine(self, current, process, exe, args):
        return "node {0}/run.js file://{1} {2} {3}".format(
            self.getCommonDir(current),
            os.path.join(self.getTestCwd(process, current), exe),
            Path(exe).stem,
            args,
        )

    def getSSLProps(self, process, current):
        return {}

    def getOptions(self, current):
        options = {
            "protocol": ["ws", "wss"] if current.config.browser else ["tcp"],
            "compress": [False],
            "ipv6": [False],
            "serialize": [False],
            "mx": [False],
        }
        return options


class JavaScriptMapping(JavaScriptMixin, Mapping):
    class Config(Mapping.Config):
        @classmethod
        def getSupportedArgs(self):
            return ("", ["browser=", "worker"])

        @classmethod
        def usage(self):
            print("")
            print("JavaScript mapping options:")
            print("--browser=<name>      Run with the given browser.")
            print("--worker              Run with Web workers enabled.")

        def __init__(self, options=[]):
            Mapping.Config.__init__(self, options)

            if self.browser and self.protocol == "tcp":
                self.protocol = "ws"

    def getCommonDir(self, current):
        return os.path.join(self.getPath(), "test", "Common")

    def _getDefaultSource(self, processType):
        return {
            "client": "Client.js",
            "serveramd": "ServerAMD.js",
            "server": "Server.js",
        }[processType]

    def _getDefaultExe(self, processType):
        return self.getDefaultSource(processType)

    def getTestCwd(self, process, current):
        return os.path.join(self.path, "test", current.testcase.getTestSuite().getId())

    def getOptions(self, current):
        options = JavaScriptMixin.getOptions(self, current)
        options.update(
            {
                "worker": [False, True] if current.config.browser else [False],
            }
        )
        return options


class SwiftMapping(Mapping):
    class Config(CppBasedClientMapping.Config):
        mappingName = "swift"
        mappingDesc = "Swift"

        def __init__(self, options=[]):
            CppBasedClientMapping.Config.__init__(self, options)
            if self.buildConfig == platform.getDefaultBuildConfig():
                # Check the OPTIMIZE environment variable to figure out if it's debug/release build
                self.buildConfig = (
                    "release" if os.environ.get("OPTIMIZE", "yes") != "no" else "debug"
                )

    def getCommandLine(self, current, process, exe, args):
        testdir = self.component.getTestDir(self)
        assert current.testcase.getPath(current).startswith(testdir)
        package = current.testcase.getPath(current)[len(testdir) + 1 :].replace(
            os.sep, "_"
        )

        testDriver = "swift run -c {0} --skip-build TestDriver".format(
            current.config.buildConfig
        )

        return "{0} {1} {2} {3}".format(testDriver, package, exe, args)

    def _getDefaultSource(self, processType):
        return {
            "client": "Client.swift",
            "server": "Server.swift",
            "serveramd": "ServerAMD.swift",
            "collocated": "Collocated.swift",
        }[processType]

    def getIOSControllerIdentity(self, current):
        category = (
            "iPhoneSimulator"
            if current.config.buildPlatform == "iphonesimulator"
            else "iPhoneOS"
        )
        return "{0}/com.zeroc.Swift-Test-Controller".format(category)

    def getIOSAppFullPath(self, current):
        cmd = "xcodebuild -project {0} \
                          -scheme 'TestDriverApp' \
                          -configuration {1} \
                          -sdk {2} \
                          -arch arm64 \
                          -showBuildSettings \
                          ".format(
            self.getXcodeProject(current),
            current.config.buildConfig.capitalize(),  # SwiftPM uses lowercase. Xcode users uppercase.
            current.config.buildPlatform,
        )
        targetBuildDir = re.search(r"\sTARGET_BUILD_DIR = (.*)", run(cmd)).groups(1)[0]
        testDriver = os.path.join(targetBuildDir, "TestDriverApp.app")
        return testDriver

    def getSSLProps(self, process, current):
        props = Mapping.getByName("cpp").getSSLProps(process, current)
        props["IceSSL.DefaultDir"] = (
            "certs"
            if current.config.buildPlatform == "iphoneos"
            else os.path.join(self.component.getSourceDir(), "certs")
        )
        return props

    def getPluginEntryPoint(self, plugin, process, current):
        return Mapping.getByName("cpp").getPluginEntryPoint(plugin, process, current)

    def getXcodeProject(self, current):
        return "{0}/{1}".format(
            current.testcase.getMapping().getPath(), "test/ios/TestDriverApp.xcodeproj"
        )


#
# Instantiate platform global variable
#
platform = None
if sys.platform == "darwin":
    platform = Darwin()
elif sys.platform.startswith("linux") or sys.platform.startswith("gnukfreebsd"):
    platform = Linux()
elif sys.platform == "win32" or sys.platform[:6] == "cygwin":
    platform = Windows()
if not platform:
    print("can't run on unknown platform `{0}'".format(sys.platform))
    sys.exit(1)

#
# Import component classes and instantiate the default component
#
from Component import component  # noqa: F401, E402

#
# Initialize the platform with component
#
platform.init(component)

#
# Import local driver (this adds the local driver to the list of supported drivers)
#
import LocalDriver  # noqa: F401, E402

Driver.add("local", LocalDriver.LocalDriver)


def runTestsWithPath(path):
    mappings = Mapping.getAllByPath(path)
    if not mappings:
        print(
            "couldn't find mapping for `{0}' (is this mapping supported on this platform?)".format(
                path
            )
        )
        sys.exit(0)
    runTests(mappings)


def runTests(mappings=None, drivers=None):
    if not mappings:
        mappings = Mapping.getAll()
    if not drivers:
        drivers = Driver.getAll()

    def usage():
        print("Usage: " + sys.argv[0] + " [options] [tests]")
        print("")
        print("Options:")
        print("-h | --help        Show this message")

        Driver.commonUsage()
        for driver in drivers:
            driver.usage()

        Mapping.Config.commonUsage()
        for mapping in mappings:
            mapping.Config.usage()

        print("")

    driver = None
    try:
        options = [Driver.getSupportedArgs(), Mapping.Config.getSupportedArgs()]
        options += [driver.getSupportedArgs() for driver in drivers]
        options += [
            mapping.Config.getSupportedArgs()
            for mapping in Mapping.getAll(includeDisabled=True)
        ]
        shortOptions = "h"
        longOptions = ["help"]
        for so, lo in options:
            shortOptions += so
            longOptions += lo
        opts, args = getopt.gnu_getopt(sys.argv[1:], shortOptions, longOptions)

        for o, a in opts:
            if o in ["-h", "--help"]:
                usage()
                sys.exit(0)

        #
        # Create the driver
        #
        driver = Driver.create(opts, component)

        #
        # Create the configurations for each mapping.
        #
        configs = {}
        for mapping in Mapping.getAll():
            if mapping not in configs:
                configs[mapping] = mapping.createConfig(opts[:])

        #
        # If the user specified --languages/rlanguages, only run matching mappings.
        #
        mappings = [m for m in mappings if driver.matchLanguage(str(m))]

        #
        # Provide the configurations to the driver and load the test suites for each mapping.
        #
        driver.setConfigs(configs)
        for mapping in mappings + driver.getMappings():
            (filters, rfilters) = driver.getFilters(mapping, configs[mapping])
            mapping.loadTestSuites(args, configs[mapping], filters, rfilters)

        #
        # Finally, run the test suites with the driver.
        #
        try:
            sys.exit(driver.run(mappings, args))
        except KeyboardInterrupt:
            pass
        finally:
            driver.destroy()

    except Exception:
        print(sys.argv[0] + ": unexpected exception raised:\n" + traceback.format_exc())
        sys.exit(1)
