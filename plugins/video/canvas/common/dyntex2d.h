/*
    Copyright (C) 2000 by Jorrit Tyberghein
    Written by Samuel Humphreys

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

#ifndef __DYNTEX2D_H__
#define __DYNTEX2D_H__

#include "csutil/scf.h"
#include "video/canvas/common/graph2d.h"

struct iSystem;

class csDynamicTextureSoft2D : public csGraphics2D
{
public:
  DECLARE_IBASE;

  csDynamicTextureSoft2D (iSystem *isys);
  virtual ~csDynamicTextureSoft2D ();

  virtual bool Open (const char *Title);
  virtual void Close ();

  virtual bool BeginDraw () { return (Memory != NULL); }

  virtual void Print (csRect *area = NULL);

  virtual iGraphics2D *CreateOffScreenCanvas (int width, int height, 
     csPixelFormat *ipfmt, void *buffer, RGBPixel *palette, int pal_size);

};

#endif // __DYNTEX2D_H__
