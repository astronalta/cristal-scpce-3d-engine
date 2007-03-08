/*
    Copyright (C) 2006 by Kapoulkine Arseny

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

#include "cell.h"

#include "csgeom/csrect.h"
#include "csgeom/vector3.h"
#include "csgfx/imagebase.h"
#include "csgfx/imagemanipulate.h"
#include "csgfx/imagememory.h"

#include "iengine/material.h"

#include "iterrain/terraincollider.h"
#include "iterrain/terrainrenderer.h"
#include "iterrain/terrainsystem.h"

#include "debug.h"

CS_PLUGIN_NAMESPACE_BEGIN(ImprovedTerrain)
{

csTerrainCell::csTerrainCell (iTerrainSystem* parent, const char* name,
  int grid_width, int grid_height, int material_width, int material_height,
  bool material_persistent, 
  const csVector2& position, const csVector3& size, iTerrainDataFeeder* feeder,
  iTerrainCellRenderProperties* render_properties,
  iTerrainCellCollisionProperties* collision_properties,
  iTerrainRenderer* renderer, iTerrainCollider* collider)
  : scfImplementationType (this), parent (parent), name (name), 
  material_width (material_width), material_height (material_height),
  material_persistent (material_persistent), position (position), size (size),
  feeder (feeder), render_properties (render_properties), 
  collision_properties (collision_properties), renderer (renderer), collider (collider),
  state (NotLoaded), render_data (0), last_colorVersion (0)
{
  // Here we do grid width/height correction. The height map will be a
  // square with size 2^n + 1

  int maxsize = MAX(grid_width, grid_height) - 1;
  int temp = 1;
  while (temp < maxsize) temp *= 2;  
  this->grid_width = temp + 1;
  this->grid_height = temp + 1;
  
  step_x = size.x / (grid_width - 1);
  step_z = size.z / (grid_height - 1);
}

csTerrainCell::~csTerrainCell ()
{
}

iTerrainSystem* csTerrainCell::GetTerrain()
{
  return parent;
}

csTerrainCell::LoadState csTerrainCell::GetLoadState () const
{
  return state;
}

void csTerrainCell::SetLoadState(LoadState state)
{
  Touch();

  switch (this->state)
  {
    case NotLoaded:
    {
      switch (state)
      {
        case NotLoaded: 
          break;
        case PreLoaded:
        {
          heightmap.SetSize (grid_width * grid_height, 0);

          if (material_persistent)
            materialmap.SetSize (material_width * material_height, 0);

          this->state = feeder->PreLoad (this) ? PreLoaded : NotLoaded;

          break;
        }
        case Loaded:
        {
          heightmap.SetSize (grid_width * grid_height);

          if (material_persistent)
            materialmap.SetSize (material_width * material_height, 0);

          this->state = feeder->Load (this) ? Loaded : NotLoaded;

          if (this->state == Loaded)
            parent->UnloadLRUCells();

          break;
        }
      }

      break;
    }
    case PreLoaded:
    {
      switch (state)
      {
        case NotLoaded: 
          break;
        case PreLoaded: 
          break;
        case Loaded:
        {
          this->state = feeder->Load (this) ? Loaded : NotLoaded;

          if (this->state == Loaded)
            parent->UnloadLRUCells();

          break;
        }
      }

      break;
    }
    case Loaded:
    {
      switch (state)
      {
        case NotLoaded:
        {
          heightmap.DeleteAll ();
          materialmap.DeleteAll ();
          staticLights.DeleteAll ();
          staticColors.DeleteAll ();

          render_data = 0;
          collision_data = 0;
  
          this->state = NotLoaded;

          break;
        }
        case PreLoaded: 
          break;
        case Loaded: 
          break;
      }

      break; 
    }
  }
}

csBox3 csTerrainCell::GetBBox () const
{
  csBox3 box;
  box.Set (position.x, 0, position.y,
  position.x + size.x, size.y, position.y + size.z);

  return box;
}

iTerrainDataFeeder* csTerrainCell::GetDataFeeder () const
{
  return feeder;
}

const char* csTerrainCell::GetName () const
{
  return name.GetData ();
}

iTerrainCellRenderProperties* csTerrainCell::GetRenderProperties () const
{
  return render_properties;
}

iTerrainCellCollisionProperties* csTerrainCell::GetCollisionProperties () const
{
  return collision_properties;
}

csRefCount* csTerrainCell::GetRenderData () const
{
  return render_data;
}

void csTerrainCell::SetRenderData (csRefCount* data)
{
  Touch();
  render_data = data;
}

csRefCount* csTerrainCell::GetCollisionData () const
{
  return collision_data;
}

void csTerrainCell::SetCollisionData (csRefCount* data)
{
  collision_data = data;
}

int csTerrainCell::GetGridWidth () const
{
  return grid_width;
}

int csTerrainCell::GetGridHeight () const
{
  return grid_height;
}

csLockedHeightData csTerrainCell::GetHeightData ()
{
  Touch();

  csLockedHeightData data;
  data.data = heightmap.GetArray ();
  data.pitch = grid_width;

  return data;
}

csLockedHeightData csTerrainCell::LockHeightData (const csRect& rectangle)
{
  Touch();

  locked_height_rect = rectangle;

  locked_height_data.data = heightmap.GetArray () + rectangle.ymin *
  grid_width + rectangle.xmin;
  locked_height_data.pitch = grid_width;

  return locked_height_data;
}

void csTerrainCell::UnlockHeightData ()
{
  Touch();

  renderer->OnHeightUpdate (this, locked_height_rect, heightmap.GetArray (),
    grid_width);
  
  if (collider)
    collider->OnHeightUpdate (this, locked_height_rect, heightmap.GetArray (),
      grid_width);
}

const csVector2& csTerrainCell::GetPosition () const
{
  return position;
}

const csVector3& csTerrainCell::GetSize () const
{
  return size;
}

int csTerrainCell::GetMaterialMapWidth () const
{
  return material_width;
}

int csTerrainCell::GetMaterialMapHeight () const
{
  return material_height;
}

bool csTerrainCell::GetMaterialPersistent() const
{
  return material_persistent;
}

csLockedMaterialMap csTerrainCell::LockMaterialMap (const csRect& rectangle)
{
  Touch();

  csLockedMaterialMap data;
  
  if (!material_persistent)
  {
    materialmap.SetSize (rectangle.Width () * rectangle.Height ());

    data.data = materialmap.GetArray ();
    data.pitch = rectangle.Width ();
  }
  else
  {
    data.data = materialmap.GetArray () + rectangle.ymin * material_width +
      rectangle.xmin;
    data.pitch = material_width;
  }

  mm_rect = rectangle;

  return data;
}

void csTerrainCell::UnlockMaterialMap ()
{
  Touch();

  csDirtyAccessArray<unsigned char> alpha;
  alpha.SetSize (mm_rect.Width () * mm_rect.Height ());
  
  for (unsigned int i = 0; i < parent->GetMaterialPalette ().GetSize (); ++i)
  {
    for (int y = 0; y < mm_rect.Height (); ++y)
    {
      for (int x = 0; x < mm_rect.Width (); ++x)
      {
        unsigned char p = materialmap[y * mm_rect.Width () + x];
        alpha[y * mm_rect.Width () + x] = (p == i) ? 255 : 0;
      }
    }
    
    renderer->OnMaterialMaskUpdate (this, i, mm_rect, alpha.GetArray (),
      mm_rect.Width ());
  }

  if (!material_persistent) materialmap.DeleteAll ();
}

void csTerrainCell::SetMaterialMask (unsigned int material, iImage* image)
{
  Touch();

  if (image->GetFormat () != CS_IMGFMT_PALETTED8) return;
  
  csRef<iImage> rescaled_image;

  if (image->GetWidth () != material_width || image->GetHeight () !=
    material_height)
  {
    rescaled_image = csImageManipulate::Rescale (image, material_width,
      material_height);
    image = rescaled_image;
  }

  renderer->OnMaterialMaskUpdate (this, material,
  csRect(0, 0, image->GetWidth (), image->GetHeight ()),
  (const unsigned char*)image->GetImageData (), image->GetWidth ());
}

void csTerrainCell::SetMaterialMask (unsigned int material,
const unsigned char* data, unsigned int width, unsigned int height)
{
  Touch();

  csImageMemory image(width, height, (void*)data, false, CS_IMGFMT_PALETTED8);
	
  SetMaterialMask (material, &image);
}

void csTerrainCell::SetBaseMaterial (iMaterialWrapper* material)
{
  Touch ();

  baseMaterial = material;
}

iMaterialWrapper* csTerrainCell::GetBaseMaterial () const
{
  return baseMaterial;
}

bool csTerrainCell::CollideSegment (const csVector3& start, const csVector3&
end, bool oneHit, iTerrainVector3Array& points)
{
  if (!collider || !collision_properties->GetCollideable ()) return false;

  Touch();

  return collider->CollideSegment (this, start, end, oneHit, points);
}

bool csTerrainCell::CollideTriangles (const csVector3* vertices,
                       unsigned int tri_count,
                       const unsigned int* indices, float radius,
                       const csReversibleTransform* trans,
                       bool oneHit, iTerrainCollisionPairArray& pairs)
{
  if (!collider || !collision_properties->GetCollideable ()) return false;

  Touch();

  return collider->CollideTriangles (this, vertices, tri_count, indices,
                                     radius, trans, oneHit, pairs);
}

bool csTerrainCell::Collide (iCollider* collider, float radius,
                       const csReversibleTransform* trans, bool oneHit,
                       iTerrainCollisionPairArray& pairs)
{
  if (!this->collider || !collision_properties->GetCollideable ())
    return false;

  return this->collider->Collide (this, collider, radius, trans, oneHit,
    pairs);
}

static inline float Lerp (const float x1, const float x2, const float t)
{
  return x1 + (x2 - x1) * t;
}

void csTerrainCell::LerpHelper (const csVector2& pos, int& x1, int& x2,
  float& xfrac, int& y1, int& y2, float& yfrac) const
{
  float x = (pos.x / size.x) * (grid_width - 1);
  float y = (pos.y / size.z) * (grid_height - 1);

  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x > grid_width - 1) x = grid_width - 1;
  if (y > grid_height - 1) y = grid_height - 1;

  x1 = floor (x);
  x2 = ceil (x);
  xfrac = x - x1;

  y1 = floor (y);
  y2 = ceil (y);
  yfrac = y - y1;
}

float csTerrainCell::GetHeight (int x, int y) const
{
  return heightmap[y * grid_width + x];
}

float csTerrainCell::GetHeight (const csVector2& pos) const
{
  int x1, y1, x2, y2;
  float xfrac, yfrac;

  LerpHelper (pos, x1, x2, xfrac, y1, y2, yfrac);

  float h1 = Lerp (GetHeight (x1, y1), GetHeight (x2, y1), xfrac);
  float h2 = Lerp (GetHeight (x1, y2), GetHeight (x2, y2), xfrac);

  return Lerp (h1, h2, yfrac);
}

static inline csVector3 Lerp (const csVector3& x, const csVector3& y,
const float t)
{
  return x + (y - x) * t;
}

csVector3 csTerrainCell::GetTangent (int x, int y) const
{
  float center = GetHeight (x, y);
  float left = x == 0 ? center : GetHeight (x-1, y);
  float right = x + 1 == grid_width ? center : GetHeight (x+1, y);

  return csVector3(1.0f / grid_width, right - left, 0);
}

csVector3 csTerrainCell::GetTangent (const csVector2& pos) const
{
  int x1, y1, x2, y2;
  float xfrac, yfrac;

  LerpHelper (pos, x1, x2, xfrac, y1, y2, yfrac);

  csVector3 n1 = Lerp (GetTangent (x1, y1), GetTangent (x2, y1), xfrac);
  csVector3 n2 = Lerp (GetTangent (x1, y2), GetTangent (x2, y2), xfrac);

  return Lerp (n1, n2, yfrac).Unit ();
}

csVector3 csTerrainCell::GetBinormal (int x, int y) const
{
  float center = GetHeight (x, y);
  float up = y == 0 ? center : GetHeight (x, y-1);
  float down = y + 1 == grid_height ? center : GetHeight (x, y+1);

  return csVector3(0, down - up, 1.0f / grid_height);
}

csVector3 csTerrainCell::GetBinormal (const csVector2& pos) const
{
  int x1, y1, x2, y2;
  float xfrac, yfrac;

  LerpHelper (pos, x1, x2, xfrac, y1, y2, yfrac);

  csVector3 n1 = Lerp (GetBinormal (x1, y1), GetBinormal (x2, y1), xfrac);
  csVector3 n2 = Lerp (GetBinormal (x1, y2), GetBinormal (x2, y2), xfrac);

  return Lerp (n1, n2, yfrac).Unit ();
}

csVector3 csTerrainCell::GetNormal (int x, int y) const
{
  float center = GetHeight (x, y);

  float dfdy = 0;
  if (y - 1 >= 0 && y + 1 < grid_height)
    dfdy = (GetHeight (x, y + 1) - GetHeight (x, y - 1)) / 2*step_z; 
  else if (y - 1 >= 0)
    dfdy = (center - GetHeight (x, y - 1)) / step_z;
  else if (y + 1 < grid_height)
    dfdy = (GetHeight (x, y + 1) - center) / step_z;

  float dfdx = 0;
  if (x - 1 >= 0 && x + 1 < grid_width)
    dfdx = (GetHeight (x + 1, y) - GetHeight (x - 1, y)) / 2*step_x; 
  else if (x - 1 >= 0)
    dfdx = (center - GetHeight (x - 1, y)) / step_x;
  else if (x + 1 < grid_width)
    dfdx = (GetHeight (x + 1, y) - center) / step_x;

  return csVector3 (dfdx, 1, dfdy);
}

csVector3 csTerrainCell::GetNormal (const csVector2& pos) const
{
  int x1, y1, x2, y2;
  float xfrac, yfrac;

  LerpHelper (pos, x1, x2, xfrac, y1, y2, yfrac);

  csVector3 n1 = Lerp (GetNormal (x1, y1), GetNormal (x2, y1), xfrac);
  csVector3 n2 = Lerp (GetNormal (x1, y2), GetNormal (x2, y2), xfrac);

  return Lerp (n1, n2, yfrac).Unit ();
}

void csTerrainCell::UpdateColors (iMovable* movable, unsigned int colorVersion,
  const csColor& baseColor)
{
  Touch();

  if (colorVersion == last_colorVersion) return;

  if (last_colorVersion == 0)
  {
    staticLights.SetSize (lmres * lmres);

    for (size_t i = 0; i < staticLights.GetSize (); ++i)
      staticLights[i] = ambient;
  }

  last_colorVersion = colorVersion;

  staticColors.SetSize (staticLights.GetSize ());
  
  for (size_t i = 0; i < staticLights.GetSize (); i++)
  {
    staticColors[i] = staticLights[i] + baseColor;
  }

  /*csHash<csShadowArray*, csPtrKey<iLight> >::GlobalIterator pdlIt =
	pseudoDynInfo.GetIterator ();
  while (pdlIt.HasNext ())
  {
    csPtrKey<iLight> light;
    csShadowArray* shadowArr = pdlIt.Next (light);
    float* intensities = shadowArr->shadowmap;
    const csColor& lightcol = light->GetColor ();

    if (lightcol.red > EPSILON || lightcol.green > EPSILON
        || lightcol.blue > EPSILON)
    {
      for (i = 0; i < staticLights.Length(); i++)
      {
        staticColors[i] += lightcol * intensities[i];
      }
    }
  }*/

  if (renderer) renderer->OnColorUpdate (this, staticColors.GetArray (), lmres);
}

}
CS_PLUGIN_NAMESPACE_END(ImprovedTerrain)
