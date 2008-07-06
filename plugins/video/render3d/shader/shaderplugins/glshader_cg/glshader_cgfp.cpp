/*
  Copyright (C) 2002 by Marten Svanfeldt
                        Anders Stenberg

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "cssysdef.h"

#include "csgeom/vector3.h"
#include "csutil/objreg.h"
#include "csutil/ref.h"
#include "csutil/scf.h"
#include "csutil/stringreader.h"
#include "iutil/databuff.h"
#include "iutil/document.h"
#include "iutil/hiercache.h"
#include "iutil/string.h"
#include "ivaria/reporter.h"
#include "ivideo/graph3d.h"
#include "ivideo/shader/shader.h"

#include "glshader_cgfp.h"
#include "profile_limits.h"

CS_PLUGIN_NAMESPACE_BEGIN(GLShaderCg)
{

CS_LEAKGUARD_IMPLEMENT (csShaderGLCGFP);

void csShaderGLCGFP::Activate()
{
  if (pswrap)
  {
    pswrap->Activate ();
    return;
  } 

  csShaderGLCGCommon::Activate();
}

void csShaderGLCGFP::Deactivate()
{
  if (pswrap)
  {
    pswrap->Deactivate ();
    return;
  } 

  csShaderGLCGCommon::Deactivate();
}

void csShaderGLCGFP::SetupState (const CS::Graphics::RenderMesh* mesh,
                                 CS::Graphics::RenderMeshModes& modes,
                                 const csShaderVariableStack& stack)
{
  if (pswrap)
  {
    pswrap->SetupState (mesh, modes, stack);
    return;
  } 

  csShaderGLCGCommon::SetupState (mesh, modes, stack);
}

void csShaderGLCGFP::ResetState()
{
  if (pswrap)
  {
    pswrap->ResetState ();
    return;
  } 
  csShaderGLCGCommon::ResetState();
}

bool csShaderGLCGFP::Compile (iHierarchicalCache* cache)
{
  if (!shaderPlug->enableFP) return false;

  size_t i;

  // See if we want to wrap through the PS plugin
  // (psplg will be 0 if wrapping isn't wanted)
  if (shaderPlug->psplg)
  {
  csRef<iDataBuffer> programBuffer = GetProgramData();
    if (!programBuffer.IsValid())
      return false;
    csString programStr;
    programStr.Append ((char*)programBuffer->GetData(), programBuffer->GetSize());
  
    ArgumentArray args;
    shaderPlug->GetProfileCompilerArgs ("fragment", shaderPlug->psProfile, 
      false, args);
    for (i = 0; i < compilerArgs.GetSize(); i++) 
      args.Push (compilerArgs[i]);
    args.Push (0);
    program = cgCreateProgram (shaderPlug->context, CG_SOURCE,
      programStr, shaderPlug->psProfile, 
      !entrypoint.IsEmpty() ? entrypoint : "main", args.GetArray());

    if (!program)
      return false;

    pswrap = shaderPlug->psplg->CreateProgram ("fp");

    if (!pswrap)
      return false;

    csArray<csShaderVarMapping> mappings;
    
    for (i = 0; i < variablemap.GetSize (); i++)
    {
      // Get the Cg parameter
      CGparameter parameter = cgGetNamedParameter (
	program, variablemap[i].destination);
      // Check if it's found, and just skip it if not.
      if (!parameter)
        continue;
      if (!cgIsParameterReferenced (parameter))
	continue;
      // Make sure it's a C-register
      CGresource resource = cgGetParameterResource (parameter);
      if (resource == CG_C)
      {
        // Get the register number, and create a mapping
        csString regnum;
        regnum.Format ("c%lu", cgGetParameterResourceIndex (parameter));
        mappings.Push (csShaderVarMapping (variablemap[i].name, regnum));
      }
    }

    csRef<iHierarchicalCache> ps1cache;
    if (pswrap->Load (0, cgGetProgramString (program, CG_COMPILED_PROGRAM), 
      mappings))
    {
      if (cache != 0) ps1cache = cache->GetRootedCache ("/ps1/");
      bool ret = pswrap->Compile (ps1cache);
      if (shaderPlug->debugDump)
        DoDebugDump();
      return ret;
    }
    else
    {
      return false;
    }
  }
  else
  {
    bool ret = TryCompile (shaderPlug->maxProfileFragment,
      loadLoadToGL | loadApplyVmap, 0);
  
    ProfileLimits limits (programProfile);
    limits.GetCurrentLimits ();
    WriteToCache (cache, limits);
    return ret;
  }

  return true;
}

bool csShaderGLCGFP::Precache (const ProfileLimits& limits, 
                               iHierarchicalCache* cache)
{

  bool ret = TryCompile (CG_PROFILE_UNKNOWN,
    loadApplyVmap | loadIgnoreConfigProgramOpts, &limits);

  WriteToCache (cache, limits);

  return ret;
}

bool csShaderGLCGFP::TryCompile (CGprofile maxFrag, uint loadFlags,
                                 const ProfileLimits* limits)
{
  csRef<iDataBuffer> programBuffer = GetProgramData();
  if (!programBuffer.IsValid())
    return false;
  csString programStr;
  programStr.Append ((char*)programBuffer->GetData(), programBuffer->GetSize());

  csStringArray testForUnused;
  csStringReader lines (programStr);
  while (lines.HasMoreLines())
  {
    csString line;
    lines.GetLine (line);
    if (line.StartsWith ("//@@UNUSED? "))
    {
      line.DeleteAt (0, 12);
      testForUnused.Push (line);
    }
  }

  if (testForUnused.GetSize() > 0)
  {
    /* A list of unused variables to test for has been given. Test piecemeal
      * which variables are really unused */
    csSet<csString> allNewUnusedParams;
    size_t step = 8;
    size_t offset = 0;
    while (offset < testForUnused.GetSize())
    {
      unusedParams.DeleteAll();
      for (size_t i = 0; i < offset; i++)
	unusedParams.Add (testForUnused[i]);
      for (size_t i = offset+step; i < testForUnused.GetSize(); i++)
	unusedParams.Add (testForUnused[i]);
      if (DefaultLoadProgram (0, programStr, CG_GL_FRAGMENT, 
	maxFrag, 
	loadIgnoreErrors | (loadFlags & loadIgnoreConfigProgramOpts),
	limits))
      {
	csSet<csString> newUnusedParams;
	CollectUnusedParameters (newUnusedParams);
	for (size_t i = 0; i < step; i++)
	{
	  if (offset+i >= testForUnused.GetSize()) break;
	  const char* s = testForUnused[offset+i];
	  if (newUnusedParams.Contains (s))
	    allNewUnusedParams.Add (s);
	}
      }
      
      offset += step;
    }
    unusedParams = allNewUnusedParams;
  }
  else
  {
    unusedParams.DeleteAll();
  }
  if (!DefaultLoadProgram (0, programStr, CG_GL_FRAGMENT, 
    maxFrag, loadFlags, limits))
    return false;
  /* Compile twice to be able to filter out unused vertex2fragment stuff on 
    * pass 2.
    * @@@ FIXME: two passes are not always needed.
    */
  CollectUnusedParameters (unusedParams);
  bool ret = DefaultLoadProgram (this, programStr, CG_GL_FRAGMENT, 
    maxFrag, loadFlags, limits);
    
  return ret;
}

int csShaderGLCGFP::ResolveTU (const char* binding)
{
  int newTU = -1;
  if (program)
  {
    CGparameter parameter = cgGetNamedParameter (program, binding);
    if (parameter)
    {
      CGresource baseRes = cgGetParameterBaseResource (parameter);
      if (((baseRes == CG_TEXUNIT0) || (baseRes == CG_TEX0))
          && (cgIsParameterUsed (parameter, program)
            || cgIsParameterReferenced (parameter)))
      {
	newTU = cgGetParameterResourceIndex (parameter);
      }
    }
  }

  return newTU;
}

}
CS_PLUGIN_NAMESPACE_END(GLShaderCg)
