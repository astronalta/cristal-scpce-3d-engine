/*
    Copyright (C) 2003 by Jorrit Tyberghein
	      (C) 2003 by Frank Richter

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "cssysdef.h"
#include "iutil/comp.h"
#include "iutil/databuff.h"
#include "iutil/vfs.h"
#include "csutil/scf.h"
#include "xriface.h"

struct csXmlReadDocWrapper : public iDocument
{
private:
  csRef<iDocument> xmlreaddoc;
public:
  SCF_DECLARE_IBASE;

  csXmlReadDocWrapper (csRef<iDocument> doc);
  virtual ~csXmlReadDocWrapper ();

  virtual void Clear ();
  virtual csRef<iDocumentNode> CreateRoot ();
  virtual csRef<iDocumentNode> GetRoot ();
  virtual const char* Parse (iFile* file);
  virtual const char* Parse (iDataBuffer* buf);
  virtual const char* Parse (iString* str);
  virtual const char* Parse (const char* buf);
  virtual const char* Write (iFile* file);
  virtual const char* Write (iString* str);
  virtual const char* Write (iVFS* vfs, const char* filename);

  virtual int Changeable ();
};

SCF_IMPLEMENT_IBASE (csXmlReadDocWrapper)
  SCF_IMPLEMENTS_INTERFACE (iDocument)
SCF_IMPLEMENT_IBASE_END

csXmlReadDocWrapper::csXmlReadDocWrapper (csRef<iDocument> doc)
{
  SCF_CONSTRUCT_IBASE (NULL);

  xmlreaddoc = doc;
}

csXmlReadDocWrapper::~csXmlReadDocWrapper ()
{
}

void csXmlReadDocWrapper::Clear ()
{
  xmlreaddoc->Clear();
}

csRef<iDocumentNode> csXmlReadDocWrapper::CreateRoot ()
{
  return NULL;
}

csRef<iDocumentNode> csXmlReadDocWrapper::GetRoot ()
{
  return xmlreaddoc->GetRoot();
}

const char* csXmlReadDocWrapper::Parse (iFile* file)
{
  char *buf = new char[file->GetSize()];
  file->Read (buf, file->GetSize());
  const char *ret = Parse (buf);
  delete[] buf;
  return ret;
}

const char* csXmlReadDocWrapper::Parse (iDataBuffer* buf)
{
  return Parse ((char*)buf->GetData());
}

const char* csXmlReadDocWrapper::Parse (iString* str)
{
  return Parse ((const char*)str);
}

const char* csXmlReadDocWrapper::Parse (const char* buf)
{
  const char* b = buf;
  while ((*b == ' ') || (*b == '\n') || (*b == '\t') || 
    (*b == '\r')) b++;
  if (*b == '<')
  {
    return xmlreaddoc->Parse (buf);
  }
  else
  {
    return "Data does not seem to be XML.";
  }
}

const char* csXmlReadDocWrapper::Write (iFile*)
{
  return "Writing not supported by this plugin!";
}

const char* csXmlReadDocWrapper::Write (iString*)
{
  return "Writing not supported by this plugin!";
}

const char* csXmlReadDocWrapper::Write (iVFS*, const char*)
{
  return "Writing not supported by this plugin!";
}

int csXmlReadDocWrapper::Changeable ()
{
  return xmlreaddoc->Changeable();
}

class csXmlReadXMLPlugin : public iDocumentSystem, public iComponent
{
private:
  csXmlReadDocumentSystem *xmlread;
public:
  SCF_DECLARE_IBASE;

  csXmlReadXMLPlugin (iBase* parent = NULL);
  virtual ~csXmlReadXMLPlugin ();

  virtual bool Initialize (iObjectRegistry* objreg);

  csRef<iDocument> CreateDocument ();
};

SCF_IMPLEMENT_IBASE(csXmlReadXMLPlugin)
  SCF_IMPLEMENTS_INTERFACE(iDocumentSystem)
  SCF_IMPLEMENTS_INTERFACE(iComponent)
SCF_IMPLEMENT_IBASE_END

csXmlReadXMLPlugin::csXmlReadXMLPlugin(iBase* parent)
{
  SCF_CONSTRUCT_IBASE(parent);

  xmlread = new csXmlReadDocumentSystem ();
}

csXmlReadXMLPlugin::~csXmlReadXMLPlugin()
{
  delete xmlread;
}

bool csXmlReadXMLPlugin::Initialize (iObjectRegistry* objreg)
{
  return true;
}

csRef<iDocument> csXmlReadXMLPlugin::CreateDocument ()
{
  return csPtr<iDocument> (new csXmlReadDocWrapper (
    xmlread->CreateDocument ()));
}

CS_IMPLEMENT_PLUGIN

SCF_IMPLEMENT_FACTORY (csXmlReadXMLPlugin)

SCF_EXPORT_CLASS_TABLE(xmlread)
  SCF_EXPORT_CLASS(csXmlReadXMLPlugin, "crystalspace.documentsystem.xmlread",
      "Crystal Space XmlRead document system")
SCF_EXPORT_CLASS_TABLE_END

