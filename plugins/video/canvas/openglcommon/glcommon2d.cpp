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

#include "sysdef.h"
#include "cssys/sysdriv.h"
#include "cs2d/openglcommon/glcommon2d.h"
#include "cs2d/common/scrshot.h"
#include "csutil/csrect.h"
#include "isystem.h"

IMPLEMENT_IBASE (csGraphics2DGLCommon)
  IMPLEMENTS_INTERFACE (iPlugIn)
  IMPLEMENTS_INTERFACE (iGraphics2D)
IMPLEMENT_IBASE_END

csGraphics2DGLCommon::csGraphics2DGLCommon (iBase *iParent) :
  csGraphics2D (),
  LocalFontServer (NULL)
{
  CONSTRUCT_IBASE (iParent);
}

bool csGraphics2DGLCommon::Initialize (iSystem *pSystem)
{
  if (!csGraphics2D::Initialize (pSystem))
    return false;

  // We don't really care about pixel format, except for ScreenShot ()
#if defined (CS_BIG_ENDIAN)
  pfmt.RedMask   = 0xff000000;
  pfmt.GreenMask = 0x00ff0000;
  pfmt.BlueMask  = 0x0000ff00;
#else
  pfmt.RedMask   = 0x000000ff;
  pfmt.GreenMask = 0x0000ff00;
  pfmt.BlueMask  = 0x00ff0000;
#endif
  pfmt.PixelBytes = 4;
  pfmt.PalEntries = 0;
  pfmt.complete ();

  return true;
}

csGraphics2DGLCommon::~csGraphics2DGLCommon ()
{
  Close ();
}

bool csGraphics2DGLCommon::Open (const char *Title)
{
  if (glGetString (GL_RENDERER))
    CsPrintf (MSG_INITIALIZATION, "OpenGL renderer %s ", glGetString (GL_RENDERER));
  if (glGetString (GL_VERSION))
    CsPrintf (MSG_INITIALIZATION, "Version %s", glGetString(GL_VERSION));
  CsPrintf (MSG_INITIALIZATION, "\n");

  if (!csGraphics2D::Open (Title))
    return false;

  // load font 'server'
  if (LocalFontServer == NULL)
  {
    LocalFontServer = new csGraphics2DOpenGLFontServer (8);
    for (int fontindex = 0; fontindex < 8; fontindex++)
      LocalFontServer->AddFont (FontList [fontindex]);
  }

  Clear (0);
  return true;
}

void csGraphics2DGLCommon::Close(void)
{
  csGraphics2D::Close ();
  CHK (delete LocalFontServer);
  LocalFontServer = NULL;
}

void csGraphics2DGLCommon::Clear (int color)
{
  float r, g, b;
  switch (pfmt.PixelBytes)
  {
    case 1: // paletted colors
      r = float (Palette [color].red  ) / 255;
      g = float (Palette [color].green) / 255;
      b = float (Palette [color].blue ) / 255;
      break;
    case 2: // 16bit color
    case 4: // truecolor
      r = float (color & pfmt.RedMask  ) / pfmt.RedMask;
      g = float (color & pfmt.GreenMask) / pfmt.GreenMask;
      b = float (color & pfmt.BlueMask ) / pfmt.BlueMask;
      break;
    default:
      return;
  }
  glClearColor (r, g, b, 0.0);
  glClear (GL_COLOR_BUFFER_BIT);
}

void csGraphics2DGLCommon::SetRGB (int i, int r, int g, int b)
{
  csGraphics2D::SetRGB (i, r, g, b);
}

void csGraphics2DGLCommon::setGLColorfromint (int color)
{
  switch (pfmt.PixelBytes)
  {
    case 1: // paletted colors
      glColor3i (Palette [color].red, Palette [color].green, Palette [color].blue);
      break;
    case 2: // 16bit color
    case 4: // truecolor
      glColor3f (
        float (color & pfmt.RedMask  ) / pfmt.RedMask,
        float (color & pfmt.GreenMask) / pfmt.GreenMask,
        float (color & pfmt.BlueMask ) / pfmt.BlueMask);
      break;
    default:
      return;
  }
}

void csGraphics2DGLCommon::DrawLine (
  float x1, float y1, float x2, float y2, int color)
{
  // prepare for 2D drawing--so we need no fancy GL effects!
  glDisable (GL_TEXTURE_2D);
  glDisable (GL_BLEND);
  glDisable (GL_DEPTH_TEST);
  setGLColorfromint (color);

  glBegin (GL_LINES);
  glVertex2i (GLint (x1), GLint (Height - y1 - 1));
  glVertex2i (GLint (x2), GLint (Height - y2 - 1));
  glEnd ();
}

void csGraphics2DGLCommon::DrawBox (int x, int y, int w, int h, int color)
{
  // prepare for 2D drawing--so we need no fancy GL effects!
  glDisable (GL_TEXTURE_2D);
  glDisable (GL_BLEND);
  glDisable (GL_DEPTH_TEST);
  setGLColorfromint (color);

  glBegin (GL_QUADS);
  glVertex2i (x, Height - y - 1);
  glVertex2i (x + w - 1, Height - y - 1);
  glVertex2i (x + w - 1, Height - (y + h - 1) - 1);
  glVertex2i (x, Height - (y + h - 1) - 1);
  glEnd ();
}

void csGraphics2DGLCommon::DrawPixel (int x, int y, int color)
{
  // prepare for 2D drawing--so we need no fancy GL effects!
  glDisable (GL_TEXTURE_2D);
  glDisable (GL_BLEND);
  glDisable (GL_DEPTH_TEST);
  setGLColorfromint(color);

  glBegin (GL_POINTS);
  glVertex2i (x, Height - y - 1);
  glEnd ();
}

void csGraphics2DGLCommon::WriteChar (int x, int y, int fg, int /*bg*/, char c)
{
  // prepare for 2D drawing--so we need no fancy GL effects!
  glDisable (GL_TEXTURE_2D);
  glDisable (GL_BLEND);
  glDisable (GL_DEPTH_TEST);
  
  setGLColorfromint(fg);

  // in fact the WriteCharacter() method properly shifts over
  // the current modelview transform on each call, so that characters
  // are drawn left-to-write.  But we bypass that because we know the
  // exact x,y location of each letter.  We manipulate the transform
  // directly, so any shift in WriteCharacter() is effectively ignored
  // due to the Push/PopMatrix calls

  glPushMatrix();
  glTranslatef (x, Height - y - FontList [Font].Height,0.0);

  LocalFontServer->WriteCharacter(c, Font);
  glPopMatrix ();
}

void csGraphics2DGLCommon::DrawPixmap (iTextureHandle *hTex,
  int sx, int sy, int sw, int sh, int tx, int ty, int tw, int th)
{
  // cache the texture if we haven't already.
  GLuint texturehandle = (GLuint)hTex->GetMipMapData (0);

  // as we are drawing in 2D, we disable some of the commonly used features
  // for fancy 3D drawing
  glShadeModel (GL_FLAT);
  glDisable (GL_DEPTH_TEST);
  glDepthMask (GL_FALSE);

  // if the texture has transparent bits, we have to tweak the
  // OpenGL blend mode so that it handles the transparent pixels correctly
  if (hTex->GetTransparent ())
  {
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  else
    glDisable (GL_BLEND);

  glEnable (GL_TEXTURE_2D);
  glColor4f (1.,1.,1.,1.);
  glBindTexture (GL_TEXTURE_2D, texturehandle);
  
  int bitmapwidth = 0, bitmapheight = 0;
  hTex->GetMipMapDimensions (0, bitmapwidth, bitmapheight);

  // convert texture coords given above to normalized (0-1.0) texture coordinates
  float ntx1,nty1,ntx2,nty2;
  ntx1 = float (tx     ) / bitmapwidth;
  ntx2 = float (tx + tw) / bitmapwidth;
  nty1 = float (ty     ) / bitmapheight;
  nty2 = float (ty + th) / bitmapheight;

  // draw the bitmap
  glBegin (GL_QUADS);
  glTexCoord2f (ntx1, nty1);
  glVertex2i (sx, Height - sy - 1);
  glTexCoord2f (ntx2, nty1);
  glVertex2i (sx + sw, Height - sy - 1);
  glTexCoord2f (ntx2, nty2);
  glVertex2i (sx + sw, Height - sy - sh - 1);
  glTexCoord2f (ntx1, nty2);
  glVertex2i (sx, Height - sy - sh - 1);
  glEnd ();
}

// This variable is usually NULL except when doing a screen shot:
// in this case it is a temporarily allocated buffer for glReadPixels ()
static UByte *screen_shot = NULL;

unsigned char* csGraphics2DGLCommon::GetPixelAt (int x, int y)
{
  return screen_shot ?
    (screen_shot + pfmt.PixelBytes * ((Height - 1 - y) * Width + x)) : NULL;
}

iImage *csGraphics2DGLCommon::ScreenShot ()
{
  if (pfmt.PixelBytes != 1 && pfmt.PixelBytes != 4)
    return NULL;

  int screen_width = Width * pfmt.PixelBytes;
  screen_shot = new UByte [screen_width * Height];
  if (!screen_shot) return NULL;

  //glPixelStore ()?
  glReadPixels (0, 0, Width, Height,
    pfmt.PixelBytes == 1 ? GL_COLOR_INDEX : GL_RGBA,
    GL_UNSIGNED_BYTE, screen_shot);

  csScreenShot *ss = new csScreenShot (this);

  delete [] screen_shot;
  screen_shot = NULL;

  return ss;
}
