#ifndef __OPCODE_PLUGIN_H__
#define __OPCODE_PLUGIN_H__

#include "iutil/comp.h"
#include "csgeom/vector3.h"
#include "ivaria/collider.h"
#include "csutil/csdllist.h"
#include "CSopcodecollider.h"
#include "Opcode.h"

struct iObjectRegistry;
//struct AABBTreeCollider;
//struct BVTCache;
/**
* This is the implementation for our API and
* also the implementation of the plugin.
*/
class csOPCODECollideSystem : public iCollideSystem
{

public:
   
	Opcode::AABBTreeCollider TreeCollider;
//  CollisionFaces CollideFaces;
//  csDLinkList CacheList;
	Opcode::BVTCache ColCache;
  csCollisionPair *pairs;
  iObjectRegistry *object_reg;

 SCF_DECLARE_IBASE;

  csOPCODECollideSystem (iBase* parent);
  virtual ~csOPCODECollideSystem ();
  bool Initialize (iObjectRegistry* iobject_reg);


  virtual iCollider* CreateCollider (iPolygonMesh* mesh);

  /**
   * Test collision between two colliders.
   * This is only supported for iCollider objects created by
   * this plugin. Returns false if no collision or else true.
   * The collisions will be added to the collision pair array
   * that you can query with GetCollisionPairs and reset/clear
   * with ResetCollisionPairs. Every call to Collide will
   * add to that array.
   */
  virtual bool Collide (
  	iCollider* collider1, const csReversibleTransform* trans1,
  	iCollider* collider2, const csReversibleTransform* trans2);

  /**
   * Get pointer to current array of collision pairs.
   * This array will grow with every call to Collide until you clear
   * it using 'ResetCollisionPairs'.
   */
  virtual csCollisionPair* GetCollisionPairs ();

  /**
   * Get number of collision pairs in array.
   */
  virtual int GetCollisionPairCount ();

  /**
   * Reset the array with collision pairs.
   */
  virtual void ResetCollisionPairs ();

  /**
   * Indicate if we are interested only in the first hit that is found.
   * This is only valid for CD algorithms that actually allow the
   * detection of multiple CD hit points.
   */
  virtual void SetOneHitOnly (bool o);

  /**
   * Return true if this CD system will only return the first hit
   * that is found. For CD systems that support multiple hits this
   * will return the value set by the SetOneHitOnly() function.
   * For CD systems that support one hit only this will always return true.
   */
  virtual bool GetOneHitOnly ();

  /**
   * Test if an object can move to a new position. The new position
   * vector will be modified to reflect the maximum new position that the
   * object could move to without colliding with something. This function
   * will return:
   * <ul>
   * <li>-1 if the object could not move at all (i.e. stuck at start position).
   * <li>0 if the object could not move fully to the desired position.
   * <li>1 if the object can move unhindered to the end position.
   * </ul>
   * <p>
   * This function will reset the collision pair array. If there was a
   * collision along the way the array will contain the information for
   * the first collision preventing movement.
   * <p>
   * The given transform should be the transform of the object corresponding
   * with the old position. 'colliders' and 'transforms' should be arrays
   * with 'num_colliders' elements for all the objects that we should
   * test against.
   */
  virtual int CollidePath (
  	iCollider* collider, const csReversibleTransform* trans,
	csVector3& newpos,
	int num_colliders,
	iCollider** colliders,
	csReversibleTransform** transforms);

  struct Component : public iComponent
  {
    SCF_DECLARE_EMBEDDED_IBASE (csOPCODECollideSystem);
    virtual bool Initialize (iObjectRegistry* object_reg)
    {
      return scfParent->Initialize (object_reg);
    }
  } scfiComponent;
};

#endif // __OPCODE_PLUGIN_H__
