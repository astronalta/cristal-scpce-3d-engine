#ifndef GL_POLYBUFEXT_H_INCLUDED
#define GL_POLYBUFEXT_H_INCLUDED
/* ========== HTTP://WWW.SOURCEFORGE.NET/PROJECTS/CRYSTAL ==========
 *
 * FILE: gl_polybufext.h
 *
 * DESCRIPTION:
 *  Implements a Vertex Buffer suitable for forming primitive types
 *
 * LICENSE:
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * AUTHOR:
 *    Philipp R. Aumayr
 *
 * CVS/RCS ID:
 *    $Id:
 *
 * === COPYRIGHT (c)2002 ============== PROJECT CRYSTAL SPACE 3D === */
/* -----------------------------------------------------------------
 * Preprocessor Includes
 * ----------------------------------------------------------------- */

struct csPolygonBufferEXT : public iPolygonBuffer
{
  csPolygonBufferEXT(iGraphics3D *g3d);
  virtual ~csPolygonBufferEXT();
  /**
   * Add a polygon to this buffer. The data pointed to by 'verts'
   * is copied so it can be discarded after calling AddPolygon.
   * 'mat_index' is an index in the material table (initialized
   * with AddMaterial()). It is best to add the polygons sorted by
   * material as that will generate the most efficient representation
   * on hardware.
   */
  virtual void AddPolygon (int* verts, int num_verts,
	const csPlane3& poly_normal,
	int mat_index,
	const csMatrix3& m_obj2tex, const csVector3& v_obj2tex,
	iPolygonTexture* poly_texture);

  /**
   * Set vertices to use for the polygons.
   * The given array is copied.
   */
  virtual void SetVertexArray (csVector3* verts, int num_verts);

  /**
   * Add a material.
   */
  virtual void AddMaterial (iMaterialHandle* mat_handle);
  /**
   * Get the number of materials.
   */
  virtual int GetMaterialCount () const;
  /**
   * Get a material.
   */
  virtual iMaterialHandle* GetMaterial (int idx) const;

  /// Gets the numebr of vertices
  virtual int GetVertexCount() const;

  ///Gets the array of vertices

  virtual csVector3* GetVertices() const;

  /**
   * Set a previously added material (this can be used to change
   * a material handle).
   */
  virtual void SetMaterial (int idx, iMaterialHandle* mat_handle);

  /// Clear all polygons, materials, and vertex array.
  virtual void Clear ();

   /** Sets the polygon buffer as dirty
  * This means that the mesh is affected by some light 
  */
  virtual void MarkLightmapsDirty();

private:

  iGraphics3D *m_g3d;
  iVertexBuffer *m_vbuf;


};

#endif
