/* 
    Copyright (C) 2006 by Seth Yastrov

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

#ifndef __CS_SAVERREF_H__
#define __CS_SAVERREF_H__

/**\file
 * Saver references. Includes csLibraryReference and csAddonReference.
 */

#include "csextern.h"

#include "csutil/csobject.h"
#include "csutil/csstring.h"
#include "csutil/scf_implementation.h"
#include "imap/saverref.h"

/**
 * An object representing a reference to a library.
 */
class CS_CRYSTALSPACE_EXPORT csLibraryReference :
  public scfImplementationExt1<csLibraryReference, csObject, iLibraryReference>
{
  csString file;
  csString path;
  bool checkDupes;
  
public:
  /// The constructor.
  csLibraryReference (const char* file, const char* path = 0,
    bool checkDupes = false);
  /// The destructor
  virtual ~csLibraryReference ();

  virtual const char* GetFile () const;

  virtual const char* GetPath () const;

  virtual bool GetCheckDupes () const;

  virtual iObject *QueryObject () { return (csObject*)this; }
};

/**
 * An object representing an addon.
 */
class CS_CRYSTALSPACE_EXPORT csAddonReference :
  public scfImplementationExt1<csAddonReference, csObject, iAddonReference>
{
  csString plugin;
  csString paramsfile;
  csRef<iBase> addonobj;
  
public:
  /// The constructor.
  csAddonReference (const char* plugin, const char* paramsfile,
    iBase* addonobj = 0);
  /// The destructor
  virtual ~csAddonReference ();
  
  virtual const char* GetPlugin () const;

  virtual const char* GetParamsFile () const;

  virtual iBase* GetAddonObject () const;

  virtual iObject *QueryObject () { return (csObject*)this; }
};

#endif // __CS_SAVERREF_H__
