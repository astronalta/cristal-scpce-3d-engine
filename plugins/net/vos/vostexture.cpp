/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
   $Id$

    This file is part of Crystal Space Virtual Object System Abstract
    3D Layer plugin (csvosa3dl).

    Copyright (C) 2004 Peter Amstutz

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#undef IMOZILLA_SUPPORT // not commited to CS CVS yet

#include "cssysdef.h"

#include "ivideo/graph3d.h"
#include "ivideo/texture.h"
#include "ivideo/txtmgr.h"
#include "igraphic/imageio.h"
#include "iutil/vfs.h"

#ifdef IMOZILLA_SUPPORT
#include "itexture/imozilla.h"
#endif

#include "vostexture.h"

using namespace VOS;

class ConstructTextureTask : public Task
{
public:
  iObjectRegistry *object_reg;
  std::string texturename;
  std::string texturedata;
  std::string cachefilename;
  vRef<csMetaTexture> metatxt;

  ConstructTextureTask(iObjectRegistry *objreg, const std::string& name,
                         const std::string& cache, csMetaTexture* mt);
  virtual ~ConstructTextureTask();
  virtual void doTask();
};

ConstructTextureTask::ConstructTextureTask(iObjectRegistry *objreg,
                                           const std::string& name,
                                           const std::string& cache,
                                           csMetaTexture* mt)
  : object_reg(objreg), texturename(name), cachefilename(cache),
    metatxt(mt, true)
{
}

ConstructTextureTask::~ConstructTextureTask()
{
}

void ConstructTextureTask::doTask()
{
  csRef<iEngine> engine = CS_QUERY_REGISTRY (object_reg, iEngine);
  csRef<iGraphics3D> g3d = CS_QUERY_REGISTRY (object_reg, iGraphics3D);
  csRef<iTextureManager> txtmgr = g3d->GetTextureManager();
  csRef<iImageIO> io = CS_QUERY_REGISTRY (object_reg, iImageIO);

  if (!io)
  {
    LOG("ConstructTextureTask", 1,
      "Error: Could not get the iLoader plugin from object registry!");
    return;
  }
  if(!txtmgr)
  {
    LOG("ConstructTextureTask", 1, "No texture manager");
    return;
  }

  csRef<iImage> image (io->Load ((uint8*) texturedata.c_str(),
                   texturedata.size(),
                 engine->GetTextureFormat()));

  csRef<iTextureHandle> handle (txtmgr->RegisterTexture (image, CS_TEXTURE_3D));

  csRef<iTextureWrapper> texture = engine->GetTextureList()->NewTexture ( handle);

  if(!texture)
  {
    LOG("ConstructTextureTask", 1,
      "Error: could not create texture (would be in cache file \""
      << cachefilename << "\")!");
    return;
  }

//  texture->Register (txtmgr);
  texture->GetTextureHandle()->Prepare ();

  metatxt->texturewrapper = texture;
}



class ConstructMozTextureTask : public Task
{
public:
  iObjectRegistry *object_reg;
  std::string texturedata;
  vRef<csMetaTexture> metatxt;

  ConstructMozTextureTask(iObjectRegistry *objreg, csMetaTexture* mt);
  virtual ~ConstructMozTextureTask();
  virtual void doTask();
};



ConstructMozTextureTask::ConstructMozTextureTask(iObjectRegistry *objreg,
                                           csMetaTexture* mt)
  : object_reg(objreg), metatxt(mt, true)
{
}

ConstructMozTextureTask::~ConstructMozTextureTask()
{
}

void ConstructMozTextureTask::doTask()
{
#ifdef IMOZILLA_SUPPORT
  csRef<iEngine> engine = CS_QUERY_REGISTRY (object_reg, iEngine);
  csRef<iGraphics3D> g3d = CS_QUERY_REGISTRY (object_reg, iGraphics3D);
  csRef<iTextureManager> txtmgr = g3d->GetTextureManager();
  csRef<iMozilla> moz = CS_QUERY_REGISTRY (object_reg, iMozilla);

  if(!moz) {
    LOG("ConstructMozTextureTask", 1, "No iMozilla plugin");
    return;
  }

  csRef<iTextureFactory> tf = moz->NewFactory();
  tf->SetSize(256, 256);
  csRef<iTextureWrapper> tw = tf->Generate();
  tw->Register(txtmgr);
  tw->GetTextureHandle()->Prepare();

  csRef<iMozillaFactory> mozfac = SCF_QUERY_INTERFACE((iTextureFactory*)tf,
                                                      iMozillaFactory);
  mozfac->Init(256, 256);

  LOG("ConstructMozTextureTask", 1, "Loading " << texturedata);

  mozfac->LoadURI(texturedata.c_str());

  metatxt->texturewrapper = tw;
#endif
}


csMetaTexture::csMetaTexture(VobjectBase* superobject)
    : A3DL::Texture(superobject), alreadyLoaded(false)
{
}

csMetaTexture::~csMetaTexture()
{
}

void csMetaTexture::Setup(csVosA3DL* vosa3dl)
{
  if(alreadyLoaded) return;
  else alreadyLoaded = true;

  vRef<Property> imagedata = getImage();

  if(imagedata->getDataType().substr(0, 5) == "text/") {
    ConstructMozTextureTask* cmtt = new ConstructMozTextureTask(
      vosa3dl->GetObjectRegistry(),
      this);

    cmtt->texturedata = "data:";
    cmtt->texturedata += imagedata->getDataType();
    cmtt->texturedata += ",";
    cmtt->texturedata += imagedata->read();

    vosa3dl->mainThreadTasks.push(cmtt);
  }
  else
  {
    char cachefilename[256];
    vRef<Site> site = imagedata->getSite();
    /*snprintf(cachefilename, sizeof(cachefilename), "/csvosa3dl_cache/%s/%s",
      site->getURL().getHost().c_str(),
      imagedata->getSiteName().c_str());*/
    snprintf(cachefilename, sizeof(cachefilename), "/tmp/%s_%s",
             site->getURL().getHost().c_str(),
             imagedata->getSiteName().c_str());

    // VFS uses ':' as a seperator
    for (int i=0; cachefilename[i]; i++)
    {
      if ((cachefilename[i] == ':'))
        cachefilename[i] = '_';
    }

    ConstructTextureTask* ctt = new ConstructTextureTask(
      vosa3dl->GetObjectRegistry(), getURLstr(),
      cachefilename, this);
    imagedata->read(ctt->texturedata);
    vosa3dl->mainThreadTasks.push(ctt);
  }
  //addChildListener(this);
}

void csMetaTexture::notifyPropertyChange(const PropertyEvent& event)
{
  try
  {
    vRef<ParentChildRelation> pcr = event.getProperty()->findParent(*this);
    if(pcr->getContextualName() == "a3dl:image")
    {
      // XXX reload the image and stuff
    }
  }
  catch(NoSuchObjectError) { }
  catch(AccessControlError) { }
  catch(RemoteError) { }
}

csRef<iTextureWrapper> csMetaTexture::GetTextureWrapper()
{
  return texturewrapper;
}

void csMetaTexture::notifyChildInserted(VobjectEvent& event)
{
  vRef<Property> p = meta_cast<Property>(event.getNewChild());
  if(p.isValid()) p->addPropertyListener(this);
}

void csMetaTexture::notifyChildReplaced(VobjectEvent& event)
{
  notifyChildRemoved(event);
  notifyChildInserted(event);
}

void csMetaTexture::notifyChildRemoved(VobjectEvent& event)
{
  vRef<Property> p = meta_cast<Property>(event.getOldChild());
  if(p.isValid()) p->removePropertyListener(this);
}

MetaObject* csMetaTexture::new_csMetaTexture(VobjectBase* superobject,
                                             const std::string& type)
{
  return new csMetaTexture(superobject);
}
