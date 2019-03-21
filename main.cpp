#include <iostream>
#include <ctime>
#include <sstream>
#include <csignal>
#include <unistd.h>


// Using https://github.com/drmgc/i3ipcpp
#include <i3ipc++/ipc.hpp>
#include <fstream>

i3ipc::connection  conn;
double next_evt = 0;
uint64_t ids[] = {NULL, NULL};
int focused_index = 0;
int delay = 100;
std::string currentWorkspace;
std::string currentOutput;
const char *pidfname = "/tmp/i3-focus-last.pidfile";


i3ipc::container_t getWindowFromContainer(i3ipc::container_t *container)
{
  for (auto n : container->nodes)
    {
      // Return the first node from each nested container
      i3ipc::container_t *con = n.get ();
      return getWindowFromContainer (con);
    }
  return *container;
}

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

  if (signum == SIGUSR2)
    {
      const std::shared_ptr<i3ipc::container_t> tree = conn.get_tree ();
      std::list<std::shared_ptr<i3ipc::container_t>> nodes = tree.get()->nodes; // Outputs

      for (auto n : nodes)
        {
          i3ipc::container_t *con = n.get ();
          if (con->name == currentOutput && con->type == "output")
            {
              //std::cout << "Found output " << con->name << std::endl;
              nodes = con->nodes;
              break;
            }
        }

      for (auto n : nodes)
        {
          i3ipc::container_t *con = n.get ();
          if (con->name == "content")
            {
              //std::cout << "Found content " << con->id << std::endl;
              nodes = con->nodes; // Workspaces
              break;
            }
        }

      //std::cout << "Looking for workspace " << currentWorkspace << std::endl;
      for (auto n : nodes)
        {
          i3ipc::container_t *con = n.get ();
          if (con->name == currentWorkspace && con->type == "workspace")
            {
              //std::cout << "Found workspace " << con->name << std::endl;
              nodes = con->nodes; // Containers

              std::stringstream ss;
              ss << "[con_id=" << getWindowFromContainer (con).id << "] focus";
              //std::cout << ss.str () << std::endl;
              conn.send_command (ss.str ());

              return;
            }
        }

      std::cerr << "Whoops, didn't find a container to focus" << std::endl;

      return;
    }

  if (signum != SIGHUP)
    std::remove (pidfname);

  exit(0);
}

int main (int argc, char *argv[])
{
  int pid = NULL;

  if (argc > 1)
     std::istringstream(argv[argc - 1]) >> delay;
  //std::cout << "delay: " << delay << std::endl;

  // Quit without removing PID file, to be sent when another process takes over
  signal(SIGHUP, signalHandler);
  // Focus last active window
  signal(SIGUSR1, signalHandler);
  signal(SIGUSR2, signalHandler);
  signal(SIGKILL, signalHandler);
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  std::ifstream pidstream(pidfname);
  if (pidstream && pidstream.is_open ())
    {
      pidstream >> pid;
      if (pid > 0 && kill (pid, 0) == 0)
        {
          kill (pid, SIGHUP);
          std::cerr << "SIGHUP pid " << pid << std::endl;
        }
      pidstream.close ();
    }

  std::ofstream opidstream(pidfname, std::ios_base::out | std::ios_base::trunc);
  pid = getpid ();
  std::cout << getpid () << std::endl;
  if (opidstream && opidstream.is_open ()) {
      opidstream << pid;
      opidstream.close ();
    }
  else
    {
      std::cerr << "Unable to write PID file." << std::endl;
      return 74; // input/output error
    }

  conn.subscribe(i3ipc::ET_WINDOW | i3ipc::ET_WORKSPACE);

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

  // Handler of workspace_event
  conn.signal_workspace_event.connect([](const i3ipc::workspace_event_t&  ev) {
      //std::cout << "workspace_event: " << (char)ev.type << std::endl;
      if (ev.type != i3ipc::WorkspaceEventType::FOCUS)
        return;
      currentWorkspace = ev.current->name;
      currentOutput = ev.current->output;
  });

  while (true) {
      conn.handle_event();
  }

  return 0;
}