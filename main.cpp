#include <iostream>
#include <ctime>
#include <sstream>
#include <csignal>
#include <unistd.h>


// Using https://github.com/drmgc/i3ipcpp
#include <i3ipc++/ipc.hpp>

i3ipc::connection  conn;
double next_evt = 0;
uint64_t ids[] = {NULL, NULL};
int focused_index = 0;
int delay = 100;
const char *pidfname = "/tmp/i3-focus-last.pidfile";

void signalHandler( int signum ) {
  //std::cout << "Interrupt signal (" << signum << ") received.\n";

  if (signum == SIGUSR1)
    {
      if (ids[!focused_index] == NULL)
          return;

      std::stringstream ss;
      //std::cout << "last focused: " << ids[!focused_index] << std::endl;
      ss << "[con_id=" << ids[!focused_index] << "] focus";
      //std::cout << ss.str () << std::endl;
      conn.send_command (ss.str ());
      return;
    }

  std::remove (pidfname);
  exit(signum);
}

int main ()
{
  signal(SIGUSR1, signalHandler);
  signal(SIGKILL, signalHandler);
  signal(SIGINT, signalHandler);

  std::cout << getpid () << std::endl;

  FILE *pidfile = std::fopen (pidfname, "w");
  fprintf (pidfile, "%d", getpid ());
  std::fclose (pidfile);

  //std::cout << "delay: " << delay << std::endl;

  conn.subscribe(i3ipc::ET_WINDOW);

  // Handler of WINDOW EVENT
  conn.signal_window_event.connect([](const i3ipc::window_event_t&  ev) {
      //std::cout << "window_event: " << (char)ev.type << std::endl;
      if (ev.type != i3ipc::WindowEventType::FOCUS) // Handle type 'NEW' here too?
        return;
      double time = std::clock ();
      if (time < next_evt)
        {
          //std::cout << "time < delay: " << (time - next_evt) << std::endl;
          return;
        }
      if (ev.container) {
          if (ids[focused_index] != ev.container->id)
            {
              focused_index = !focused_index;
              //std::cout << "\tSwitched to #" << ev.container->id << " - " << ev.container->name << ' - ' << ev.container->window_properties.window_role << std::endl;
              ids[focused_index] = ev.container->id;
            }
          next_evt = time + delay;
        }
  });

  while (true) {
      conn.handle_event();
  }

  return 0;
}