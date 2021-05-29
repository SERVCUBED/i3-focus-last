# i3-focus-last

Quickly switch to the last focused window with i3.

## To compile:
    git clone --recursive https://github.com/SERVCUBED/i3-focus-last.git
    cd i3-focus-last
    cmake .
    make

## Usage:

The application responds to the commands written to the pipe `/run/user/$UID/i3-focus-last.sock`. Use your
favourite keybinding software or i3's own configuration files to bind a key to:

    // Switch to last focused window
    echo fl > /run/user/$UID/i3-focus-last.sock

    // Switch to first window in the current workspace
    echo ft > /run/user/$UID/i3-focus-last.sock

    // Switch to first window on the output DP-2
    echo ftoDP-2 > /run/user/$UID/i3-focus-last.sock

    // Switch to last window on the workspace "Home"
    echo fbwHome > /run/user/$UID/i3-focus-last.sock

Alternatively, if `i3-focus-last` is already running, you can execute the same `i3-focus-last` binary, but with your command as the first argument. If the daemon is not already running, the command will exit with failure status.

    // Switch to last focused window
    ./i3-focus-last fl

    // Switch to first window in the current workspace
    ./i3-focus-last ft

    // Switch to first window on the output DP-2
    ./i3-focus-last ftoDP-2

    // Switch to last window on the workspace "Home"
    ./i3-focus-last fbwHome

## Debugging
By default, the binary is built in release mode without debugging features. Use the following commands to build with debugging messages supported.

    cmake -B cmake-build-debug
    cmake --build cmake-build-debug

The build files would then be written to the `cmake-build-debug` directory, relative to the project root. You can then use `-d` as the first argument to the `Debug`-built `i3-focus-last` binary to automatically show debug messages when running as either daemon or command mode. You can later enable or disable the daemon's outputting of debugging messages by sending the `d#` commands, where `#` is the debug message level. `d0` disables messages completely.

    // Run the main daemon
    cmake-build-debug/i3-focus-last -d

    // Stop the daemon outputting debugging messages
    cmake-build-debug/i3-focus-last -d d0

    // Switch to last focused window
    cmake-build-debug/i3-focus-last -d fl

    // Enable verbose daemon debugging messages
    cmake-build-debug/i3-focus-last d15

See the file [util.h](util.h) for details about the debugging levels available.