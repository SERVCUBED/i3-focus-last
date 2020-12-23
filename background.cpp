#include "background.h"

namespace i3_focus_last
{

    void background::winSetLastFocused (uint64_t id)
    {
      DBG_WINDOW ("id " << id);
      double time = std::clock ();
//          if (time > next_evt || idsMap.end ()->first == currentWorkspaceNum)
//            focused_index = !focused_index;
      winRemoveIfExists (id);
      if (time < next_evt
          && (idsMap.end () - 1)->first == currentWorkspaceNum)
        {
          DBG_WINDOW ("Replacing, time left: " << (next_evt - time));
          (idsMap.end ()
           - 1)->second = id; // insert after last item on current WS
        }
      else
        idsMap.emplace_back (currentWorkspaceNum, id);
      next_evt = time + delay;
    }

    void background::winRemoveIfExists (uint64_t id)
    {
      for (auto i = idsMap.begin (); i != idsMap.end (); i++)
        {
          if (i->second == id)
            {
              DBG_WINDOW ("Exists. Removing " << id);
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
      DBG_BASIC("Getting last on workspace " << WSNum << " skip? " << skip);
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
      DBG_BASIC("con id " << container->id << " reversed? " << reversed << " layout "
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
              DBG_BASIC("Container null. Backing up one level");
            }
        }
      return (container->xwindow_id != 0) ? container : nullptr;
    }

    void background::focusConID (uint64_t id)
    {
      sendCommandConID (id, "focus");
    }

    void background::sendCommandConID (uint64_t id, const char *command)
    {
      if (id == 0)
        {
          ERROR_MSG ("Container ID is zero");
          return;
        }

      std::stringstream cmds;
      cmds << "[con_id=" << id << "] " << command;

      DBG_CMD ("Sending command: " << cmds.str ());
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
              DBG_BASIC ("Found output " << con->name);
              nodes = con->nodes;
              break;
            }
        }

      for (const auto &n : nodes)
        {
          i3ipc::container_t *con = n.get ();
          if (con->name == "content")
            {
              DBG_BASIC ("Found content " << con->id);
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
              DBG_BASIC ("Found workspace " << con->name);
              if (!con->nodes.empty())
                con = getWindowFromContainer (con->nodes.begin ()->get (), !top);
              else
                break;

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

    void background::writeToPath (const char *fname, uint64_t id)
    {
      int fd = open (fname, O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK, S_IWUSR | S_IRUSR | S_IRGRP), len = 0;
      ERROR_MSG("buffer size: " << sizeof (uint64_t)*4);
      char content[sizeof (uint64_t)*4];
      if (unlikely (fd == -1))
        {
          ERROR_MSG ("Failed to write to file " << fname);
          return;
        }

      len = sprintf (content, "%lu", id);;

      if (unlikely(len == 0))
        {
          ERROR_MSG ("Failed to parse string");
          return;
        }

      ssize_t written = write (fd, content, len);
      close (fd);
      if (written == -1)
        {
          ERROR_MSG ("Error writing to file");
          return;;
        }

      DBG_BASIC ("Written: " << (int)written);
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
      DBG_CMD ("Command: " << buf);
      if (buf[0] == 'e')
        exit (0);

      // Parse workspace/output/any
      // flw flo fl, ftw fto ft, fbw fbo fb, gl glo; glw;, d0 d1
      // Get last: gl[o<output>|w<workspace>];<receiving socket path>

      if (buf[0] == 'f' || buf[0] == 'g')
        {
          if (buf[1] != 't' && buf[1] != 'b' && buf[1] != 'l')
            return;

          next_evt = 0;

          if (buf[2] == '\n')
            {
              if (buf[1] == 'l')
                focusLastActive ();
              else
                focusTopBottom (currentOutput, currentWorkspace, buf[1] == 't');
              return;
            }

          if (buf[0] == 'g')
            {
              char *arg = index (buf, ':');
              if (arg == nullptr)
                {
                  ERROR_MSG("File name required");
                  return;;
                }

              u_int64_t  id;
              const char *delim = "\n";
              char *fname = strtok(arg + 1 , delim);
              DBG_CMD("fname: " << fname);

              if (buf[2] == ':')
                id = (idsMap.end()-2)->second;
              else
                {
                  const char *delim2 = ":";
                  arg = strtok(buf + 3, delim2);
                  DBG_CMD("arg: " << arg);

                  int wsnum = 0;
                  if (buf[2] == 'w')
                    wsnum = workspaceNumFromName (arg);
                  else if (buf[2] == 'o')
                    {
                      if (arg == currentOutput)
                        wsnum = currentWorkspaceNum;
                      else
                        wsnum = workspaceNumFromOutput (arg);
                    }
                  DBG_CMD("wsnum: " << wsnum);
                  if (wsnum == 0)
                    {
                      ERROR_MSG("Invalid workspace or output argument");
                      return;
                    }
                  id = winGetLastOnWorkspace (wsnum);
                }
              DBG_CMD("got last id: " << id);
              if (id < 1024)
                {
                  ERROR_MSG("Value of ID  seems invalid");
                  return; // i3's ID is technically a pointer
                }
              writeToPath(fname, id);
              return;
            }

          std::string arg (buf + 3, index (buf + 3, '\n') - (buf + 3));
          DBG_CMD ("argstring: " << arg);

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
          isDebug = std::stoi (buf+1);
          if (isDebug > DBG_MAX_LEVEL)
            {
              ERROR_MSG ("Invalid debug level: " << (int)isDebug);
              isDebug = 15;
            }
          std::cout << "Set debug mode " << (int)isDebug << std::endl;
        }
#endif
    }

    [[noreturn]] void background::handlePipe ()
    {
      int fd;
      char buf[MAX_BUF];
      ssize_t len;
      memset (buf, 0, MAX_BUF);
      buf[MAX_BUF - 2] = '\n';

      mkfifo (pipefname, 0666);

      while (true)
        {
          fd = open (pipefname, O_RDONLY);
          len = read (fd, buf, MAX_BUF);
          close (fd);
          handlePipeRead (buf);
          memset (buf, 0, len);
        }
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
      conn.on_window_event = [this] (const i3ipc::window_event_t &ev)
                                        {
                                            DBG_WINDOW ("window_event: " << (char) ev.type);
                                            if (!ev.container)
                                              return;
                                            DBG_PRIVATE ("ID: " << ev.container->id << " (i3's; X11's - " << ev.container->xwindow_id << ")\n"
                                                   << "name = \"" << ev.container->name << "\"\n"
                                                   << "type = \"" << ev.container->type << "\"\n"
                                                   << "class = \"" << ev.container->window_properties.xclass << "\"\n"
                                                   << "border = \"" << ev.container->border_raw << "\"\n"
                                                   << "current_border_width = " << ev.container->current_border_width << "\n"
                                                   << "layout = \"" << ev.container->layout_raw << "\"\n"
                                                   << "percent = " << ev.container->percent << "\n"
                                                   << ((ev.container->urgent)? "urgent" : "")
                                                   << ((ev.container->focused)? "focused" : ""));
                                            if (ev.type == i3ipc::WindowEventType::FOCUS)
                                              {
                                                uint64_t id = ev.container->id;
                                                winSetLastFocused(id);
                                                DBG_PRIVATE ("Switched to #" << ev.container->id << " - " << ev.container->name << " - " << ev.container->window_properties.window_role << std::endl);
                                              }
                                            else if (ev.type == i3ipc::WindowEventType::NEW && ev.container->floating == i3ipc::FloatingMode::AUTO_OFF && ev.container->geometry.width < maxAutoFloatW && ev.container->geometry.height < maxAutoFloatH)
                                              {
                                                DBG_WINDOW ("auto-floating container " << ev.container->id << " of mode " << (int)ev.container->floating);
                                                sendCommandConID (ev.container->id, "floating enable");
                                              }
                                            else if (ev.type == i3ipc::WindowEventType::CLOSE)
                                              {
                                                DBG_WINDOW ("Closed " << ev.container->id);
                                                winRemoveIfExists (ev.container->id);
                                              }
                                            else if (ev.type == i3ipc::WindowEventType::MOVE)
                                              {
                                                DBG_WINDOW ("Setting to workspace " << currentWorkspace);
                                                winSetWorkspace (ev.container->id, currentWorkspaceNum);
                                              }
                                        };

      // Handler of workspace_event
      conn.on_workspace_event = [this] (const i3ipc::workspace_event_t &ev)
                                           {
                                               DBG_WORKSPACE ("workspace_event: " << (char) ev.type << " number: " << ev.current->num);
                                               if (ev.type != i3ipc::WorkspaceEventType::FOCUS)
                                                 return;

                                               DBG_WORKSPACE ("Switched from " << ev.old->num);
                                               if (ev.current->num == currentWorkspaceNum)
                                                 {
                                                   DBG_WORKSPACE ("Duplicate workspace event");
                                                   return;
                                                 }
                                               currentWorkspace = ev.current->name;
                                               currentWorkspaceNum = ev.current->num;
                                               currentOutput = ev.current->output;
//                                               if (idsMap.size () > 1 && idsMap.rbegin ()->first == ev.old->num)
//                                                 idsMap.rbegin ()->first = currentWorkspaceNum;

                                               workspaceMap[currentWorkspaceNum] = currentWorkspace;
                                               outputMap[currentOutput] = currentWorkspaceNum;
                                           };

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
      while (true)
        {
          conn.handle_event ();
        }
#pragma clang diagnostic pop
    }
}
