/*
  Copyright (C) 2006 by Marten Svanfeldt

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

#ifndef __CS_MESH_BUILTINEFFECTORS_H__
#define __CS_MESH_BUILTINEFFECTORS_H__

#include "csutil/scf_implementation.h"

#include "imesh/particles.h"
#include "iutil/comp.h"


CS_PLUGIN_NAMESPACE_BEGIN(Particles)
{

  class ParticleEffectorFactory : public 
    scfImplementation2<ParticleEffectorFactory,
                       iParticleBuiltinEffectorFactory,
                       iComponent>
  {
  public:
    ParticleEffectorFactory (iBase* parent)
      : scfImplementationType (this, parent)
    {
    }

    //-- iParticleBuiltinEffectorFactory
    virtual csPtr<iParticleBuiltinEffectorForce> CreateForce () const;
    virtual csPtr<iParticleBuiltinEffectorLinColor> CreateLinColor () const;

    //-- iComponent
    virtual bool Initialize (iObjectRegistry*)
    {
      return true;
    }
  };


  class ParticleEffectorForce : public 
    scfImplementation2<ParticleEffectorForce,
                       iParticleBuiltinEffectorForce,
                       scfFakeInterface<iParticleEffector> >
  {
  public:
    ParticleEffectorForce ()
      : scfImplementationType (this),
      acceleration (0.0f), force (0.0f), randomAcceleration (0.0f)
    {
    }

    //-- iParticleEffector
    virtual csPtr<iParticleEffector> Clone () const;

    virtual void EffectParticles (iParticleSystemBase* system,
      const csParticleBuffer& particleBuffer, float dt, float totalTime);

    //-- iParticleBuiltinEffectorForce
    virtual void SetAcceleration (const csVector3& acceleration)
    {
      this->acceleration = acceleration;
    }

    virtual const csVector3& GetAcceleration () const
    {
      return acceleration;
    }

    virtual void SetForce (const csVector3& force)
    {
      this->force = force;
    }

    virtual const csVector3& GetForce () const
    {
      return force; 
    }

    virtual void SetRandomAcceleration (float magnitude)
    {
      randomAcceleration = magnitude;
    }

    virtual float GetRandomAcceleration () const
    {
      return randomAcceleration;
    }

  private:
    csVector3 acceleration;
    csVector3 force;
    float randomAcceleration;
  };

  class ParticleEffectorLinColor : public
    scfImplementation2<ParticleEffectorLinColor,
                       iParticleBuiltinEffectorLinColor,
                       scfFakeInterface<iParticleEffector> >
  {
  public:
    //-- ParticleEffectorLinColor
    ParticleEffectorLinColor ();

    //-- iParticleEffector
    virtual csPtr<iParticleEffector> Clone () const;

    virtual void EffectParticles (iParticleSystemBase* system,
      const csParticleBuffer& particleBuffer, float dt, float totalTime);


    //-- iParticleBuiltinEffectorLinColor
    virtual size_t AddColor (const csColor& color, float endTime);

    virtual void SetColor (size_t index, const csColor& color);

    virtual void GetColor (size_t index, csColor& color, float& time) const
    {
      if (index >= colorList.GetSize ())
        return;

      color = colorList[index].color;
      time = colorList[index].endTime;
    }

    virtual size_t GetColorCount () const
    {
      return colorList.GetSize ();
    }

    virtual void SetMaxAge (float max)
    {
      maxAge = max;
    }

    virtual float GetMaxAge () const
    {
      return maxAge;
    }

  private:
    void Precalc ();

    struct ColorEntry
    {
      csColor color;
      float endTime;
    };
    csArray<ColorEntry> colorList;

    struct PrecalcEntry
    {
      csColor mult;
      csColor add;
      float endTime;
    };
    bool precalcInvalid;
    csArray<PrecalcEntry> precalcList;

    float maxAge;
  };

}
CS_PLUGIN_NAMESPACE_END(Particles)

#endif

