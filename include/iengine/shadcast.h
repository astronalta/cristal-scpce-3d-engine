/*
    Crystal Space 3D engine
    Copyright (C) 2001 by Jorrit Tyberghein
  
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

#ifndef __I_SHADCAST_H__
#define __I_SHADCAST_H__

#include "csutil/scf.h"

struct iShadowCaster;
struct iShadowReceiver;
struct iMovable;
class csBox3;

SCF_VERSION (iShadowSystem, 0, 0, 1);

/**
 * A shadow system is very much like a visibility culler and very
 * often a shadow system actually also implements a visibility
 * culler system. A shadow system basically knows how cast shadows
 * for shadow casters from a given point and the send those shadows to
 * shadow receivers. To use a shadow system you first need to register
 * shadow casters and shadow receivers.
 */
struct iShadowSystem : public iBase
{
  /// Register a shadow caster.
  virtual void RegisterCaster (iShadowCaster* caster) = 0;
  /// Unregister a shadow caster.
  virtual void UnregisterCaster (iShadowCaster* caster) = 0;
  /// Register a shadow receiver.
  virtual void RegisterReceiver (iShadowReceiver* receiver) = 0;
  /// Unregister a shadow receiver.
  virtual void UnregisterReceiver (iShadowReceiver* receiver) = 0;
  /**
   * Start casting shadows from a given point in space.
   */
  virtual void CastShadows (const csVector3& pos) = 0;
};

SCF_VERSION (iShadowCaster, 0, 0, 1);

/**
 * An object that can cast shadows.
 */
struct iShadowCaster : public iBase
{
  /// Get the reference to the movable from this object.
  virtual iMovable* GetMovable () = 0;
  /// Get the shape number of the underlying object.
  virtual long GetShapeNumber () = 0;
  /// Get the bounding box of the object in object space.
  virtual void GetBoundingBox (csBox3& bbox) = 0;
};

SCF_VERSION (iShadowReceiver, 0, 0, 1);

/**
 * An object that is interested in getting shadow information.
 */
struct iShadowReceiver : public iBase
{
  /// Get the reference to the movable from this object.
  virtual iMovable* GetMovable () = 0;
  /// Get the shape number of the underlying object.
  virtual long GetShapeNumber () = 0;
  /// Get the bounding box of the object in object space.
  virtual void GetBoundingBox (csBox3& bbox) = 0;
};

#endif // __I_SHADCAST_H__

