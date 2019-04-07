# i3-focus-last

Quickly switch to the last focused window with i3.

## To compile:
    git clone --recursive https://github.com/SERVCUBED/i3-focus-last.git
    cd i3-focus-last
    cmake .
    make

## Usage:

The application responds to the commands written to the pipe `/tmp/i3-focus-last.pipe`. Use your
favourite keybinding software or i3's own configuration files to bind a key to:

    // Switch to last focused window
    echo fl > /tmp/i3-focus-last.pipe

    // Switch to first window in the current workspace
    echo ft > /tmp/i3-focus-last.pipe

    // Switch to first window on the output DP-2
    echo ftoDP-2 > /tmp/i3-focus-last.pipe

    // Switch to last window on the workspace "Home"
    echo fbwHome > /tmp/i3-focus-last.pipe
