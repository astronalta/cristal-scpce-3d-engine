/*
    Copyright (C) 1998 by Jorrit Tyberghein

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "sysdef.h"
#include "csutil/inifile.h"
#include "cssys/unix/unix.h"
#include "cssys/system.h"
#include "igraph3d.h"

/*
 * Signal handler to clean up and give some
 * final debugging information.
 */
extern void debug_dump ();
extern void cleanup ();
void handler (int sig)
{
  static bool in_exit = false;
  if (in_exit)
    exit (1);
  in_exit = true;

  int err = errno;
#if defined (__USE_GNU)
  fprintf (stderr, "\n%s signal caught; last error: %s\n",
    strsignal (sig), strerror (err));
#elif defined (OS_LINUX) || defined (OS_FREEBSD)
  fprintf (stderr, "\n%s signal caught; last error: %s\n",
    sys_siglist [sig], strerror (err));
#else
  fprintf (stderr, "\nSignal %d caught; last error: %s\n",
    sig, strerror (err));
#endif

  if (sig != SIGINT)
    debug_dump ();

  SysSystemDriver::Shutdown = true;
  cleanup ();
  exit (1);
}

void init_sig ()
{
#ifndef DO_COREDUMP
  signal (SIGHUP, handler);
  signal (SIGINT, handler);
  signal (SIGTRAP, handler);
  signal (SIGABRT, handler);
  signal (SIGALRM, handler);
  signal (SIGTERM, handler);
  signal (SIGPIPE, handler);
  signal (SIGSEGV, handler);
  signal (SIGBUS, handler);
  signal (SIGFPE, handler);
  signal (SIGILL, handler);
#endif // ! DO_COREDUMP
}

//------------------------------------------------------ The System driver ---//

SysSystemDriver::SysSystemDriver () : csSystemDriver ()
{
  // Initialize signal handler for clean shutdown
  init_sig ();
}

void *SysSystemDriver::QueryInterface (const char *iInterfaceID, int iVersion)
{
  IMPLEMENTS_INTERFACE_COMMON (iUnixSystemDriver, (iUnixSystemDriver *)this)
  return csSystemDriver::QueryInterface (iInterfaceID, iVersion);
}

void SysSystemDriver::SetSystemDefaults (csIniFile *config)
{
  csSystemDriver::SetSystemDefaults (config);
  SimDepth = Config->GetInt ("VideoDriver", "SIMULATE_DEPTH", 0);
  UseSHM = Config->GetYesNo ("VideoDriver", "XSHM", true);
  HardwareCursor = Config->GetYesNo ("VideoDriver", "SystemMouseCursor", true);

  const char *val;
  if ((val = GetOptionCL ("shm")))
    UseSHM = true;

  if ((val = GetOptionCL ("noshm")))
    UseSHM = false;

  if ((val = GetOptionCL ("sdepth")))
  {
    SimDepth = atol (val);
    if (SimDepth != 8 && SimDepth != 15 && SimDepth != 16 && SimDepth != 32)
    {
      Printf (MSG_FATAL_ERROR, "Crystal Space can't run in this simulated depth! (use 8, 15, 16, or 32)!\n");
      exit (0);
    }
  }
}

void SysSystemDriver::Help ()
{
  csSystemDriver::Help ();
  Printf (MSG_STDOUT, "  -sdepth=<depth>    set simulated depth (8, 15, 16, or 32) (default=none)\n");
  Printf (MSG_STDOUT, "  -shm/noshm         SHM extension (default '%sshm')\n",
    UseSHM ? "" : "no");
}

void SysSystemDriver::Loop(void)
{
  while (!Shutdown && !ExitLoop)
  {
    static long prev_time = -1;
    long new_prev_time = Time ();
    NextFrame ((prev_time == -1) ? 0 : new_prev_time - prev_time, Time ());
    prev_time = new_prev_time;
  }
}

void SysSystemDriver::Sleep (int SleepTime)
{
  usleep (SleepTime * 1000);
}

//------------------------------------------------------ XUnixSystemDriver ---//

void SysSystemDriver::GetExtSettings (int &oSimDepth,
  bool &oUseSHM, bool &oHardwareCursor)
{
  oSimDepth = SimDepth;
  oUseSHM = UseSHM;
  oHardwareCursor = HardwareCursor;
}
