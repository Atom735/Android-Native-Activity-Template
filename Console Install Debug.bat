CALL gradle --info installDebug
PAUSE

rem ------------------------------------------------------------
rem All tasks runnable from root project
rem ------------------------------------------------------------

rem Android tasks
rem -------------
rem androidDependencies - Displays the Android dependencies of the project.
rem signingReport - Displays the signing info for each variant.
rem sourceSets - Prints out all the source sets defined in this project.

rem Build tasks
rem -----------
rem assemble - Assembles all variants of all applications and secondary packages.
rem assembleAndroidTest - Assembles all the Test applications.
rem assembleDebug - Assembles all Debug builds.
rem assembleRelease - Assembles all Release builds.
rem build - Assembles and tests this project.
rem buildDependents - Assembles and tests this project and all projects that depend on it.
rem buildNeeded - Assembles and tests this project and all projects it depends on.
rem clean - Deletes the build directory.
rem cleanBuildCache - Deletes the build cache directory.
rem compileDebugAndroidTestSources
rem compileDebugSources
rem compileDebugUnitTestSources
rem compileReleaseSources
rem compileReleaseUnitTestSources
rem mockableAndroidJar - Creates a version of android.jar that's suitable for unit tests.

rem Build Setup tasks
rem -----------------
rem init - Initializes a new Gradle build.
rem wrapper - Generates Gradle wrapper files.

rem Help tasks
rem ----------
rem buildEnvironment - Displays all buildscript dependencies declared in root project 'NativeTest'.
rem components - Displays the components produced by root project 'NativeTest'. [incubating]
rem dependencies - Displays all dependencies declared in root project 'NativeTest'.
rem dependencyInsight - Displays the insight into a specific dependency in root project 'NativeTest'.
rem dependentComponents - Displays the dependent components of components in root project 'NativeTest'. [incubating]
rem help - Displays a help message.
rem model - Displays the configuration model of root project 'NativeTest'. [incubating]
rem projects - Displays the sub-projects of root project 'NativeTest'.
rem properties - Displays the properties of root project 'NativeTest'.
rem tasks - Displays the tasks runnable from root project 'NativeTest' (some of the displayed tasks may belong to subprojects).

rem Install tasks
rem -------------
rem installDebug - Installs the Debug build.
rem installDebugAndroidTest - Installs the android (on device) tests for the Debug build.
rem uninstallAll - Uninstall all applications.
rem uninstallDebug - Uninstalls the Debug build.
rem uninstallDebugAndroidTest - Uninstalls the android (on device) tests for the Debug build.
rem uninstallRelease - Uninstalls the Release build.

rem Verification tasks
rem ------------------
rem check - Runs all checks.
rem connectedAndroidTest - Installs and runs instrumentation tests for all flavors on connected devices.
rem connectedCheck - Runs all device checks on currently connected devices.
rem connectedDebugAndroidTest - Installs and runs the tests for debug on connected devices.
rem deviceAndroidTest - Installs and runs instrumentation tests using all Device Providers.
rem deviceCheck - Runs all device checks using Device Providers and Test Servers.
rem lint - Runs lint on all variants.
rem lintDebug - Runs lint on the Debug build.
rem lintRelease - Runs lint on the Release build.
rem test - Run unit tests for all variants.
rem testDebugUnitTest - Run unit tests for the debug build.
rem testReleaseUnitTest - Run unit tests for the release build.