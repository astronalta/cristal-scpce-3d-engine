/////////////////////////////////////////////////////////////////////////////
// Copyright:   (c) Aleksandras Gluchovas
// Licence:     wxWindows license
//
// Ported to Crystal Space by Norman Kr�mer <norman@users.sourceforge.net>
/////////////////////////////////////////////////////////////////////////////

#include "cssysdef.h"
#include "csws/csabslay.h"

/***** Implementation for class csAbsoluteLayout *****/

csAbsoluteLayout::csAbsoluteLayout (csComponent *pParent) : csLayout (pParent)
{
}
// impl. of LayoutManager interface

void csAbsoluteLayout::SuggestSize (int &sugw, int& sugh)
{
  sugw = 0;
  sugh = 0;
  int i, cnt = vConstraints.Length ();

  for (i=0; i<cnt; i++)
  {
    sugw = MAX (sugw, vConstraints.Get (i)->comp->bound.xmax);
    sugh = MAX (sugh, vConstraints.Get (i)->comp->bound.ymax);
  }

}

void csAbsoluteLayout::LayoutContainer ()
{
  // no layouting is done of absolute-layouts,
  // positions of components are left unchanged (i.e. initial)
}
