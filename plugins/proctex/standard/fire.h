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

#ifndef __CS_FIRE_H__
#define __CS_FIRE_H__

#include "cstool/basetexfact.h"
#include "csutil/strhash.h"
#include "csutil/csstring.h"

SCF_DECLARE_FACTORY(csPtFireType);
SCF_DECLARE_FACTORY(csPtFireLoader);

#define CLASSID_FIRETYPE	    "crystalspace.texture.type.fire"
#define CLASSID_FIRELOADER	    "crystalspace.texture.loader.fire"

class csPtFireType : public csBaseProctexType
{
public:
  csPtFireType (iBase *p);

  virtual csPtr<iTextureFactory> NewFactory();
};

class csPtFireFactory : public csBaseTextureFactory
{
public:
  csPtFireFactory (iBase* p, iObjectRegistry* object_reg);

  virtual csPtr<iTextureWrapper> Generate ();
};

class csPtFireLoader : public csBaseProctexLoader
{
  csStringHash tokens;
#define CS_TOKEN_ITEM_FILE "proctex/standard/fire.tok"
#include "cstool/tokenlist.h"
public:
  csPtFireLoader(iBase *p);

  csPtr<iBase> Parse (iDocumentNode* node, iLoaderContext* ldr_context,
  	iBase* context);
};

#endif 
