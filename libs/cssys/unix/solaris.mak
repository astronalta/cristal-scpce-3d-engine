# This is an include file for all the makefiles which describes system specific
# settings. Also have a look at mk/user.mak.

# Friendly names for building environment
DESCRIPTION.solaris = Solaris
DESCRIPTION.OS.solaris = Solaris

ifeq ($(X11.AVAILABLE),yes)
  PLUGINS+=video/canvas/softx
  PLUGINS+=video/canvas/linex
  ifeq ($(GL.AVAILABLE),yes)
    PLUGINS+=video/renderer/opengl video/canvas/openglx
  endif

  # The X-Window plugin
  PLUGINS+=video/canvas/xwindow
  # Shared Memory Plugin
  PLUGINS+=video/canvas/xextshm
  # Video Modes Plugin
  PLUGINS+=video/canvas/xextf86vm
endif

# udp/tcp network plugin
PLUGINS+=net/driver/ensocket

# video support
# formats (this is the wrapping format for the video data)
# Microsofts AVI
PLUGINS+=video/format/avi
# CODECS (some formats are dynamic, that is they need codecs to encod/decode data)
# OpenDivX : you need an additional library you can get from www.projectmayo.com
#PLUGINS+=video/format/codecs/opendivx
#PLUGINS+=video/format/codecs/rle

# Uncomment the following to build sound renderer
#PLUGINS+=sound/renderer/software

#--------------------------------------------------- rootdefines & defines ---#
ifneq (,$(findstring defines,$(MAKESECTION)))

# Processor is autodetected and written to config.mak
#PROC=SPARC

# Operating system. Can be one of:
# MACOSX, SOLARIS, LINUX, IRIX, BSD, UNIX, WIN32
OS=SOLARIS

# Operating system family: UNIX (for Unix or Unix-like platforms), WIN32, etc.
OS_FAMILY=UNIX

# Compiler. Can be one of: GCC, MPWERKS, VC (Visual C++), UNKNOWN
COMP=GCC

endif # ifneq (,$(findstring defines,$(MAKESECTION)))

#----------------------------------------------------------------- defines ---#
ifeq ($(MAKESECTION),defines)

include mk/unix.mak

# Extra libraries needed on this system.
LIBS.EXE=$(LFLAGS.l)dl $(LFLAGS.l)nsl $(LFLAGS.l)m

# Socket library
LIBS.SOCKET.SYSTEM=$(LFLAGS.l)socket

# Where can the optional sound libraries be found on this system?
SOUND_LIBS=

# Indicate where special include files can be found.
CFLAGS.INCLUDE=$(CFLAGS.I)/usr/local/include

# General flags for the compiler which are used in any case.
CFLAGS.GENERAL=$(CFLAGS.SYSTEM) $(CSTHREAD.CFLAGS)

# Flags for the compiler which are used when optimizing.
CFLAGS.optimize=-O -fomit-frame-pointer

# Flags for the compiler which are used when debugging.
CFLAGS.debug=-g3 -gstabs

# Flags for the compiler which are used when profiling.
CFLAGS.profile=-pg -O -g

# Flags for the compiler which are used when building a shared library.
CFLAGS.DLL=

# General flags for the linker which are used in any case.
LFLAGS.GENERAL=$(LFLAGS.L)/usr/local/lib $(CSTHREAD.LFLAGS)

# Flags for the linker which are used when debugging.
LFLAGS.debug=-g3

# Flags for the linker which are used when profiling.
LFLAGS.profile=-pg

# Flags for the linker which are used when building a shared library.
#LFLAGS.DLL=-Wl,-G -nostdlib $(LFLAGS.l)gcc $(LFLAGS.l)c
LFLAGS.DLL=-Wl,-G

# System dependent source files included into CSSYS library
SRC.SYS_CSSYS= $(wildcard libs/cssys/unix/*.cpp) \
  libs/cssys/general/sysroot.cpp \
  libs/cssys/general/findlib.cpp \
  libs/cssys/general/getopt.cpp \
  libs/cssys/general/printf.cpp \
  libs/cssys/general/runloop.cpp \
  libs/cssys/general/sysinit.cpp \
  $(CSTHREAD.SRC)

# Use makedep to build dependencies
DEPEND_TOOL=mkdep

endif # ifeq ($(MAKESECTION),defines)

#-------------------------------------------------------------- confighelp ---#
ifeq ($(MAKESECTION),confighelp)

SYSHELP += \
  $(NEWLINE)echo $"  make solaris      Prepare for building on $(DESCRIPTION.solaris)$"

endif # ifeq ($(MAKESECTION),confighelp)

#--------------------------------------------------------------- configure ---#
ifeq ($(ROOTCONFIG),config)

SYSCONFIG=libs/cssys/unix/unixconf.sh solaris >>config.tmp

endif # ifeq ($(ROOTCONFIG),config)
