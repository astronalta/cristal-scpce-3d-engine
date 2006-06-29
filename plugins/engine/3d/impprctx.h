/*
    Copyright (C) 2002 by Keith Fulton and Jorrit Tyberghein

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

#ifndef __CS_IMPPRCTX_H__
#define __CS_IMPPRCTX_H__

#include "csutil/ref.h"
#include "csgeom/vector3.h"
#include "iengine/engine.h"
#include "iengine/rview.h"
#include "cstool/procmesh.h"

class csEngine;
class csImposterMesh;

class csImposterProcTex : public scfImplementation0<csImposterProcTex>
{
private:
  csEngine* engine;
  iRenderView* View;
  csImposterMesh *mesh;
  bool imposter_ready;
  csMeshOnTexture* mesh_on_texture;
  csRef<iGraphics3D> g3d;
  csRef<iGraphics2D> g2d;
  iTextureWrapper *tex;

public:
  csImposterProcTex (csEngine* engine, csImposterMesh *parent);
  ~csImposterProcTex ();

  bool GetImposterReady () { return imposter_ready; }
  void SetImposterReady (bool r) { imposter_ready = r; }
  void Animate (iRenderView *rview);
};

#endif // __CS_IMPPRCTX_H__

