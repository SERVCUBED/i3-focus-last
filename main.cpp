#include <iostream>
#include <ctime>
#include <sstream>
#include <csignal>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <thread>

// Using https://github.com/drmgc/i3ipcpp
#include <i3ipc++/ipc.hpp>
#include <fstream>
#include <tkPort.h>

i3ipc::connection  conn;
double next_evt = 0;
uint64_t ids[] = {NULL, NULL};
int focused_index = 0;
int delay = 200;
std::string currentWorkspace;
std::string currentOutput;
const char *pipefname = "/tmp/i3-focus-last.pipe";

#define MAX_BUF 512

i3ipc::container_t *getWindowFromContainer(i3ipc::container_t *container, bool reversed = false)
{
  //if (container->xwindow_id != NULL)
  //  std::cout << "xwindow_id " << container->xwindow_id << std::endl;
  if (!container->nodes.empty ())
    {
      if (reversed)
        container->nodes.reverse ();
      for (auto &node : container->nodes)
      {
          i3ipc::container_t *con = node.get ();
          con = getWindowFromContainer (con, reversed && con->layout == i3ipc::ContainerLayout::STACKED);
          if (con != nullptr)
            {
              //std::cout << "Looking at con id " << con->id << std::endl;
              return con;
            }
          //std::cout << "Container null. Backing up one level" << std::endl;
        }
    }
  return (container->xwindow_id != NULL)? container : nullptr;
}

void focusLastActive()
{
  if (ids[!focused_index] == NULL)
    return;

  std::stringstream ss;
  //std::cout << "last focused: " << ids[!focused_index] << std::endl;
  ss << "[con_id=" << ids[!focused_index] << "] focus";
  //std::cout << ss.str () << std::endl;
  conn.send_command (ss.str ());
}

void focusTopBottom(const std::string &outputName, const std::string &workspaceName, bool top)
{
  const std::shared_ptr<i3ipc::container_t> tree = conn.get_tree ();
  std::list<std::shared_ptr<i3ipc::container_t>> nodes = tree.get()->nodes; // Outputs

  for (const auto &n : nodes)
    {
      i3ipc::container_t *con = n.get ();
      if (con->name == outputName && con->type == "output")
        {
          //std::cout << "Found output " << con->name << std::endl;
          nodes = con->nodes;
          break;
        }
    }

  for (const auto &n : nodes)
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
  for (const auto &n : nodes)
    {
      i3ipc::container_t *con = n.get ();
      if (con->name == workspaceName && con->type == "workspace")
        {
          //std::cout << "Found workspace " << con->name << std::endl;
          con = getWindowFromContainer (con, !top);

          if (con == nullptr)
            break;

          std::stringstream ss;
          ss << "[con_id=" << con->id << "] focus";
          //std::cout << ss.str () << std::endl;
          conn.send_command (ss.str ());

          return;
        }
    }

  std::cerr << "Whoops, didn't find a container to focus; output " << outputName
            << "; workspace " << workspaceName << std::endl;
}

void focusTopBottomOnOutput(const std::string &outputName, bool top = true)
{
  if (outputName == currentOutput)
    {
      focusTopBottom (currentOutput, currentWorkspace, top);
      return;
    }

  const std::vector<std::shared_ptr<i3ipc::output_t>> outs = conn.get_outputs ();

  for (const std::shared_ptr<i3ipc::output_t> &out : outs)
    {
      if (out->name == outputName)
        {
          focusTopBottom (outputName, out->current_workspace, top);
          return;
        }
    }
  std::cerr << "Couldn't find output with name: " << outputName << std::endl;
}

void focusTopBottomOnWorkspace(const std::string &workspaceName, bool top = true)
{
  if (workspaceName == currentWorkspace)
    {
      focusTopBottom (currentOutput, currentWorkspace, top);
      return;
    }

  const std::vector<std::shared_ptr<i3ipc::workspace_t>> wss = conn.get_workspaces ();

  for (const std::shared_ptr<i3ipc::workspace_t> &ws : wss)
    {
      if (ws->name == workspaceName)
        {
          focusTopBottom (ws->output, workspaceName, top);
          return;
        }
    }
  std::cerr << "Couldn't find workspace with name: " << workspaceName << std::endl;
}

void signalHandler( int signum ) {
  //std::cout << "Interrupt signal (" << signum << ") received.\n";

  unlink (pipefname);

  exit(0);
}


void handlePipeRead(char buf[])
{
  if (buf[0] == 'e')
    exit(0);

  // Parse workspace/output/any
  // flw flo fl, ftw fto ft, fbw fbo fb

  if (buf[0] == 'f')
    {
      if (buf[1] == 'l')
        {
          focusLastActive ();
          return;
        }
      if (buf[1] != 't' && buf[1] != 'b')
        return;

      if (buf[2] == '\n')
        {
          focusTopBottom (currentOutput, currentWorkspace, buf[1] == 't');
          return;
        }

      std::string arg (buf + 3, index (buf + 3, '\n') - (buf + 3));
      //std::cout << "argstring: " << arg << std::endl;
      if (buf[2] == 'o')
        focusTopBottomOnOutput (arg, buf[1] == 't');
      else if (buf[2] == 'w')
        focusTopBottomOnWorkspace (arg, buf[1] == 't');
    }
}


void handlePipe()
{
  int fd;
  char buf[MAX_BUF];

  fd = open (pipefname, O_WRONLY);
  if (fd != -1)
    {
      write (fd, "e\n", 2);
      close (fd);
    }
  mkfifo(pipefname, 0666);

  while (true)
    {
      fd = open (pipefname, O_RDONLY);
      read (fd, buf, MAX_BUF);
      //printf ("Received: %s\n", buf);
      close (fd);
      handlePipeRead(buf);
    }
}

int main (int argc, char *argv[])
{
  if (argc > 1)
     std::istringstream(argv[argc - 1]) >> delay;
  //std::cout << "delay: " << delay << std::endl;

  signal(SIGHUP, signalHandler);
  signal(SIGKILL, signalHandler);
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  std::thread pipeThread(handlePipe);

  //TODO re-enable
  conn.subscribe(i3ipc::ET_WINDOW | i3ipc::ET_WORKSPACE);

  // Handler of WINDOW EVENT
  conn.signal_window_event.connect([](const i3ipc::window_event_t&  ev) {
      //std::cout << "window_event: " << (char)ev.type << std::endl;
      if (ev.type != i3ipc::WindowEventType::FOCUS) // Handle type 'NEW' here too?
        return;
      if (ev.container) {
          uint64_t id = ev.container->id;
          double time = std::clock ();
          if (time > next_evt || ids[!focused_index] == id)
            focused_index = !focused_index;
          ids[focused_index] = id;
          next_evt = time + delay;
          //std::cout << "\tSwitched to #" << ev.container->id << " - " << ev.container->name << ' - ' << ev.container->window_properties.window_role << std::endl;
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