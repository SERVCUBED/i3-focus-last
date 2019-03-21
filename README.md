# i3-focus-last

Quickly switch to the last focused window with i3.

## To compile:
    git clone --recursive https://github.com/SERVCUBED/i3-focus-last.git
    cd i3-focus-last
    cmake .
    make

## Usage:

The application responds to the signals SIGUSR1 and SIGUSR2 sent to the PIDfile `/tmp/i3-focus-last.pidfile`. Use your
favourite keybinding software or i3's own configuration files to bind a key to:

    // Switch to last focused window
    pkill -F /tmp/i3-focus-last.pidfile -USR1

    // Switch to first window in the workspace
    pkill -F /tmp/i3-focus-last.pidfile -USR2
