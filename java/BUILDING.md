# Ice for Java build instructions

This page describes how to build and install Ice for Java from source. If
you prefer, you can also download a [binary distribution].

* [Build Requirements](#build-requirements)
  * [Operating Systems](#operating-systems)
  * [Slice to Java Compiler](#slice-to-java-compiler)
  * [JDK Version](#jdk-version)
  * [Gradle](#gradle)
  * [Bzip2 Compression](#bzip2-compression)
  * [JGoodies](#jgoodies)
  * [ProGuard](#proguard)
  * [Java Application Bundler](#java-application-bundler)
* [Building Ice for Java](#building-ice-for-java)
* [Installing Ice for Java](#installing-ice-for-java)
* [Running the Java Tests](#running-the-java-tests)
* [Building the Ice for Android Tests](#building-the-ice-for-android-tests)
* [IceGrid GUI Tool](#icegrid-gui-tool)

## Build Requirements

### Operating Systems

Ice for Java builds and runs properly on Windows, macOS, and any recent Linux
distribution. It is fully supported on the platforms listed on the [supported platforms]
page.

### Slice to Java Compiler

You need the Slice to Java compiler to build Ice for Java and also to use
Ice for Java. The Slice to Java compiler (`slice2java`) is a command-line tool
written in C++. You can build the Slice to Java compiler from source, or
alternatively you can install an Ice [binary distribution] that includes
this compiler.

### JDK Version

You need JDK 8, JDK 11, JDK 17, or JDK 21 to build Ice for Java.

Make sure that the `javac` and `java` commands are present in your PATH.

> The build produces bytecode in the Java 8 class file format ([major version] 52).

The IceGrid GUI tool's Metrics Graph feature requires JavaFX support. If you
build the source with a JVM that lacks JavaFX support, this feature will be
unavailable. Alternatively, building the source in an environment with
JavaFX produces an IceGrid GUI JAR file that can be used in JVMs with or without
JavaFX support, as the Metrics Graph feature is enabled dynamically.

### Gradle

Ice for Java uses the [Gradle] build system, and includes the Gradle wrapper
in the distribution. You cannot build the Ice for Java source distribution without
an Internet connection. Gradle will download all required packages automatically
from the Maven Central repository located at https://repo1.maven.org/maven2/

### Bzip2 Compression

Ice for Java supports protocol compression using the bzip2 classes included
with [Apache Commons Compress].

The Maven package ID for the commons-compress JAR file is as follows:

```gradle
groupId=org.apache.commons, version=1.20, artifactId=commons-compress
```

The demos and tests are automatically setup to enable protocol compression by
adding the commons-compress JAR to the manifest class path. For your own
applications you must add the commons-compress JAR to the application `CLASSPATH`
to enable protocol compression.

### JGoodies

The IceGrid GUI tool uses the JGoodies libraries Forms and Looks. The following
versions were tested:

* JGoodies Forms 1.9.0
* JGoodies Looks 2.7.0

The Maven package ids for the JGoodies packages are as follows:

```gradle
groupId=com.jgoodies, version=1.9.0, artifactId=jgoodies-forms
groupId=com.jgoodies, version=2.7.0, artifactId=jgoodies-looks
```

### ProGuard

Gradle uses [ProGuard] to create the standalone JAR file for the IceGrid
GUI tool.

The Maven package id for the ProGuard gradle plugin is as follows:

```gradle
groupId=com.guardsquare, version=7.3.1, artifactId=proguard-gradle
```

### Java Application Bundler

Under macOS Gradle uses the Java Application Bundler to create an application
bundle for the IceGrid GUI tool.

The Maven package id for the application bundler package is as follows:

```gradle
groupId=com.panayotis, version=1.1.0, artifactId=appbundler
```

## Building Ice for Java

The build system requires the Slice to Java compiler from Ice for C++.

On Windows, you must set the `CPP_PLATFORM` and `CPP_CONFIGURATION` environment
variables to match the platform and configuration used in your C++ build:

```shell
set CPP_PLATFORM=x64
set CPP_CONFIGURATION=Debug
```

The supported values for `CPP_PLATFORM` are `Win32` and `x64` and the supported
values for `CPP_CONFIGURATION` are `Debug` and `Release`.

Before building Ice for Java, review the settings in the file `gradle.properties` and edit as necessary.

To build Ice, all services, and tests, run

```shell
gradlew build
```

Upon completion, the Ice JAR and POM files are placed in the `lib` subdirectory.

If at any time you wish to discard the current build and start a new one, use
these commands:

```shell
gradlew clean
gradlew build
```

## Installing Ice for Java

To install Ice for Java in the directory specified by the `prefix` variable in
`gradle.properties` run the following command:

```shell
gradlew install
```

The following JAR files will be installed to `<prefix>/lib`.

* glacier2-3.7.10.jar
* ice-3.7.10.jar
* icebox-3.7.10.jar
* icebt-3.7.10.jar
* icediscovery-3.7.10.jar
* icegrid-3.7.10.jar
* icegridgui.jar
* icelocatordiscovery-3.7.10.jar
* icestorm-3.7.10.jar

POM files are also installed for ease of deployment to a Maven-based
distribution system.

## Running the Java Tests

Some of the Ice for Java tests employ applications that are part of the Ice for
C++ distribution. If you have not built Ice for C++ in this source distribution
then you must set the `ICE_HOME` environment variable with the path name of your
Ice installation. On Unix:

```shell
export ICE_HOME=/opt/Ice-3.7.10 (For local build)
export ICE_HOME=/usr (For RPM installation)
```

On Windows:

```shell
set ICE_HOME=C:\Program Files\ZeroC\Ice-3.7.10
```

Python is required to run the test suite. To run the tests, open a command
window and change to the top-level directory. At the command prompt, execute:

```shell
python allTests.py
```

If everything worked out, you should see lots of `ok` messages. In case of a
failure, the tests abort with `failed`.

## Building the Ice for Android Tests

The `test/android/controller` directory contains an Android Studio project for
the Ice test suite controller.

### Android Build Requirements

To build an Ice application for Android, you need Android Studio and the Android SDK
build tools. We tested the following components:

* Android Studio Giraffe
* Android SDK 33

To use Ice's Java mapping with Java 8, you need at least API level 24:

* Android 7 (API24)

### Building the Android Test Controller

You must first build Ice for Java refer to [Building Ice for Java](#building-ice-for-java)
for instructions, then follow these steps:

1. Start Android Studio
2. Select "Open an existing Android Studio project"
3. Navigate to and select the "java/test/android/controller" subdirectory
4. Click OK and wait for the project to open and build

### Running the Android Test Suite

The Android Studio project contains a `controller` app for the Ice test
suite. Prior to running the app, you must disable Android Studio's Instant Run
feature, located in File / Settings / Build, Execution, Deployment /
Instant Run.

Tests are started from the dev machine using the `allTests.py` script, similar
to the other language mappings. The script uses Ice for Python to communicate
with the Android app, therefore you must build the [Python mapping] before continuing.

You also need to add the `tools\bin`, `platform-tools` and `emulator`
directories from the Android SDK to your PATH. On macOS, you can use the
following commands:

```shell
export PATH=~/Library/Android/sdk/cmdline-tools/latest/bin:$PATH
export PATH=~/Library/Android/sdk/platform-tools:$PATH
export PATH=~/Library/Android/sdk/emulator:$PATH
```

On Windows, you can use the following commands:

```shell
set PATH=%LOCALAPPDATA%\Android\sdk\cmdline-tools\latest\bin;%PATH%
set PATH=%LOCALAPPDATA%\Android\sdk\platform-tools;%PATH%
set PATH=%LOCALAPPDATA%\Android\sdk\emulator;%PATH%
```

Run the tests with the Android emulator by running the following command:

```shell
python allTests.py --android --controller-app
```

To run the tests on a Android device connected through USB, you can use
the `--device=usb` option as shown below:

```shell
python allTests.py --android --device=usb --controller-app
```

To connect to an Android device that is running adb you can use the
`--device=<ip-address>`

```shell
python allTests.py --android --device=<ip-address> --controller-app
```

To run the tests against a `controller` application started from Android
Studio you should omit the `--controller-app` option from the commands above.

## IceGrid GUI Tool

Ice for Java includes the IceGrid GUI tool. It can be found in the file
`lib/icegridgui.jar`.

This JAR file is completely self-contained and has no external dependencies.
You can start the tool with the following command:

```shell
java -jar icegridgui.jar
```

On macOS, the build also creates an application bundle named IceGrid GUI. You
can start the IceGrid GUI tool by double-clicking the IceGrid GUI icon in
Finder.

[binary distribution]: https://zeroc.com/downloads/ice
[major version]: https://docs.oracle.com/javase/specs/jvms/se21/html/jvms-4.html#jvms-4.1-200-B.2
[supported platforms]: https://doc.zeroc.com/ice/3.7/release-notes/supported-platforms-for-ice-3-7-10
[Gradle]: https://gradle.org
[ProGuard]: http://proguard.sourceforge.net
[Apache Commons Compress]: https://commons.apache.org/proper/commons-compress/
[Python Mapping]: ../python
