/*
    OS/2 support for Crystal Space 3D library
    Copyright (C) 1998 by Andrew Zabolotny <bit@eltech.ru>

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

#include <stdarg.h>
#include <limits.h>
#include "cssysdef.h"
#include "csutil/scf.h"
#include "csutil/csrect.h"
#include "cssys/csevent.h"
#include "isystem.h"
#include "csdive.h"
#include "libDIVE.h"
#include "libDIVEprv.h"

#if defined (PROC_INTEL)
inline void memsetd (void *dest, unsigned int value, size_t count)
{
  asm
  (	"	cld;"
	"	rep;"
	"	stosl;"
	: : "D" (dest), "c" (count), "a" (value)
  );
}
#endif

//------------------------------------------------------ csGraphics2DOS2DIVE ---

IMPLEMENT_FACTORY (csGraphics2DOS2DIVE)

EXPORT_CLASS_TABLE (csdive)
  EXPORT_CLASS (csGraphics2DOS2DIVE, "crystalspace.graphics2d.dive",
    "OS/2 DIVE 2D graphics driver for Crystal Space")
EXPORT_CLASS_TABLE_END

IMPLEMENT_IBASE (csGraphics2DOS2DIVE)
  IMPLEMENTS_INTERFACE (iPlugIn)
  IMPLEMENTS_INTERFACE (iGraphics2D)
IMPLEMENT_IBASE_END

csGraphics2DOS2DIVE::csGraphics2DOS2DIVE (iBase *iParent) :
  csGraphics2D (),
  dblbuff (true), HardwareCursor (true),
  WindowX (INT_MIN), WindowY (INT_MIN),
  WindowWidth (-1), WindowHeight (-1)
{
  CONSTRUCT_IBASE (iParent);
  // Initialize module handle
#ifdef CS_STATIC_LINKED
  gdMH = NULLHANDLE;
#else
  extern unsigned long dll_handle;
  gdMH = dll_handle;
#endif

  OS2System = NULL;
}

csGraphics2DOS2DIVE::~csGraphics2DOS2DIVE ()
{
  Close ();
  // Deallocate DIVE resources
  gdDiveDeinitialize ();
  if (OS2System)
    OS2System->DecRef ();
}

bool csGraphics2DOS2DIVE::Initialize (iSystem* pSystem)
{
  if (!csGraphics2D::Initialize (pSystem))
    return false;

  // Initialize DIVE
  if (!gdDiveInitialize ())
  {
    CsPrintf (MSG_FATAL_ERROR, "Unable to initialize DIVE\n");
    return false;
  }

  OS2System = QUERY_INTERFACE (pSystem, iOS2SystemDriver);
  if (!OS2System)
  {
    CsPrintf (MSG_FATAL_ERROR, "The system driver does not support the iOS2SystemDriver interface\n");
    return false;
  }

  return true;
}

bool csGraphics2DOS2DIVE::Open (const char *Title)
{
  if (!csGraphics2D::Open (Title))
    return false;

  // Query settings from system driver
  OS2System->GetExtSettings (WindowX, WindowY, WindowWidth, WindowHeight, HardwareCursor);

  // Set up FGVideoMode
  // Check first all available 16-bit modes for 'native' flag
  // if so, use it. Otherwise choose R5G6B5 format.
  PixelFormat = FOURCC_LUT8;
  switch (Depth)
  {
    case 32:
      for (int i = 0; i < (int)vmCount; i++)
        if (vmList[i].PixelFormat == FOURCC_RGB4
         || vmList[i].PixelFormat == FOURCC_BGR4)
        {
          if (vmList[i].Flags & vmfNative)
          {
            PixelFormat = vmList[i].PixelFormat;
            break;
          }

          if (PixelFormat == FOURCC_LUT8)
            PixelFormat = vmList[i].PixelFormat;
        } /* endif */
      if (PixelFormat != FOURCC_LUT8)
        break;
#if 0
    // When Crystal Space (hopefully) oneday will support
    // 24 bpp modes, enable this branch
    case 24:
      for (int i = 0; i < (int)vmCount; i++)
        if (vmList[i].PixelFormat == FOURCC_RGB3
         || vmList[i].PixelFormat == FOURCC_BGR3)
        {
          if (vmList[i].Flags & vmfNative)
          {
            PixelFormat = vmList[i].PixelFormat;
            break;
          }

          if (PixelFormat == FOURCC_LUT8)
            PixelFormat = vmList[i].PixelFormat;
        } /* endif */
      if (PixelFormat != FOURCC_LUT8)
        break;
#endif
    case 16:
      for (int i = 0; i < (int)vmCount; i++)
      {
        if (vmList[i].PixelFormat == FOURCC_R565
         || vmList[i].PixelFormat == FOURCC_R664
         || vmList[i].PixelFormat == FOURCC_R555)
        {
          if (vmList[i].Flags & vmfNative)
          {
            PixelFormat = vmList[i].PixelFormat;
            break;
          }

          if ((vmList[i].PixelFormat == FOURCC_R565)
           || ((vmList[i].PixelFormat == FOURCC_R664) && (PixelFormat != FOURCC_R565))
           || (PixelFormat == FOURCC_LUT8))
            PixelFormat = vmList[i].PixelFormat;
        } /* endif */
      } /* endfor */
      if (PixelFormat != FOURCC_LUT8)
        break;
    case 15:
      for (int i = 0; i < (int)vmCount; i++)
        if (vmList[i].PixelFormat == FOURCC_R555)
        {
          PixelFormat = vmList[i].PixelFormat;
          break;
        }
      if (PixelFormat != FOURCC_LUT8)
        break;
    case 8:
      break;
    default:
      CsPrintf (MSG_FATAL_ERROR, "ERROR: %d bits per pixel modes not supported\n", Depth);
      break;
  } /* endswitch */

  switch (PixelFormat)
  {
    case FOURCC_LUT8:
      pfmt.PalEntries = 256;
      pfmt.PixelBytes = 1;
      pfmt.RedMask    = 0xff;
      pfmt.GreenMask  = 0xff;
      pfmt.BlueMask   = 0xff;
      break;
    case FOURCC_R565:
      pfmt.PalEntries = 0;
      pfmt.PixelBytes = 2;
      pfmt.RedMask    = 0xf800;
      pfmt.GreenMask  = 0x07e0;
      pfmt.BlueMask   = 0x001f;
      break;
    case FOURCC_R664:
      pfmt.PalEntries = 0;
      pfmt.PixelBytes = 2;
      pfmt.RedMask    = 0xfc00;
      pfmt.GreenMask  = 0x03f0;
      pfmt.BlueMask   = 0x000f;
      break;
    case FOURCC_R555:
      pfmt.PalEntries = 0;
      pfmt.PixelBytes = 2;
      pfmt.RedMask    = 0x7c00;
      pfmt.GreenMask  = 0x03e0;
      pfmt.BlueMask   = 0x001f;
      break;
    case FOURCC_RGB3:
      pfmt.PalEntries = 0;
      pfmt.PixelBytes = 3;
      pfmt.RedMask    = 0x000000ff;
      pfmt.GreenMask  = 0x0000ff00;
      pfmt.BlueMask   = 0x00ff0000;
      break;
    case FOURCC_BGR3:
      pfmt.PalEntries = 0;
      pfmt.PixelBytes = 3;
      pfmt.RedMask    = 0x00ff0000;
      pfmt.GreenMask  = 0x0000ff00;
      pfmt.BlueMask   = 0x000000ff;
      break;
    case FOURCC_RGB4:
      pfmt.PalEntries = 0;
      pfmt.PixelBytes = 4;
      pfmt.RedMask    = 0x000000ff;
      pfmt.GreenMask  = 0x0000ff00;
      pfmt.BlueMask   = 0x00ff0000;
      break;
    case FOURCC_BGR4:
      pfmt.PalEntries = 0;
      pfmt.PixelBytes = 4;
      pfmt.RedMask    = 0x00ff0000;
      pfmt.GreenMask  = 0x0000ff00;
      pfmt.BlueMask   = 0x000000ff;
      break;
  } /* endswitch */
  pfmt.complete ();

  if ((Depth >> 3) != pfmt.PixelBytes)
  {
    CsPrintf (MSG_WARNING, "WARNING: %d bpp mode requested, but not available: using %d bpp mode\n",
      Depth, pfmt.PixelBytes << 3);
    Depth = pfmt.PixelBytes << 3;
  }

  PMrq rq;
  u_int rc;
  FGVideoMode Mode = // selected mode with double buffering
  { Width, Height, PixelFormat, 2, vmfWindowed };

  CsPrintf (MSG_INITIALIZATION, "Using %c%c%c%c pixel format\n",
    PixelFormat, PixelFormat >> 8, PixelFormat >> 16, PixelFormat >> 24);

  // Create PM window
  rq.Parm.CreateWindow.Title = Title;
  if ((rc = PMcall (pmcmdCreateWindow, &rq)) != pmrcOK)
  {
    CsPrintf (MSG_FATAL_ERROR, "Cannot create PM window: no resources bound to executable?\n");
    return false;
  }
  WinHandle = rq.Parm.CreateWindow.Handle;

  // Create DIVE contect
  rq.Parm.CreateCtx.Mode = &Mode;
  if ((rc = PMcall (pmcmdCreateDIVEctx, &rq)) != pmrcOK)
  {
    CsPrintf (MSG_FATAL_ERROR, "Cannot create DIVE context\n");
    return false;
  }

  dW = rq.Parm.CreateCtx.dW;

  // Setup event handlers
  dW->SetKeyboardHandler (KeyboardHandlerStub, this);
  dW->SetMouseHandler (MouseHandlerStub, this);
  dW->SetTerminateHandler (TerminateHandlerStub, this);
  dW->SetFocusHandler (FocusHandlerStub, this);

  // Bind DIVE context to window
  rq.Parm.BindCtx.dW = dW;
  rq.Parm.BindCtx.Handle = WinHandle;
  rq.Parm.BindCtx.DesktopW = DesktopW;
  rq.Parm.BindCtx.DesktopH = DesktopH;
  if ((rc = PMcall (pmcmdBindDIVEctx, &rq)) != pmrcOK)
  {
    CsPrintf (MSG_FATAL_ERROR, "Cannot bind DIVE context to window!\n");
    return false;
  }

  if ((WindowWidth != -1) && (WindowHeight != -1))
  {
    rq.Parm.Resize.dW = dW;
    rq.Parm.Resize.Width = WindowWidth;
    rq.Parm.Resize.Height = WindowHeight;
    rq.Parm.Resize.Center = true;
    PMcall (pmcmdResizeWindow, &rq);
  }
  if ((WindowX != INT_MIN) || (WindowY != INT_MIN))
  {
    rq.Parm.Locate.dW = dW;
    if (WindowX != INT_MIN)
      rq.Parm.Locate.x = ((DesktopW * WindowX) / 100) & ~1;
    else
      rq.Parm.Locate.x = INT_MIN;
    if (WindowY != INT_MIN)
      rq.Parm.Locate.y = (DesktopH * WindowY) / 100;
    else
      rq.Parm.Locate.y = INT_MIN;
    PMcall (pmcmdLocateWindow, &rq);
  }

  if (FullScreen)
    dW->Command (cmdFullScreen);

  // Update drawing routine addresses
  switch (pfmt.PixelBytes)
  {
    case 1:
      break;
    case 2:
      _DrawPixel = DrawPixel16;
      _WriteChar = WriteChar16;
      _GetPixelAt = GetPixelAt16;
      break;
    case 4:
      _DrawPixel = DrawPixel32;
      _WriteChar = WriteChar32;
      _GetPixelAt = GetPixelAt32;
      break;
    default:
      CsPrintf (MSG_WARNING, "WARNING: No 2D routines for selected mode!\n");
      break;
  } /* endif */

  // Show window
  rq.Parm.ShowWin.dW = dW;
  rq.Parm.ShowWin.State = 1;
  if ((rc = PMcall (pmcmdShowWindow, &rq)) != pmrcOK)
    return false;

  UpdatePalette = FALSE;
  return true;
}

void csGraphics2DOS2DIVE::Close (void)
{
  PMrq rq;

  if (!dW)
    return;

  // Destroy DIVE context
  rq.Parm.DestroyCtx.dW = dW;
  PMcall (pmcmdDestroyDIVEctx, &rq);

  // Destroy PM window
  rq.Parm.DestroyWindow.Handle = WinHandle;
  PMcall (pmcmdDestroyWindow, &rq);

  dW = NULL;
}

void csGraphics2DOS2DIVE::Print (csRect *area)
{
  long switchmode = dblbuff ? DIVE_NEXTBUFFER : 0;
  // If we're in double-buffering mode, wait previous frame to complete
  if (dblbuff)
    dW->WaitSwitch ();

  // Now switch to next buffer
  if (area)
  {
    // EXPERIMENTAL FEATURE: although DIVE docs states that SetupBlitter
    // is a longplay operation, on some (all?) videocards it is still
    // faster to do a SetupBlitter() with a small area (that should be redrawn)
    // followed by a blit rather than blitting constantly entire image.

    RECTL rect;
    rect.xLeft = area->xmin;
    rect.yBottom = dW->BufferHeight () - 1 - area->ymax;
    rect.xRight = area->xmax + 1;
    rect.yTop = dW->BufferHeight () - 1 - area->ymin + 1;

    // re-scale
    float AspectX = (float)dW->WindowWidth () / (float)dW->BufferWidth ();
    float AspectY = (float)dW->WindowHeight () / (float)dW->BufferHeight ();
    rect.xLeft = (int) (rect.xLeft * AspectX);
    rect.xRight = (int) (rect.xRight * AspectX);
    rect.yBottom = (int) (rect.yBottom * AspectY);
    rect.yTop = (int) (rect.yTop * AspectY);
    // Increment top & right margin by one pixel to cover calculation errors
    rect.xRight++; rect.yTop++;
    dW->Switch (switchmode, &rect);
  }
  else
    dW->Switch (switchmode);

  // If we're in single-buffered mode, wait right now for buffer to be printed
  if (!dblbuff)
    dW->WaitSwitch ();
}

int csGraphics2DOS2DIVE::GetPage ()
{
  return dW->ActiveBuff ();
}

bool csGraphics2DOS2DIVE::DoubleBuffer (bool Enable)
{
  dblbuff = Enable;
  return true;
}

bool csGraphics2DOS2DIVE::GetDoubleBufferState ()
{
  return dblbuff;
}

void csGraphics2DOS2DIVE::Clear (int color)
{
  switch (pfmt.PixelBytes)
  {
    case 1:
      color &= 0xff;
      color |= (color << 8) | (color << 16) | (color << 24);
      break;
    case 2:
      color &= 0xffff;
      color |= (color << 16);
      break;
  }
  memsetd (Memory, color, (Width * Height * pfmt.PixelBytes) >> 2);
}

void csGraphics2DOS2DIVE::SetRGB (int i, int r, int g, int b)
{
  // set a rgb color in the palette of your graphic interface
  if (i < 0 && i > 255)
    return;
  DivePalette[i] = b | g << 8 | r << 16;
  UpdatePalette = TRUE;
  csGraphics2D::SetRGB (i, r, g, b);
}

bool csGraphics2DOS2DIVE::BeginDraw ()
{
  csGraphics2D::BeginDraw ();
  if (FrameBufferLocked != 1)
    return true;

  ULONG bpl;
  long switchmode = dblbuff ? DIVE_NEXTBUFFER : 0;

  Memory = dW->BeginPaint (&bpl, switchmode);
  // if paused, return false
  if (Memory == NULL)
  {
    if (System->GetShutdown ())
      dW->Pause (FALSE);
    csGraphics2D::FinishDraw ();
    return false;
  } /* endif */

  if (bpl != LineAddressFrameW)
  {
    int i,addr;
    for (i = 0, addr = 0; i < Height; i++, addr += bpl)
      LineAddress [i] = addr;
    LineAddressFrameW = bpl;
  } /* endif */

  if (UpdatePalette && pfmt.PalEntries)
  {
    dW->SetCLUT(DivePalette, pfmt.PalEntries);
    UpdatePalette = FALSE;
  }
  return true;
}

void csGraphics2DOS2DIVE::FinishDraw ()
{
  csGraphics2D::FinishDraw ();
  if (FrameBufferLocked)
    return;

  dW->EndPaint ();
  Memory = NULL;
}

bool csGraphics2DOS2DIVE::SetMousePosition (int x, int y)
{
  int ww = dW->WindowWidth (), wh = dW->WindowHeight ();
  if ((ww <= 0) || (wh <= 0))
    return false;

  POINTL pp;
  pp.x = (x * ww) / Width;
  pp.y = ((Height - 1 - y) * wh) / Height;
  WinMapWindowPoints (dW->diveCL, HWND_DESKTOP, &pp, 1);

  return WinSetPointerPos (HWND_DESKTOP, pp.x, pp.y);
}

bool csGraphics2DOS2DIVE::SetMouseCursor (csMouseCursorID iShape)
{
  if (!HardwareCursor)
  {
    dW->MouseVisible (FALSE);
    return false;
  } /* endif */

  switch (iShape)
  {
    case csmcNone:
      dW->MouseVisible (FALSE);
      return true;
    case csmcArrow:
      dW->MouseCursor (SPTR_ARROW);
      dW->MouseVisible (TRUE);
      return true;
    case csmcLens:
    case csmcCross:
    case csmcPen:
      dW->MouseVisible (FALSE);
      return false;
    case csmcMove:
      dW->MouseCursor (SPTR_MOVE);
      dW->MouseVisible (TRUE);
      return true;
    case csmcSizeNWSE:
      dW->MouseCursor (SPTR_SIZENWSE);
      dW->MouseVisible (TRUE);
      return true;
    case csmcSizeNESW:
      dW->MouseCursor (SPTR_SIZENESW);
      dW->MouseVisible (TRUE);
      return true;
    case csmcSizeNS:
      dW->MouseCursor (SPTR_SIZENS);
      dW->MouseVisible (TRUE);
      return true;
    case csmcSizeEW:
      dW->MouseCursor (SPTR_SIZEWE);
      dW->MouseVisible (TRUE);
      return true;
    case csmcStop:
      dW->MouseCursor (SPTR_ILLEGAL);
      dW->MouseVisible (TRUE);
      return true;
    case csmcWait:
      dW->MouseCursor (SPTR_WAIT);
      dW->MouseVisible (TRUE);
      return true;
    default:
      dW->MouseVisible (FALSE);
      return false;
  } /* endswitch */
}

void csGraphics2DOS2DIVE::MouseHandlerStub (void *Self, int Button, bool Down,
  int x, int y, int /*ShiftFlags*/)
{
  csGraphics2DOS2DIVE *This = (csGraphics2DOS2DIVE *)Self;
  if (!This)
    return;
  int ww = This->dW->WindowWidth (), wh = This->dW->WindowHeight ();
  if ((ww <= 0) || (wh <= 0))
    return;

  x = (x * This->Width) / ww;
  y = ((wh - 1 - y) * This->Height) / wh;

  This->System->QueueMouseEvent (Button, Down, x, y);
}

void csGraphics2DOS2DIVE::KeyboardHandlerStub (void *Self, unsigned char ScanCode,
  bool Down, unsigned char RepeatCount, int ShiftFlags)
{
  csGraphics2DOS2DIVE *This = (csGraphics2DOS2DIVE *)Self;
  if (!This)
    return;
  This->OS2System->KeyboardEvent (ScanCode, Down);
}

void csGraphics2DOS2DIVE::FocusHandlerStub (void *Self, bool Enable)
{
  csGraphics2DOS2DIVE *This = (csGraphics2DOS2DIVE *)Self;
  This->System->QueueFocusEvent (Enable);
}

void csGraphics2DOS2DIVE::TerminateHandlerStub (void *Self)
{
  csGraphics2DOS2DIVE *This = (csGraphics2DOS2DIVE *)Self;
  This->System->StartShutdown ();
}
