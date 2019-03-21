# i3-focus-last

Quickly switch to the last focused window with i3.

## To compile:
    git clone --recursive https://github.com/SERVCUBED/i3-focus-last.git
    cd i3-focus-last
    cmake .
    make

## To switch to the last focused window:

The application responds to the signal SIGUSR1 sent to the PIDfile `/tmp/i3-focus-last.pidfile`:

    pkill -F /tmp/i3-focus-last.pidfile -USR1
