/*
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

#ifndef __SIMPCD_H__
#define __SIMPCD_H__

#include <stdarg.h>
#include <crystalspace.h>

class Simple
{
private:
  iObjectRegistry* object_reg;

  csEventID Process;
  csEventID FinalProcess;
  csEventID KeyboardDown;

  csRef<iEngine> engine;
  csRef<iLoader> loader;
  csRef<iGraphics3D> g3d;
  csRef<iKeyboardDriver> kbd;
  csRef<iVirtualClock> vc;
  iSector* room;
  csRef<iView> view;
  csRef<iCollideSystem> cdsys;

  csRef<iMeshWrapper> parent_sprite;
  float rot1_direction;
  csRef<iMeshWrapper> sprite1;
  iCollider* sprite1_col;
  float rot2_direction;
  csRef<iMeshWrapper> sprite2;
  iCollider* sprite2_col;

  static bool SimpleEventHandler (iEvent& ev);
  bool HandleEvent (iEvent& ev);
  void SetupFrame ();
  void FinishFrame ();
  iCollider* InitCollider (iMeshWrapper* mesh);

public:
  Simple ();
  ~Simple ();

  bool Initialize (iObjectRegistry* object_reg);
  void Start ();
};

#endif // __SIMPCD_H__

