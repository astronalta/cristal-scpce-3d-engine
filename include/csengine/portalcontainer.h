/*
    Copyright (C) 2003 by Jorrit Tyberghein

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

#ifndef __CS_PORTALCONTAINER_H__
#define __CS_PORTALCONTAINER_H__

#include "iengine/portalcontainer.h"
#include "csutil/refarr.h"
#include "csutil/garray.h"
#include "csgeom/vector3.h"
#include "csgeom/pmtools.h"
#include "csengine/portal.h"
#include "cstool/meshobjtmpl.h"

/**
 * A helper class for iPolygonMesh implementations used by csPortalContainer.
 */
class csPortalContainerPolyMeshHelper : public iPolygonMesh
{
public:
  SCF_DECLARE_IBASE;

  /**
   * Make a polygon mesh helper which will accept polygons which match
   * with the given flag (one of CS_POLY_COLLDET or CS_POLY_VISCULL).
   */
  csPortalContainerPolyMeshHelper (uint32 flag) :
  	polygons (0), vertices (0), poly_flag (flag), triangles (0)
  {
    SCF_CONSTRUCT_IBASE (0);
  }
  virtual ~csPortalContainerPolyMeshHelper ()
  {
    SCF_DESTRUCT_IBASE ();
    Cleanup ();
  }

  void Setup ();
  void SetPortalContainer (csPortalContainer* pc);

  virtual int GetVertexCount ()
  {
    Setup ();
    return vertices->Length ();
  }
  virtual csVector3* GetVertices ()
  {
    Setup ();
    return vertices->GetArray ();
  }
  virtual int GetPolygonCount ()
  {
    Setup ();
    return num_poly;
  }
  virtual csMeshedPolygon* GetPolygons ()
  {
    Setup ();
    return polygons;
  }
  virtual int GetTriangleCount ()
  {
    Triangulate ();
    return tri_count;
  }
  virtual csTriangle* GetTriangles ()
  {
    Triangulate ();
    return triangles;
  }
  virtual void Lock () { }
  virtual void Unlock () { }
 
  virtual csFlags& GetFlags () { return flags;  }
  virtual uint32 GetChangeNumber() const { return 0; }

  void Cleanup ();

private:
  csPortalContainer* parent;
  uint32 data_nr;		// To see if the portal container has changed.
  csMeshedPolygon* polygons;	// Array of polygons.
  // Array of vertices from portal container.
  csDirtyAccessArray<csVector3>* vertices;
  int num_poly;			// Total number of polygons.
  uint32 poly_flag;		// Polygons must match with this flag.
  csFlags flags;
  csTriangle* triangles;
  int tri_count;

  void Triangulate ()
  {
    if (triangles) return;
    csPolygonMeshTools::Triangulate (this, triangles, tri_count);
  }
};

/**
 * This is a container class for portals.
 */
class csPortalContainer : public csMeshObject, public iPortalContainer
{
private:
  csDirtyAccessArray<csVector3> vertices;
  csRefArray<csPortal> portals;
  bool prepared;
  // Number that is increased with every significant change.
  uint32 data_nr;

  csBox3 object_bbox;
  csVector3 object_radius;

protected:
  /**
   * Destructor.  This is private in order to force clients to use DecRef()
   * for object destruction.
   */
  virtual ~csPortalContainer ();

public:
  /// Constructor.
  csPortalContainer (iEngine* engine);

  uint32 GetDataNumber () const { return data_nr; }
  void Prepare ();
  csDirtyAccessArray<csVector3>* GetVertices () { return &vertices; }
  const csRefArray<csPortal>& GetPortals () const { return portals; }

  SCF_DECLARE_IBASE_EXT (csMeshObject);

  //-------------------- iPolygonMesh interface implementation ---------------
  csPortalContainerPolyMeshHelper scfiPolygonMesh;
  //------------------- CD iPolygonMesh implementation ---------------
  csPortalContainerPolyMeshHelper scfiPolygonMeshCD;
  //------------------- LOD iPolygonMesh implementation ---------------
  csPortalContainerPolyMeshHelper scfiPolygonMeshLOD;

  //-------------------For iPortalContainer ----------------------------//
  virtual iPortal* CreatePortal (csVector3* vertices, int num);
  virtual void RemovePortal (iPortal* portal);

  //--------------------- For iMeshObject ------------------------------//
  virtual iMeshObjectFactory* GetFactory () const { return 0; }
  virtual bool DrawTest (iRenderView* rview, iMovable* movable);
  virtual bool Draw (iRenderView* rview, iMovable* movable,
  	csZBufMode zbufMode);
  virtual void HardTransform (const csReversibleTransform& t);
  virtual bool SupportsHardTransform () const { return true; }
  virtual bool HitBeamOutline (const csVector3& start,
  	const csVector3& end, csVector3& isect, float* pr);
  virtual bool HitBeamObject (const csVector3& start, const csVector3& end,
  	csVector3& isect, float* pr);
  virtual int GetPortalCount () const { return portals.Length () ; }
  virtual iPortal* GetPortal (int idx) const { return (iPortal*)portals[idx]; }

  //--------------------- For csMeshObject ------------------------------//
  virtual void GetObjectBoundingBox (csBox3& bbox, int)
  {
    Prepare ();
    bbox = object_bbox;
  }
  virtual void GetRadius (csVector3& radius, csVector3& center);
};

#endif // __CS_PORTALCONTAINER_H__

