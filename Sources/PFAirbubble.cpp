//
//  PFGoldfish.h
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#include "PFGoldfish.h"
#include "PFWorld.h"

#define MAX_SWIM_SPEED 50.0f

namespace PF
{
	RNDefineMeta(Goldfish, RN::Entity)

	Goldfish::Goldfish(RN::Model *model) : _rotationChangeTimer(0.0f), _rotationSpeed(0.0f)
	{
		SetModel(World::GetSharedInstance()->MakeDeepCopy(model));
		model->GetSkeleton()->SetAnimation(RNCSTR("swimming"));
	}
	
	Goldfish::~Goldfish()
	{
		
	}
	
	void Goldfish::Update(float delta)
	{
		if(delta > 0.2f)
		{
			delta = 0.2f;
		}
		
		_rotationChangeTimer -= delta;
		if(_rotationChangeTimer <= 0.0f)
		{
			_rotationChangeTimer = RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomFloatRange(10.0f, 50.0f);
			_rotationSpeed = RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomFloatRange(-30.0f, 30.0f);
			
			Rotate(RN::Vector3(180.0f + RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomFloatRange(-30.0f, 30.0f), 0.0f, 0.0f));
		}
		
		World *world = World::GetSharedInstance();
		RN::PhysXWorld *physicsWorld = world->GetPhysicsWorld();
		
		GetModel()->GetSkeleton()->Update(delta * 12.0f);
		
		Rotate(RN::Vector3(_rotationSpeed * delta, 0.0f, 0.0f));
		RN::Vector3 movement = -GetForward() * MAX_SWIM_SPEED * delta;
		
		const RN::PhysXContactInfo &hitInfo = physicsWorld->CastRay(GetWorldPosition(), GetWorldPosition() - GetForward() * 40.0f + movement);
		if(hitInfo.distance >= 0.0f)
		{
			_rotationChangeTimer = 0.0f;
		}
		else
		{
			Translate(movement);
		}

		SceneNode::Update(delta);
	}
}
