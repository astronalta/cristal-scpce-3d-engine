/*
    Copyright (C) 2007-2008 by Marten Svanfeldt

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

#ifndef __CS_CSPLUGINCOMMON_RENDERMANAGER_LIGHTSETUP_H__
#define __CS_CSPLUGINCOMMON_RENDERMANAGER_LIGHTSETUP_H__

#include "ivideo/shader/shader.h"

#include "csplugincommon/rendermanager/operations.h"
#include "csplugincommon/rendermanager/rendertree.h"

class csShaderVariable;

namespace CS
{
namespace RenderManager
{
  template<typename RenderTree, typename LayerConfigType,
    typename NewLayersType>
  class LayerHelper
  {
  public:
    struct PersistentData
    {
      csArray<size_t> newLayerIndices;
      csArray<size_t> newLayerCounts;
    };

    LayerHelper (PersistentData& persist, 
      const LayerConfigType& layerConfig,
      NewLayersType& newLayers) : persist (persist), 
      layerConfig (layerConfig), newLayers (newLayers)
    {
      persist.newLayerIndices.SetSize (layerConfig.GetLayerCount ());
      persist.newLayerCounts.SetSize (layerConfig.GetLayerCount ());
      for (size_t l = 0; l < layerConfig.GetLayerCount (); l++)
      {
        persist.newLayerIndices[l] = l;
        persist.newLayerCounts[l] = 1;
      }
    }

    size_t GetNewLayerIndex (size_t layer, size_t subLayer)
    {
      return persist.newLayerIndices[layer] + subLayer;
    }

    void Ensure (size_t layer, size_t neededSubLayers,
                 typename RenderTree::MeshNode* node)
    {
      if (neededSubLayers > persist.newLayerCounts[layer])
      {
	// We need to insert new layers

	// How many?
	size_t insertLayerNum = neededSubLayers - persist.newLayerCounts[layer];
	// The actual insertion
	for (size_t n = persist.newLayerCounts[layer]; n < neededSubLayers; n++)
	{
	  node->owner.InsertLayer (persist.newLayerIndices[layer] + n - 1);
	  newLayers.InsertLayer (persist.newLayerIndices[layer] + n - 1, layer);
	}
	// Update indices for in new index table
	for (size_t l = layer+1; l < layerConfig.GetLayerCount (); l++)
	  persist.newLayerIndices[l] += insertLayerNum;
	persist.newLayerCounts[layer] += insertLayerNum;
      }
    }
  protected:
    PersistentData& persist;
    const LayerConfigType& layerConfig;
    NewLayersType& newLayers;
  };

  class LightingSorter
  {
    struct IndexLightTypePair
    {
      iLight* light;
      csLightType type;

      bool operator<(const IndexLightTypePair& other) const
      { return type < other.type; }
    };
  public:
    struct PersistentData
    {
      csArray<IndexLightTypePair> lightTypeScratch;
    };

    LightingSorter (PersistentData& persist,
      csLightInfluence* influenceLights, size_t numLights)
      : persist (persist)
    {
      // Sort lights by type
      persist.lightTypeScratch.Empty();
      for (size_t l = 0; l < numLights; l++)
      {
	IndexLightTypePair iltp;
	//iltp.index = l;
        iltp.light = influenceLights[l].light;
	iltp.type = influenceLights[l].light->GetType();
	persist.lightTypeScratch.Push (iltp);
      }
      persist.lightTypeScratch.Sort();
    }

    size_t GetSize() const
    {
      return persist.lightTypeScratch.GetSize();
    }

    iLight* GetLight (size_t n) const
    {
      return persist.lightTypeScratch[n].light;
    }

    csLightType GetLightType (size_t n) const
    {
      return persist.lightTypeScratch[n].type;
    }
  protected:
    PersistentData& persist;
  };

  template<typename RenderTree, typename LayerConfigType>
  class ShadowNone
  {
    struct CachedLightData
    {
      const csRefArray<csShaderVariable>* shaderVars;
    };

    void MergeAsArrayItems (csShaderVariableStack& dst, 
      const csRefArray<csShaderVariable>& allVars, size_t index)
    {
      for (size_t v = 0; v < allVars.GetSize(); v++)
      {
        csShaderVariable* sv = allVars[v];
        CS::ShaderVarStringID name = sv->GetName();
        
        if (name >= dst.GetSize()) break;
        csShaderVariable*& dstVar = dst[name];

        if (dstVar == 0) dstVar = new csShaderVariable (name);
        if ((dstVar->GetType() != csShaderVariable::UNKNOWN)
          && (dstVar->GetType() != csShaderVariable::ARRAY)) continue;
        dstVar->SetArraySize (csMax (index+1, dstVar->GetArraySize()));
        dstVar->SetArrayElement (index, sv);
      }
    }

    csShaderVariable* CreateVarOnStack (CS::ShaderVarStringID name,
                                        csShaderVariableStack& stack)
    {
      csRef<csShaderVariable> var = persist.svAlloc.Alloc();
      var->SetName (name);
      stack[name] = var;
      persist.svKeeper.Push (var);
      return var;
    }
  public:

    struct PersistentData
    {
      csLightShaderVarCache svNames;
      CS::ShaderVarStringID svPassNum;
      csShaderVarBlockAlloc<> svAlloc;
      /* A number of SVs have to be kept alive even past the expiration
       * of the actual step */
      csRefArray<csShaderVariable> svKeeper;
      LightingSorter::PersistentData lightSorterPersist;
      csHash<CachedLightData, csPtrKey<iLight> > lightDataCache;

      ~PersistentData()
      {
        if (lcb.IsValid()) lcb->parent = 0;
      }
      
      void Initialize (iShaderVarStringSet* strings)
      {
	svNames.SetStrings (strings);
        svPassNum = strings->Request ("pass number");
      }
      void UpdateNewFrame ()
      {
        svKeeper.Empty();
      }

      iLightCallback* GetLightCallback()
      {
        if (!lcb.IsValid()) lcb.AttachNew (new LightCallback (this));
        return lcb;
      }
    protected:
      class LightCallback : public scfImplementation1<LightCallback, 
                                                      iLightCallback>
      {
      public:
        PersistentData* parent;

        LightCallback (PersistentData* parent)
          : scfImplementation1<LightCallback, iLightCallback> (this),
            parent (parent) {}

	void OnColorChange (iLight* light, const csColor& newcolor) { }
	void OnPositionChange (iLight* light, const csVector3& newpos) { }
	void OnSectorChange (iLight* light, iSector* newsector) { }
	void OnRadiusChange (iLight* light, float newradius) { }
	void OnDestroy (iLight* light)
        {
          if (parent != 0)
          {
            parent->lightDataCache.DeleteAll (light);
          }
        }
	void OnAttenuationChange (iLight* light, int newatt) { }
      };
      csRef<LightCallback> lcb;
    };

    ShadowNone (PersistentData& persist,
      const LayerConfigType& layerConfig) 
      : persist (persist), layerConfig (layerConfig), lastShader (0) { }

    template<typename LayerHelper>
    void HandleLights (csLightInfluence* influenceLights, size_t numLights,
      size_t layer, LayerHelper& layers,
      typename RenderTree::MeshNode::SingleMesh& mesh,
      typename RenderTree::MeshNode* node)
    {
      LightingSorter sortedLights (persist.lightSorterPersist, influenceLights,
        numLights);

      /* Get the shader since the number of passes for that layer depend
	* on it */
      iShader* shaderToUse =
	node->owner.shaderArray[layers.GetNewLayerIndex (layer, 0) 
          * node->owner.totalRenderMeshes + mesh.contextLocalId];
      if (!shaderToUse) return;

      UpdateMetadata (shaderToUse);
      if (lastMetadata.numberOfLights == 0) return;

      const size_t layerLights = sortedLights.GetSize();

      // Set up layers
      size_t firstLight = 0;
      size_t remainingLights = layerLights;
      size_t totalLayers = 0;
      while (firstLight < layerLights)
      {
	csLightType lightType = sortedLights.GetLightType (firstLight);
	size_t num = 1;
	for (; num < remainingLights; num++)
	{
	  if (sortedLights.GetLightType (firstLight + num) != lightType)
	    break;
	}
	/* We have a subset of the lights that are of the same type.
	 * Check the size of it against the shader limit */
	size_t thisPassLayers = (num 
	  + lastMetadata.numberOfLights - 1) / lastMetadata.numberOfLights;
	thisPassLayers = csMin (thisPassLayers, layerConfig.GetMaxLightPasses (layer));
	size_t neededLayers = totalLayers + thisPassLayers;

        layers.Ensure (layer, neededLayers, node);

	firstLight += num;
	remainingLights -= num;
	totalLayers = neededLayers;
      }

      // Now render lights for each light type
      firstLight = 0;
      remainingLights = layerLights;
      totalLayers = 0;
      while (firstLight < layerLights)
      {
	csLightType lightType = sortedLights.GetLightType (firstLight);
	size_t num = 1;
	for (; num < remainingLights; num++)
	{
	  if (sortedLights.GetLightType (firstLight + num) != lightType)
	    break;
	}
	/* We have a subset of the lights that are of the same type.
	  * Check the size of it against the shader limit */
	size_t thisPassLayers = (num 
	  + lastMetadata.numberOfLights - 1) / lastMetadata.numberOfLights;
	thisPassLayers = csMin (thisPassLayers, layerConfig.GetMaxLightPasses (layer));
	size_t neededLayers = totalLayers + thisPassLayers;

	csShaderVariableStack localStack;
	for (size_t n = 0; n < thisPassLayers; n++)
	{
	  if ((totalLayers != 0) || (n > 0))
	  {
	    /* The first layer will have the shader to use set;
	      * subsequent ones don't */
	    node->owner.CopyLayerShader (mesh.contextLocalId,
              layers.GetNewLayerIndex (layer, 0),
              layers.GetNewLayerIndex (layer, n + totalLayers));
	  }
	  node->owner.svArrays.SetupSVStack (localStack, 
            layers.GetNewLayerIndex (layer, n + totalLayers),
	    mesh.contextLocalId);
  
	  size_t thisNum = csMin (num,
	    layerConfig.GetMaxLightNum (layer));
	  csShaderVariable* lightNum = CreateVarOnStack (
	    persist.svNames.GetDefaultSVId (
	      csLightShaderVarCache::varLightCount), localStack);
	  lightNum->SetValue ((int)thisNum);

	  csShaderVariable* passNum = CreateVarOnStack (
	    persist.svPassNum, localStack);
	  passNum->SetValue ((int)(n + totalLayers));
  
	  csShaderVariable* lightTypeSV = CreateVarOnStack (
	    persist.svNames.GetLightSVId (
	      csLightShaderVarCache::lightType), localStack);
	  lightTypeSV->SetValue ((int)(lightType));
  
	  for (size_t l = thisNum; l-- > 0; )
	  //for (size_t l = 0; l < thisNum; l++)
	  {
	    iLight* light = sortedLights.GetLight (firstLight + l);
	    CachedLightData* thisLightSVs =
	      persist.lightDataCache.GetElementPointer (light);
	    if (thisLightSVs == 0)
	    {
              CachedLightData newCacheData;
              newCacheData.shaderVars =
                &(light->GetSVContext()->GetShaderVariables());
	      thisLightSVs = &persist.lightDataCache.Put (
		light, newCacheData);
	    }
  
	    MergeAsArrayItems (localStack, *(thisLightSVs->shaderVars), l);
	  }
	  num -= thisNum;
	  firstLight += thisNum;
	  remainingLights -= thisNum;
	}

	totalLayers = neededLayers;
      }
    }
  protected:
    PersistentData& persist;
    const LayerConfigType& layerConfig;

    // Simple cache
    iShader* lastShader;
    csShaderMetadata lastMetadata;
    
    inline void UpdateMetadata (iShader* shaderToUse)
    {
      if (shaderToUse != lastShader)
      {
	lastMetadata = shaderToUse->GetMetadata();
	lastShader = shaderToUse;
      }
    }
  };

  /**
   * For each mesh determine the array of affecting lights and generate shader
   * vars for it.
   * Must be done after shader and shader SV (usually SetupStandardShader())
   * and before ticket setup.
   */
  template<typename RenderTree, typename LayerConfigType,
    typename ShadowHandler = ShadowNone<RenderTree, LayerConfigType> >
  class LightSetup
  {
  public:
    struct PersistentData;
    typedef csArray<iShader*> ShaderArrayType;

    LightSetup (PersistentData& persist, iLightManager* lightmgr,
      SVArrayHolder& svArrays, const LayerConfigType& layerConfig)
      : persist (persist), lightmgr (lightmgr), svArrays (svArrays),
        allMaxLights (0), newLayers (layerConfig)
    {
      // Sum up the number of lights we can possibly handle
      for (size_t layer = 0; layer < layerConfig.GetLayerCount (); ++layer)
      {
	const size_t layerMax = layerConfig.GetMaxLightNum (layer);
	// Max lights can be ~0, so need to avoid overflow
	allMaxLights += csMin (layerMax, ((size_t)~0) - allMaxLights);
      }
    }

    void operator() (typename RenderTree::MeshNode* node)
    {
      // The original layers
      const LayerConfigType& layerConfig = newLayers.GetOriginalLayers();

      /* This step will insert layers, keep track of the new indices of
       * the original layer as well as how often a layer has been
       * duplicated */
      LayerHelper<RenderTree, LayerConfigType,
        PostLightingLayers> layerHelper (persist.layerPersist, layerConfig,
        newLayers);
      ShadowHandler shadows (persist.shadowPersist, layerConfig);

      for (size_t i = 0; i < node->meshes.GetSize (); ++i)
      {
        typename RenderTree::MeshNode::SingleMesh& mesh = node->meshes[i];

        size_t numLights;
        csLightInfluence* influences;
	lightmgr->GetRelevantLights (node->owner.sector,
	  mesh.bbox, influences, numLights, allMaxLights);
	if (numLights == 0) continue; // Not much to do ...
        
        size_t lightOffset = 0;
	for (size_t layer = 0; layer < layerConfig.GetLayerCount (); ++layer)
        {
          // Get the subset of lights for this layer
          size_t layerLights = csMin (layerConfig.GetMaxLightNum (layer),
            numLights - lightOffset);
          if (layerLights == 0) continue;
          csLightInfluence* currentInfluences = influences + lightOffset;

          shadows.HandleLights (currentInfluences,
            numLights, layer, layerHelper, mesh, node);
	  lightOffset += layerLights;
	}

        lightmgr->FreeInfluenceArray (influences);
      }
    }

    class PostLightingLayers
    {
      const LayerConfigType& layerConfig;
      csArray<size_t> layerMap;

      friend class LightSetup;
      const LayerConfigType& GetOriginalLayers() const
      {
        return layerConfig;
      }
    public:
      PostLightingLayers (const LayerConfigType& layerConfig)
        : layerConfig (layerConfig)
      {
        layerMap.SetCapacity (layerConfig.GetLayerCount());
        for (size_t l = 0; l < layerConfig.GetLayerCount(); l++)
          layerMap.Push (l);
      }

      size_t GetLayerCount () const
      {
	return layerMap.GetSize();
      }
  
      const csStringID* GetShaderTypes (size_t layer, size_t& num) const
      {
        return layerConfig.GetShaderTypes (layerMap[layer], num);
      }
  
      iShader* GetDefaultShader (size_t layer) const
      {
        return layerConfig.GetDefaultShader (layerMap[layer]);
      }
      
      size_t GetMaxLightNum (size_t layer) const
      {
        return layerConfig.GetMaxLightNum (layerMap[layer]);
      }
  
      size_t GetMaxLightPasses (size_t layer) const
      {
        return layerConfig.GetMaxLightPasses (layerMap[layer]);
      }


      void InsertLayer (size_t after, size_t oldLayer)
      {
        layerMap.Insert (after+1, oldLayer);
      }
    };
    
    const PostLightingLayers& GetPostLightingLayers () const
    {
      return newLayers;
    }

    struct PersistentData
    {
      typename ShadowHandler::PersistentData shadowPersist;
      typename LayerHelper<RenderTree, LayerConfigType,
        PostLightingLayers>::PersistentData layerPersist;
      
      void Initialize (iShaderVarStringSet* strings)
      {
        shadowPersist.Initialize (strings);
      }
      void UpdateNewFrame ()
      {
        shadowPersist.UpdateNewFrame();
      }
    };

  private:
    PersistentData& persist;
    iLightManager* lightmgr;
    SVArrayHolder& svArrays; 
    size_t allMaxLights;
    PostLightingLayers newLayers;
  };

}
}

#endif // __CS_CSPLUGINCOMMON_RENDERMANAGER_LIGHTSETUP_H__
