/*
  Copyright (C) 2010 Christian Van Brussel, Communications and Remote
      Sensing Laboratory of the School of Engineering at the 
      Universite catholique de Louvain, Belgium
      http://www.tele.ucl.ac.be

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
#include "sintel.h"

#define EXPRESSION_TRANSITION_DURATION 250.0f

SintelScene::SintelScene (AvatarTest* avatarTest)
  : avatarTest (avatarTest), activeFacialExpression (0),
    currentExpressionIndex (0), activeFacialTransition (false)
{
}

SintelScene::~SintelScene ()
{
  if (!animesh)
    return;

  // Remove the mesh from the scene
  csRef<iMeshObject> animeshObject = scfQueryInterface<iMeshObject> (animesh);
  avatarTest->engine->RemoveObject (animeshObject->GetMeshWrapper ());
}

csVector3 SintelScene::GetCameraStart ()
{
  return csVector3 (0.0f, 0.08f, -0.5f);
}

csVector3 SintelScene::GetCameraTarget ()
{
  return csVector3 (0.0f, 0.15f, 0.0f);
}

float SintelScene::GetSimulationSpeed ()
{
  return 1.0f;
}

void SintelScene::Frame ()
{
  // Update the facial expression if we are still in the transition
  if (activeFacialTransition)
  {
    csTicks currentTime = avatarTest->vc->GetCurrentTicks ();

    bool allTargetReached = true;
    for (int index = ((int) activeMorphComponents.GetSize ()) - 1; index >= 0; index--)
    {
      ActiveMorphComponent& activeMorphComponent = activeMorphComponents.Get (index);

      // Check if the transition is over
      float timeRatio;
      bool targetReached = false;
      if (currentTime >= activeMorphComponent.targetTime)
      {
	timeRatio = 1.0f;
	targetReached = true;
      }

      // Else compute the new time ratio
      else
      {
	allTargetReached = false;
	timeRatio = 1.0f - ((float) (activeMorphComponent.targetTime - currentTime))
	  / EXPRESSION_TRANSITION_DURATION;
      }

      // Compute the new weight
      activeMorphComponent.currentWeight =
	activeMorphComponent.startWeight
	+ (activeMorphComponent.targetWeight - activeMorphComponent.startWeight) * timeRatio;

      // Apply the new weight
      animesh->SetMorphTargetWeight (activeMorphComponent.morphTarget,
				     activeMorphComponent.currentWeight);

      // Check if the component can be removed
      //if (targetExpressionReached
      if (targetReached
	  && activeMorphComponent.targetWeight < SMALL_EPSILON)
	activeMorphComponents.DeleteIndex (index);
    }

    // Update the target reached status
    activeFacialTransition = !allTargetReached;
  }
}

bool SintelScene::OnKeyboard (iEvent &ev)
{
  csKeyEventType eventtype = csKeyEventHelper::GetEventType(&ev);
  if (eventtype == csKeyEventTypeDown)
  {
    // Changing facial expression
    int newExpressionIndex = -1;
    if (csKeyEventHelper::GetCookedCode (&ev) == '1')
      newExpressionIndex = 0;

    else if (csKeyEventHelper::GetCookedCode (&ev) == '2')
      newExpressionIndex = 1;

    else if (csKeyEventHelper::GetCookedCode (&ev) == '3')
      newExpressionIndex = 2;

    else if (csKeyEventHelper::GetCookedCode (&ev) == '4')
      newExpressionIndex = 3;

    if (newExpressionIndex != -1
	&& newExpressionIndex != currentExpressionIndex)
    {
      // Update the active expression
      currentExpressionIndex = newExpressionIndex;

      // Setup the new target time of the facial expression
      csTicks targetExpressionTime = avatarTest->vc->GetCurrentTicks ()
	+ (int) EXPRESSION_TRANSITION_DURATION;
      activeFacialTransition = true;

      // Cancel the active facial expression
      if (activeFacialExpression)
      {
	for (csArray<MorphComponent>::Iterator it =
	       activeFacialExpression->morphComponents.GetIterator (); it.HasNext (); )
	{
	  MorphComponent& component = it.Next ();

	  // Find the corresponding active component
	  ActiveMorphComponent* activeComponent;
	  for (csArray<ActiveMorphComponent>::Iterator activeIt =
		 activeMorphComponents.GetIterator (); activeIt.HasNext (); )
	  {
	    activeComponent = &activeIt.Next ();
	    if (activeComponent->morphTarget == component.morphTarget)
	      break;
	  }

	  // Reset the target weight of this component
	  if (activeComponent->targetWeight > SMALL_EPSILON)
	  {
	    activeComponent->startWeight = activeComponent->currentWeight;
	    activeComponent->targetWeight = 0.0f;
	    activeComponent->targetTime = targetExpressionTime;
	  }
	}
      }

      // If the target expression is the 'neutral', then it is over
      if (currentExpressionIndex == 0)
      {
	activeFacialExpression = 0;
	return true;
      }

      // Setup the morph components of the new facial expression
      activeFacialExpression = &facialExpressions.Get (newExpressionIndex - 1);
      for (csArray<MorphComponent>::Iterator it =
	     activeFacialExpression->morphComponents.GetIterator (); it.HasNext (); )
      {
	MorphComponent& component = it.Next ();

	// Check if this component is already active
	ActiveMorphComponent* activeComponent;
	bool componentFound = false;
	for (csArray<ActiveMorphComponent>::Iterator activeIt =
	       activeMorphComponents.GetIterator (); activeIt.HasNext (); )
	{
	  activeComponent = &activeIt.Next ();

	  if (activeComponent->morphTarget == component.morphTarget)
	  {
	    componentFound = true;
	    break;
	  }
	}

	// Else create a new active component
	if (!componentFound)
	{
	  activeComponent = new ActiveMorphComponent ();
	  activeComponent->morphTarget = component.morphTarget;
	  activeComponent->currentWeight = 0.0f;
	  size_t index = activeMorphComponents.Push (*activeComponent);
	  delete activeComponent;
	  activeComponent = &activeMorphComponents.Get (index);
	}

	// Set the new target weight of the component
	activeComponent->startWeight = activeComponent->currentWeight;
	activeComponent->targetWeight = component.weight;
	activeComponent->targetTime = targetExpressionTime;
      }

      return true;
    }

    // Reset of the scene
    if (csKeyEventHelper::GetCookedCode (&ev) == 'r')
    {
      ResetScene ();
      return true;
    }
  }

  return false;
}

bool SintelScene::OnMouseDown (iEvent &ev)
{
  return false;
}

bool SintelScene::CreateAvatar ()
{
  printf ("Loading Sintel...\n");

  // Load animesh factory
  csLoadResult rc = avatarTest->loader->Load ("/lib/sintel/sintel.xml");
  if (!rc.success)
    return avatarTest->ReportError ("Can't load Sintel library file!");

  csRef<iMeshFactoryWrapper> meshfact =
    avatarTest->engine->FindMeshFactory ("sintel_head");
  if (!meshfact)
    return avatarTest->ReportError ("Can't find Sintel's mesh factory!");

  animeshFactory = scfQueryInterface<iAnimatedMeshFactory>
    (meshfact->GetMeshObjectFactory ());
  if (!animeshFactory)
    return avatarTest->ReportError ("Can't find Sintel's animesh factory!");

  // Create a new animation tree. The structure of the tree is:
  //   + idle animation node (root and only node)
  csRef<iSkeletonAnimPacketFactory2> animPacketFactory =
    animeshFactory->GetSkeletonFactory ()->GetAnimationPacket ();

  // Create the 'idle01' animation node
  csRef<iSkeletonAnimationNodeFactory2> idle01NodeFactory =
    animPacketFactory->CreateAnimationNode ("idle01");
  idle01NodeFactory->SetAnimation
    (animPacketFactory->FindAnimation ("idle01"));
  animPacketFactory->SetAnimationRoot (idle01NodeFactory);

  idle01NodeFactory->SetAutomaticReset (true);
  idle01NodeFactory->SetAutomaticStop (false);

  // Create the facial expressions
  FacialExpression expression;
  MorphComponent component;

  // Expression 1
  component.morphTarget = animeshFactory->FindMorphTarget ("MOUTH-e.R");
  component.weight = 0.5f;
  expression.morphComponents.Push (component);

  component.morphTarget = animeshFactory->FindMorphTarget ("CHEEK-out.R");
  component.weight = 0.5f;
  expression.morphComponents.Push (component);

  facialExpressions.Push (expression);

  // Expression 2
  expression.morphComponents.DeleteAll ();
  component.morphTarget = animeshFactory->FindMorphTarget ("MOUTH-e.L");
  component.weight = 1.0f;
  expression.morphComponents.Push (component);

  component.morphTarget = animeshFactory->FindMorphTarget ("CHEEK-out.L");
  component.weight = 1.0f;
  expression.morphComponents.Push (component);

  facialExpressions.Push (expression);

  // Expression 3
  expression.morphComponents.DeleteAll ();
  component.morphTarget = animeshFactory->FindMorphTarget ("BROW-surp.L");
  component.weight = 1.0f;
  expression.morphComponents.Push (component);

  component.morphTarget = animeshFactory->FindMorphTarget ("BROW-mad.L");
  component.weight = 1.0f;
  expression.morphComponents.Push (component);

  component.morphTarget = animeshFactory->FindMorphTarget ("BROW-inner_up.L");
  component.weight = 1.0f;
  expression.morphComponents.Push (component);

  component.morphTarget = animeshFactory->FindMorphTarget ("BROW-outer_up.L");
  component.weight = 1.0f;
  expression.morphComponents.Push (component);

  component.morphTarget = animeshFactory->FindMorphTarget ("BROW-surp.R");
  component.weight = 1.0f;
  expression.morphComponents.Push (component);

  component.morphTarget = animeshFactory->FindMorphTarget ("BROW-mad.R");
  component.weight = 1.0f;
  expression.morphComponents.Push (component);

  component.morphTarget = animeshFactory->FindMorphTarget ("BROW-inner_up.R");
  component.weight = 1.0f;
  expression.morphComponents.Push (component);

  component.morphTarget = animeshFactory->FindMorphTarget ("BROW-outer_up.R");
  component.weight = 1.0f;
  expression.morphComponents.Push (component);

  facialExpressions.Push (expression);

  // Create the animated mesh
  csRef<iMeshWrapper> avatarMesh =
    avatarTest->engine->CreateMeshWrapper (meshfact, "sintel",
					   avatarTest->room, csVector3 (0.0f));
  animesh = scfQueryInterface<iAnimatedMesh> (avatarMesh->GetMeshObject ());

  // Start animation
  iSkeletonAnimNode2* rootNode =
    animesh->GetSkeleton ()->GetAnimationPacket ()->GetAnimationRoot ();
  rootNode->Play ();

  // Reset the scene so as to put the parameters of the animation nodes in a default state
  ResetScene ();

  return true;
}

void SintelScene::ResetScene ()
{
}

void SintelScene::DisplayKeys ()
{
  int x = 20;
  int y = 20;
  int fg = avatarTest->g2d->FindRGB (255, 150, 100);
  int lineSize = 18;

  // Write available keys
  avatarTest->WriteShadow (x - 5, y, fg, "Keys available:");
  y += lineSize;

  avatarTest->WriteShadow (x, y, fg, "1: set neutral");
  y += lineSize;

  avatarTest->WriteShadow (x, y, fg, "2: set expression 1");
  y += lineSize;

  avatarTest->WriteShadow (x, y, fg, "3: set expression 2");
  y += lineSize;

  avatarTest->WriteShadow (x, y, fg, "4: set expression 3");
  y += lineSize;

  avatarTest->WriteShadow (x, y, fg, "r: reset scene");
  y += lineSize;

  avatarTest->WriteShadow (x, y, fg, "n: switch to next scene");
  y += lineSize;

  // Write FPS and other info
  y = 480;
  csTicks elapsed_time = avatarTest->vc->GetElapsedTicks ();
  const float speed = elapsed_time / 1000.0f;
  if (speed != 0.0f)
  {
    avatarTest->WriteShadow (x, y, fg, "FPS: %.2f", 1.0f / speed);
    y += lineSize;
  }
}
