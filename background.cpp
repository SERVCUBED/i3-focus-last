#include "background.h"

namespace i3_focus_last
{

    void background::winRemoveIfExists (uint64_t id)
    {
      for (auto i = idsMap.begin (); i != idsMap.end (); i++)
        {
          if (i->second == id)
            {
              DEBUG_MSG ("Exists. Removing " << id);
              idsMap.erase (i);
              return;
            }
        }
    }

    void background::winSetWorkspace (uint64_t id, int newWSNum)
    {
      for (auto &i : idsMap)
        {
          if (i.second == id)
            {
              i.first = newWSNum;
              return;
            }
        }
      idsMap.emplace_back (std::make_pair (newWSNum, id));
    }

    uint64_t background::winGetLastOnWorkspace (int WSNum)
    {
      // Skip the 1st. Dont want most recent
      int skip = WSNum == currentWorkspaceNum;
      DEBUG_MSG ("Getting last on workspace " << WSNum << " skip? " << skip);
      for (auto i = idsMap.rbegin (); i != idsMap.rend (); i++)
        if (i->first == WSNum && (!skip || --skip))
          return i->second;
      return 0;
    }

    const std::string background::workspaceFromOutput (const std::string &outputName)
    {
      if (outputName == currentOutput)
        return currentWorkspace;

      int num = workspaceNumFromOutput (outputName);
      if (num > 0)
        return workspaceMap[num];

      ERROR_MSG ("Couldn't find output with name: " << outputName);
      return "";
    }

    const std::string background::workspaceNameFromNum (int workspaceNum)
    {
      if (workspaceNum == currentWorkspaceNum)
        return currentWorkspace;

      return workspaceMap[workspaceNum];
    }

    int background::workspaceNumFromName (const std::string &workspaceName)
    {
      if (workspaceName == currentWorkspace)
        return currentWorkspaceNum;

      for (const auto &i : workspaceMap)
        {
          if (i.second == workspaceName)
            return i.first;
        }
      return 0;
    }

    int background::workspaceNumFromOutput (const std::string &outputName)
    {
      if (outputName == currentOutput)
        return currentWorkspaceNum;

      return outputMap[outputName];
    }

    std::string background::outputFromWorkspaceName (const std::string &workspaceName)
    {
      if (workspaceName == currentWorkspace)
        return currentOutput;

      int num = workspaceNumFromName (workspaceName);

      if (num != 0)
        for (const auto &i : outputMap)
          {
            if (i.second == num)
              return i.first;
          }

      ERROR_MSG ("Couldn't find workspace with name: " << workspaceName);
      return "";
    }

    i3ipc::container_t *background::getWindowFromContainer (i3ipc::container_t *container, bool reversed)
    {
      //if (container->xwindow_id != NULL)
      //  std::cout << "xwindow_id " << container->xwindow_id << std::endl;
      DEBUG_MSG ("con id " << container->id << " reversed? " << reversed << " layout "
                           << static_cast<char>(container->layout));
      if (!container->nodes.empty ())
        {
          if (reversed)
            container->nodes.reverse ();
          for (auto &node : container->nodes)
            {
              i3ipc::container_t *con = node.get ();
              con = getWindowFromContainer (con, reversed && con->layout
                                                             == i3ipc::ContainerLayout::STACKED); // TODO fix, keep true if has child nodes?
              if (con != nullptr)
                {
                  //std::cout << "Looking at con id " << con->id << std::endl;
                  return con;
                }
              DEBUG_MSG ("Container null. Backing up one level");
            }
        }
      return (container->xwindow_id != 0) ? container : nullptr;
    }

    void background::focusConID (uint64_t id)
    {
      if (id == 0)
        {
          ERROR_MSG ("Container ID is zero");
          return;
        }

      std::stringstream cmds;
      cmds << "[con_id=" << id << "] focus";

      DEBUG_MSG ("Sending command: " << cmds.str ());
      conn.send_command (cmds.str ());
    }

    void background::focusLastActive ()
    {
      if (idsMap.size () < 2)
        {
          ERROR_MSG ("Not enough containers to focus last");
          return;
        }

      focusConID ((idsMap.end () - 2)->second);
    }

    void background::focusLastOnWorkspace (int workspaceNum)
    {
      focusConID (winGetLastOnWorkspace (workspaceNum));
    }

    void background::focusLastOnOutput (const std::string &outputName)
    {
      if (outputName == currentOutput)
        {
          focusConID (winGetLastOnWorkspace (currentWorkspaceNum));
          return;
        }

      focusConID (winGetLastOnWorkspace (workspaceNumFromOutput (outputName)));
    }

    void background::focusTopBottom (const std::string &outputName, const std::string &workspaceName, bool top)
    {
      const std::shared_ptr<i3ipc::container_t> tree = conn.get_tree ();
      std::list<std::shared_ptr<i3ipc::container_t>> nodes = tree.get ()->nodes; // Outputs

      for (const auto &n : nodes)
        {
          i3ipc::container_t *con = n.get ();
          if (con->name == outputName && con->type == "output")
            {
              DEBUG_MSG ("Found output " << con->name);
              nodes = con->nodes;
              break;
            }
        }

      for (const auto &n : nodes)
        {
          i3ipc::container_t *con = n.get ();
          if (con->name == "content")
            {
              DEBUG_MSG ("Found content " << con->id);
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
              DEBUG_MSG ("Found workspace " << con->name);
              con = getWindowFromContainer (con, !top);

              if (con == nullptr)
                break;

              focusConID (con->id);
              return;
            }
        }

      ERROR_MSG ("Whoops, didn't find a container to focus; output " << outputName
                                                                     << "; workspace " << workspaceName);
    }

    void background::focusTopBottomOnOutput (const std::string &outputName, bool top)
    {
      focusTopBottom (outputName, workspaceFromOutput (outputName), top);
    }

    void background::focusTopBottomOnWorkspace (const std::string &workspaceName, bool top)
    {
      focusTopBottom (outputFromWorkspaceName (workspaceName), workspaceName, top);
    }

    void background::signalHandler (int signum)
    {
      //std::cout << "Interrupt signal (" << signum << ") received.\n";

      if (signum != SIGINT)
        unlink (pipefname);

      exit (0);
    }

    void background::handlePipeRead (char buf[])
    {
      DEBUG_MSG ("Command: " << buf);
      if (buf[0] == 'e')
        exit (0);

      // Parse workspace/output/any
      // flw flo fl, ftw fto ft, fbw fbo fb, d0 d1

      if (buf[0] == 'f')
        {
          if (buf[1] != 't' && buf[1] != 'b' && buf[1] != 'l')
            return;

          if (buf[2] == '\n')
            {
              if (buf[1] == 'l')
                focusLastActive ();
              else
                focusTopBottom (currentOutput, currentWorkspace, buf[1] == 't');
              return;
            }

          std::string arg (buf + 3, index (buf + 3, '\n') - (buf + 3));
          DEBUG_MSG ("argstring: " << arg);

          if (buf[1] == 'l')
            {
              if (buf[2] == 'o')
                focusLastOnOutput (arg);
              else if (buf[2] == 'w')
                focusLastOnWorkspace (workspaceNumFromName (arg));
              return;
            }
          if (buf[2] == 'o')
            focusTopBottomOnOutput (arg, buf[1] == 't');
          else if (buf[2] == 'w')
            focusTopBottomOnWorkspace (arg, buf[1] == 't');
        }
#if DEBUG
      else if (buf[0] == 'd')
        {
          isDebug = buf[1] == '1';
          std::cout << "Set debug mode " << isDebug << std::endl;
        }
#endif
    }

    void background::handlePipe ()
    {
      int fd;
      char buf[MAX_BUF];

      mkfifo (pipefname, 0666);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
      while (true)
        {
          fd = open (pipefname, O_RDONLY);
          read (fd, buf, MAX_BUF);
          //printf ("Received: %s\n", buf);
          close (fd);
          handlePipeRead (buf);
        }
#pragma clang diagnostic pop
    }

    void background::run (int argc, char **argv)
    {
      if (argc > 1)
        std::istringstream (argv[argc - 1]) >> delay;
      //std::cout << "delay: " << delay << std::endl;

      signal (SIGHUP, background::signalHandler);
      signal (SIGKILL, background::signalHandler);
      signal (SIGINT, background::signalHandler);
      signal (SIGTERM, background::signalHandler);

      std::thread pipeThread (&background::handlePipe, this);

      conn.subscribe (i3ipc::ET_WINDOW | i3ipc::ET_WORKSPACE);

      // Handler of WINDOW EVENT
      conn.signal_window_event.connect ([this] (const i3ipc::window_event_t &ev)
                                        {
                                            DEBUG_MSG ("window_event: " << (char) ev.type);
                                            if (!ev.container)
                                              return;
                                            if (ev.type == i3ipc::WindowEventType::FOCUS)
                                              {
                                                uint64_t id = ev.container->id;
                                                DEBUG_MSG ("id " << id);
                                                double time = std::clock ();
//          if (time > next_evt || idsMap.end ()->first == currentWorkspaceNum)
//            focused_index = !focused_index;
                                                winRemoveIfExists (id);
                                                if (time < next_evt
                                                    && (idsMap.end () - 1)->first == currentWorkspaceNum)
                                                  {
                                                    DEBUG_MSG ("Replacing");
                                                    (idsMap.end ()
                                                     - 1)->second = id; // insert after last item on current WS
                                                  }
                                                else
                                                  idsMap.emplace_back (currentWorkspaceNum, id);
                                                next_evt = time + delay;
                                                //std::cout << "\tSwitched to #" << ev.container->id << " - " << ev.container->name << ' - ' << ev.container->window_properties.window_role << std::endl;
                                              }
                                            else if (ev.type == i3ipc::WindowEventType::CLOSE)
                                              {
                                                winRemoveIfExists (ev.container->id);
                                              }
                                            else if (ev.type == i3ipc::WindowEventType::MOVE)
                                              {
                                                winSetWorkspace (ev.container->id, currentWorkspaceNum);
                                              }
                                        });

      // Handler of workspace_event
      conn.signal_workspace_event.connect ([this] (const i3ipc::workspace_event_t &ev)
                                           {
                                               DEBUG_MSG ("workspace_event: " << (char) ev.type);
                                               if (ev.type != i3ipc::WorkspaceEventType::FOCUS)
                                                 return;
                                               currentWorkspace = ev.current->name;
                                               currentWorkspaceNum = ev.current->num;
                                               currentOutput = ev.current->output;

                                               workspaceMap[currentWorkspaceNum] = currentWorkspace;
                                               outputMap[currentOutput] = currentWorkspaceNum;
                                           });

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
      while (true)
        {
          conn.handle_event ();
        }
#pragma clang diagnostic pop
    }
}
