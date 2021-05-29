#include <iostream>
#include <ctime>
#include <sstream>
#include <csignal>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <thread>
#include <fstream>
#include <cstring>
#include <map>
#include "util.h"

// Using https://github.com/drmgc/i3ipcpp
#include <i3ipc++/ipc.hpp>

namespace i3_focus_last
{
    class background {
     private:

      i3ipc::connection conn;
      double next_evt = 0;
      std::vector<std::pair<int, uint64_t>> idsMap;
      std::map<int, std::string> workspaceMap;
      std::map<std::string, int> outputMap;
      int delay = 50;
      int maxAutoFloatH = 600;
      int maxAutoFloatW = 700;
      std::string currentWorkspace;
      int currentWorkspaceNum;
      std::string currentOutput;

#define MAX_BUF 512

      void winSetLastFocused (uint64_t id);

      void winRemoveIfExists (uint64_t id);

      void winSetWorkspace (uint64_t id, int newWSNum);

      uint64_t winGetLastOnWorkspace (int WSNum);

      const std::string workspaceFromOutput (const std::string &outputName);

      const std::string workspaceNameFromNum (int workspaceNum);

      int workspaceNumFromName (const std::string &workspaceName);

      int workspaceNumFromOutput (const std::string &outputName);

      std::string outputFromWorkspaceName (const std::string &workspaceName);

      i3ipc::container_t *getWindowFromContainer (i3ipc::container_t *container, bool reversed = false);

      void focusConID (uint64_t id);

      void sendCommandConID (uint64_t id, const char *command);

      void focusLastActive ();

      void focusLastOnWorkspace (int workspaceNum);

      void focusLastOnOutput (const std::string &outputName);

      void focusTopBottom (const std::string &outputName, const std::string &workspaceName, bool top);

      void focusTopBottomOnOutput (const std::string &outputName, bool top = true);

      void focusTopBottomOnWorkspace (const std::string &workspaceName, bool top = true);

      static void writeToPath (const char *fname, u_int64_t  id);

      void handlePipeRead (char buf[]);

      [[noreturn]] void handlePipe ();

     public:
      void handle_window_event (const i3ipc::window_event_t &ev);
      void handle_workspace_event (const i3ipc::workspace_event_t &ev);
      static void signalHandler (int signum);
      void run (int argc, char *argv[]);
    };
}
