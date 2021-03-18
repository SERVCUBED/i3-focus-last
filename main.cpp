#include <iostream>
#include <fcntl.h>
#include <cstring>
#include "background.h"

using namespace i3_focus_last;

void handleCheckAlreadyRunning(int argc, char *argv[])
{
  DEBUG_MSG ("Starting");
  bool iscmd = likely (argc > 1 && (argv[1][0] == 'f' || argv[1][0] == 'd' || argv[1][0] == 'g'));
  DEBUG_MSG ("isCmd: " << iscmd);

  // file exists and is being read from - write cmd
  // cmd mode - file exists and not read from - remove file
  // cmd mode - file not exists - ret

  int fd = open (pipefname, O_WRONLY | O_NONBLOCK);
  if (unlikely (fd == -1))
    {
      if (likely (iscmd))
        {
          ERROR_MSG ("Pipe open fail");
          exit(EXIT_FAILURE);
        }
      DEBUG_MSG ("Pipe open fail");
      return;
    }

  DEBUG_MSG ("Pipe read success");
  std::string cmd;
  if (likely (iscmd))
    {
      for (int i = 1; i < argc; ++i)
        {
          cmd.append(argv[i]);
        }
    }
  else
    cmd.push_back ('e');
  cmd.push_back ('\n');

  DEBUG_MSG ("Writing to pipe");
  ssize_t written = write(fd, cmd.c_str (), cmd.length ());
  close (fd);
  if (!iscmd || written == -1) // && errno == EPIPE) // Pipe error
    {
      DEBUG_MSG ("Unlinking pipe");
      unlink (pipefname);
    }

  DEBUG_MSG ("Written: " << written);
  if (iscmd)
    exit (EXIT_SUCCESS);
}

int main (int argc, char *argv[], char** envp)
{
#if DEBUG
  if (isatty (STDOUT_FILENO) && argc > 1 && argv[1][0] == '-' && argv[1][1] == 'd')
    {
      isDebug = 15u;
      argc--;
      argv++;
    }
#endif
  signal(SIGPIPE, SIG_IGN);
  handleCheckAlreadyRunning (argc, argv);
  signal(SIGPIPE, SIG_DFL);

  DEBUG_MSG ("Starting");
  (new background)->run(argc, argv);
}
