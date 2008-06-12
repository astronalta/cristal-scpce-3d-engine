/*
    Copyright (C) 2008 by Frank Richter

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

#ifndef __CS_CSPLUGINCOMMON_RENDERMANAGER_HDRHELPER_H__
#define __CS_CSPLUGINCOMMON_RENDERMANAGER_HDRHELPER_H__

#include "iutil/objreg.h"
#include "imap/loader.h"

#include "csplugincommon/rendermanager/posteffects.h"

struct iConfigFile;

namespace CS
{
  namespace RenderManager
  {
    /**
     * To help setting up a post effects manager for HDR rendering.
     *
     * Usage:
     * - The rendermanager must have an instance member for the HDR helper.
     * - The Setup() method must be called.
     * - The HDR helper owns a post effects manager, retrievable with
     *	 GetHDRPostEffects(). It should be chained to the post effects manager
     *   set up by the render manager. (Or, if the RM does not have a post
     *	 effects manager, properly driven like an RM one would.)
     */
    class CS_CRYSTALSPACE_EXPORT HDRHelper
    {
    public:
      /// Level of HDR quality
      enum Quality
      {
        /**
         * Use 8-bit integers: fastest, but can look very, very bad. Use only
         * when desperate
         */
        qualInt8,
        /**
         * Use 10-bit integers: faster and usually looking good enough;
         * range is still rather limited.
         */
        qualInt10,
        /**
         * Use 16-bit integers: fast and usually good enough;
         * range is still limited.
         */
        qualInt16,
        /**
         * Use 16-bit floats: slower than integers, but wider range.
         * Use if you should run into color precision issues with integer.
         */
        qualFloat16,
        /**
         * Use 32-bit floats: slowest, but also highest range and precision.
         * However, rarely needed.
         */
        qualFloat32
      };
    
      /**
       * Set up a post processing effects manager for rendering to HDR 
       * textures.
       * \param objectReg Pointer to the object registry.
       * \param quality Quality of the intermediate textures rendered to.
       * \param colorRange Range of colors for integer texture qualities.
       *   Typical values are 16 for qualInt16 and 4 for qualInt8.
       * \return Whether the setup succeeded.
       * \remarks By default a simple linear tone mapping to the screen
       *   color space is used. This can be changed with SetMappingShader().
       */
      bool Setup (iObjectRegistry* objectReg, 
        Quality quality, int colorRange);

      PostEffectManager& GetHDRPostEffects() { return postEffects; }

      /// Set the shader used for tonemapping the final image.
      void SetMappingShader (iShader* shader);
      /// Get the shader used for tonemapping the final image.
      iShader* GetMappingShader ();
      /// Get the shader variable context for the tonemapping stage.
      iShaderVariableContext* GetMapppingShaderVarContext();
    private:
      PostEffectManager postEffects;
      PostEffectManager::Layer* mappingLayer;
    };
  
    /// Read HDR settings from a config file
    class CS_CRYSTALSPACE_EXPORT HDRSettings
    {
      iConfigFile* config;
      csString prefix;
    public:
      HDRSettings (iConfigFile* config, const char* prefix);
      
      /// Whether HDR rendering should be enabled
      bool IsEnabled();
      /// Get requested quality
      HDRHelper::Quality GetQuality();
      /// Get requested color range
      int GetColorRange();
    };
  
  } // namespace RenderManager
} // namespace CS

#endif // __CS_CSPLUGINCOMMON_RENDERMANAGER_HDRHELPER_H__
