/*
    Copyright (C) 2003 by Jorrit Tyberghein
	      (C) 2003 by Frank Richter

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

#include "cssysdef.h"

#include "ivideo/graph3d.h"
#include "ivideo/graph2d.h"
#include "iutil/document.h"
#include "iutil/objreg.h"
#include "iengine/engine.h"
#include "iengine/material.h"
#include "iengine/texture.h"
#include "imap/loader.h"

#include "ptanimimg.h"

// Plugin stuff

SCF_IMPLEMENT_IBASE(csAnimateProctexLoader);
  SCF_IMPLEMENTS_INTERFACE(iLoaderPlugin);
  SCF_IMPLEMENTS_INTERFACE(iComponent);
SCF_IMPLEMENT_IBASE_END;

CS_IMPLEMENT_PLUGIN

SCF_IMPLEMENT_FACTORY(csAnimateProctexLoader);

SCF_EXPORT_CLASS_TABLE (ptanimimg)
  SCF_EXPORT_CLASS_DEP (csAnimateProctexLoader, 
    "crystalspace.proctex.loader.animimg",
    "Animated image procedural texture loader", 
    "crystalspace.graphics3d., crystalspace.level.loader, ")
SCF_EXPORT_CLASS_TABLE_END

//----------------------------------------------------------------------------

csAnimateProctexLoader::csAnimateProctexLoader (iBase *p)
{
  SCF_CONSTRUCT_IBASE (p);
}

csAnimateProctexLoader::~csAnimateProctexLoader ()
{
}

bool csAnimateProctexLoader::Initialize(iObjectRegistry *object_reg)
{
  csAnimateProctexLoader::object_reg = object_reg;

  return true;
}

csPtr<iBase> csAnimateProctexLoader::Parse (iDocumentNode* node, 
					    iLoaderContext* ldr_context,
  					    iBase* context)
{
  if (!node) return NULL;

  csRef<iDocumentNode> file = node->GetNode ("file");
  if (!file) return NULL;
  if (!file->GetContentsValue()) return NULL;

  csRef<iLoader> LevelLoader = CS_QUERY_REGISTRY (object_reg, iLoader);
  if (!LevelLoader) return NULL;

  csRef<iImage> img = LevelLoader->LoadImage (file->GetContentsValue (),
    CS_IMGFMT_TRUECOLOR | CS_IMGFMT_ALPHA);
  if (!img) return NULL;

  csRef<csProcTexture> pt = csPtr<csProcTexture> (new csProcAnimated (img));
  if (pt->Initialize (object_reg))
  {
    csRef<iTextureWrapper> tw = pt->GetTextureWrapper ();
    return csPtr<iBase> (tw);
  }
  else
  {
    return NULL;
  }
}

//----------------------------------------------------------------------------

csProcAnimated::csProcAnimated (iImage* img)
{
  image = img;
  animation = csPtr<iAnimatedImage>
    (SCF_QUERY_INTERFACE (image, iAnimatedImage));

  mat_w = image->GetWidth ();
  mat_h = image->GetHeight ();

  texFlags = CS_TEXTURE_3D | CS_TEXTURE_NOMIPMAPS;

  last_time = (csTicks)-1;
}

csProcAnimated::~csProcAnimated ()
{
}

bool csProcAnimated::PrepareAnim ()
{
  if (anim_prepared) return true;
  if (!csProcTexture::PrepareAnim ()) return false;
  return true;
}

void csProcAnimated::Animate (csTicks current_time)
{
  bool first = (last_time == (csTicks)-1);
  
  if (first || animation)
  {
    if (first || animation->Animate (last_time - current_time))
    {
      last_time = current_time;
      g3d->SetRenderTarget (tex->GetTextureHandle (), true);
      if (!g3d->BeginDraw (CSDRAW_2DGRAPHICS)) return;
      g2d->Blit (0, 0, mat_w, mat_h, 
	(unsigned char*)image->GetImageData());
      g3d->FinishDraw ();
    }
  }
}

