/*
    Copyright (C) 2004 by Jorrit Tyberghein
	      (C) 2004 by Frank Richter

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

#ifndef __CS_SHADERPLUGINS_COMMON_SHADERPROGRAM_H__
#define __CS_SHADERPLUGINS_COMMON_SHADERPROGRAM_H__

/**\file
 * Base class for iShaderProgram plugins.
 */

#include "csextern.h"
#include "csgfx/shadervararrayhelper.h"
#include "csutil/array.h"
#include "csutil/dirtyaccessarray.h"
#include "csutil/leakguard.h"
#include "csutil/ref.h"
#include "csutil/scf_implementation.h"
#include "csutil/strhash.h"
#include "imap/services.h"
#include "iutil/document.h"
#include "iutil/strset.h"
#include "iutil/vfs.h"

#include "csplugincommon/shader/shaderplugin.h"

struct iDataBuffer;
struct iObjectRegistry;

/* Hack to have the Jam dependency scanner pick up shaderprogram.tok.
 * Since the enums generated by this .tok are intended to be used by
 * descendants of csShaderProgram as well, this approach instead of the usual 
 * "Includes" one was taken here since it also plays well with external 
 * projects, and a dependency on the .tok file is picked up as well. */
#define CS_TOKEN_LIST_TOKEN(x)
#include "csplugincommon/shader/shaderprogram.tok"
#undef CS_TOKEN_LIST_TOKEN

/**\addtogroup plugincommon
 * @{ */
/**
 * Base class for iShaderProgram plugins.
 * Provides basic services such as holding and of parameter mapping
 * information, basic program data and data dumping.
 */
class CS_CRYSTALSPACE_EXPORT csShaderProgram : 
  public scfImplementation2<csShaderProgram, 
			    iShaderProgram, 
			    iShaderDestinationResolver>
{
protected:
  csStringHash commonTokens;
#define CS_INIT_TOKEN_TABLE_NAME InitCommonTokens
#define CS_TOKEN_ITEM_FILE \
  "csplugincommon/shader/shaderprogram.tok"
#include "cstool/tokenlist.h"
#undef CS_TOKEN_ITEM_FILE
#undef CS_INIT_TOKEN_TABLE_NAME

protected:
  iObjectRegistry* objectReg;
  csRef<iSyntaxService> synsrv;
  csRef<iShaderVarStringSet> stringsSvName;

public:
  /**
   * Expected/accepted types for a program parameter 
   */
  enum ProgramParamType
  {
    ParamInvalid    = 0,
    ParamInt	    = 0x0001,
    ParamFloat	    = 0x0002,
    ParamVector2    = 0x0004,
    ParamVector3    = 0x0008,
    ParamVector4    = 0x0010,
    ParamMatrix	    = 0x0020,
    ParamTransform  = 0x0040,
    ParamArray      = 0x0080,
    ParamShaderExp  = 0x0100,
    
    ParamVector     = ParamInt | ParamFloat | ParamVector2 | ParamVector3 | ParamVector4
  };

  /**
   * Program parameter, either a SV reference or a const value 
   */
  struct CS_CRYSTALSPACE_EXPORT ProgramParam
  {
    bool valid;
    
    // Name of SV to use (if any)
    CS::ShaderVarStringID name;
    csDirtyAccessArray<size_t, csArrayElementHandler<size_t>,
      CS::Memory::LocalBufferAllocator<size_t, 2,
	CS::Memory::AllocatorMalloc, true> > indices;
    // Reference to const value shadervar
    csRef<csShaderVariable> var;

    ProgramParam() : valid (false), name (CS::InvalidShaderVarStringID) { }
    /// Returns whether this parameter 
    bool IsConstant() const { return valid && var.IsValid(); }
    
    //@{
    /// Set to a constant value
    void SetValue (float val);
    void SetValue (const csVector4& val);
    //@}
  };

  class CS_CRYSTALSPACE_EXPORT ProgramParamParser
  {
    iSyntaxService* synsrv;
    iShaderVarStringSet* stringsSvName;
  public:
    ProgramParamParser (iSyntaxService* synsrv,
      iShaderVarStringSet* stringsSvName) : synsrv (synsrv),
      stringsSvName (stringsSvName) {}

    /**
     * Parse program parameter node.
     * \param node Node to parse.
     * \param param Output program parameter.
     * \param types Combination of ProgramParamType flags, specifying the 
     *   allowed parameter types.
     */
    bool ParseProgramParam (iDocumentNode* node,
      ProgramParam& param, uint types = ~0);
  };

protected:
  /**
   * Parse program parameter node
   * \sa ProgramParamParser::ParseProgramParam
   */
  bool ParseProgramParam (iDocumentNode* node,
    ProgramParam& param, uint types = ~0)
  {
    ProgramParamParser parser (synsrv, stringsSvName);
    return parser.ParseProgramParam (node, param, types);
  }

  /**
   * Holder of variable mapping 
   */
  struct VariableMapEntry : public csShaderVarMapping
  {
    ProgramParam mappingParam;
    intptr_t userVal;

    VariableMapEntry (CS::ShaderVarStringID s, const char* d) :
      csShaderVarMapping (s, d)
    { 
      userVal = 0;
      mappingParam.name = s;
      mappingParam.valid = true;
    }
    VariableMapEntry (const csShaderVarMapping& other) :
      csShaderVarMapping (other.name, other.destination)
    {
      userVal = 0;
      mappingParam.name = other.name;
      mappingParam.valid = true;
    }
  };
  /// Variable mappings
  csSafeCopyArray<VariableMapEntry> variablemap;

  void TryAddUsedShaderVarName (CS::ShaderVarStringID name, csBitArray& bits) const
  {
    if (name != CS::InvalidShaderVarStringID)
    {
      if (bits.GetSize() > name) bits.SetBit (name);
    }
  }
  void TryAddUsedShaderVarProgramParam (const ProgramParam& param, 
    csBitArray& bits) const
  {
    if (param.valid)
    {
      TryAddUsedShaderVarName (param.name, bits);
    }
  }
  void GetUsedShaderVarsFromVariableMappings (csBitArray& bits) const;

  /// Program description
  csString description;

  /// iDocumentNode the program is loaded from
  csRef<iDocumentNode> programNode;
  /// File the program is loaded from (if any)
  csRef<iFile> programFile;

  /// Filename of program
  csString programFileName;
  
  /**
   * Whether the shader program should report additional information during
   * runtime.
   */
  bool doVerbose;

  /// Parse common properties and variablemapping
  bool ParseCommon (iDocumentNode* child);
  /// Get the program node
  iDocumentNode* GetProgramNode ();
  /// Get the raw program data
  csPtr<iDataBuffer> GetProgramData ();

  /// Dump all program info to output
  void DumpProgramInfo (csString& output);
  /// Dump variable mapping
  void DumpVariableMappings (csString& output);

  /**
   * Resolve the SV of a ProgramParam
   */
  inline csShaderVariable* GetParamSV (const csShaderVariableStack& stack, 
    const ProgramParam &param)
  {
    csShaderVariable* var = 0;
  
    var = csGetShaderVariableFromStack (stack, param.name);
    if (var)
      var = CS::Graphics::ShaderVarArrayHelper::GetArrayItem (var,
        param.indices.GetArray(), param.indices.GetSize(),
        CS::Graphics::ShaderVarArrayHelper::maFail);
    if (!var)
      var = param.var;
  
    return var;
  }
  //@{
  /**
   * Query the value of a ProgramParam variable by reading the constant or
   * resolving the shader variable.
   */
  inline bool GetParamVectorVal (const csShaderVariableStack& stack, 
    const ProgramParam &param, csVector4* result)
  {
    csShaderVariable* var (GetParamSV (stack, param));
    
    // If var is null now we have no const nor any passed value, ignore it
    if (!var)
      return false;
  
    var->GetValue (*result);
    return true;
  }
  inline csVector4 GetParamVectorVal (const csShaderVariableStack& stack, 
    const ProgramParam &param, const csVector4& defVal)
  {
    csVector4 v;
    if (!GetParamVectorVal (stack, param, &v)) return defVal;
    return v;
  }
  
  inline bool GetParamTransformVal (const csShaderVariableStack& stack, 
    const ProgramParam &param, csReversibleTransform* result)
  {
    csShaderVariable* var (GetParamSV (stack, param));
    
    // If var is null now we have no const nor any passed value, ignore it
    if (!var)
      return false;
  
    var->GetValue (*result);
    return true;
  }
  inline csReversibleTransform GetParamTransformVal (const csShaderVariableStack& stack, 
    const ProgramParam &param, const csReversibleTransform& defVal)
  {
    csReversibleTransform t;
    if (!GetParamTransformVal (stack, param, &t)) return defVal;
    return t;
  }
  
  inline bool GetParamFloatVal (const csShaderVariableStack& stack, 
    const ProgramParam &param, float* result)
  {
    csShaderVariable* var (GetParamSV (stack, param));
    
    // If var is null now we have no const nor any passed value, ignore it
    if (!var)
      return false;
  
    var->GetValue (*result);
    return true;
  }
  inline float GetParamFloatVal (const csShaderVariableStack& stack, 
    const ProgramParam &param, float defVal)
  {
    float f;
    if (!GetParamFloatVal (stack, param, &f)) return defVal;
    return f;
  }
  //@}
public:
  CS_LEAKGUARD_DECLARE (csShaderProgram);

  csShaderProgram (iObjectRegistry* objectReg);
  virtual ~csShaderProgram ();

  virtual int ResolveTU (const char* /*binding*/)
  { return -1; }

  virtual csVertexAttrib ResolveBufferDestination (const char* /*binding*/)
  { return CS_VATTRIB_INVALID; }

  virtual void GetUsedShaderVars (csBitArray& bits) const;
};

/** @} */

#endif // __CS_SHADERPLUGINS_COMMON_SHADERPROGRAM_H__
