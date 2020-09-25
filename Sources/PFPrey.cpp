//
//  PFPrey.h
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#include "PFPrey.h"
#include "PFWorld.h"

#define MAX_SWIM_SPEED 5.0f

namespace PF
{
	RNDefineMeta(Prey, RN::Entity)

	Prey::Prey(RN::Model *model) : _rotationChangeTimer(0.0f), _rotationSpeed(0.0f), _isHooked(false), _energy(1.0f)
	{
		SetModel(model->Copy());
		GetModel()->SetSkeleton(model->GetSkeleton()->Copy());
		GetModel()->GetSkeleton()->SetAnimation(RNCSTR("swimming"));
		
		RN::PhysXMaterial *physicsMaterial = new RN::PhysXMaterial();
		_physicsBody = RN::PhysXDynamicBody::WithShape(RN::PhysXSphereShape::WithRadius(0.5f, physicsMaterial->Autorelease()), 1.0f);
		_physicsBody->SetCollisionFilter(Types::CollisionPrey, Types::CollisionThreadMask);
		AddAttachment(_physicsBody);
		_physicsBody->SetEnableKinematic(true);
		
		World::GetSharedInstance()->AddPrey();
	}
	
	Prey::~Prey()
	{
		World::GetSharedInstance()->RemovePrey();
	}
	
	void Prey::Update(float delta)
	{
		if(delta > 0.2f)
		{
			delta = 0.2f;
		}
		
		if(!_isHooked)
		{
			_rotationChangeTimer -= delta;
			if(_rotationChangeTimer <= 0.0f)
			{
				_rotationChangeTimer = RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomFloatRange(10.0f, 50.0f);
				_rotationSpeed = RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomFloatRange(-30.0f, 30.0f);
				
				Rotate(RN::Vector3(180.0f + RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomFloatRange(-30.0f, 30.0f), 0.0f, 0.0f));
			}
			
			World *world = World::GetSharedInstance();
			RN::PhysXWorld *physicsWorld = world->GetPhysicsWorld();
			
			if(world->GetPlayer()->GetWorldPosition().GetDistance(GetWorldPosition()) > 200.0f)
			{
				SetWorldPosition(GetWorldPosition() + RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomVector3Range(RN::Vector3(-200.0f, 10.0f, -200.0f), RN::Vector3(200.0f, 10.0f, 200.0f)));
				SetWorldRotation(RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomVector3Range(RN::Vector3(0.0f, 0.0f, 0.0f), RN::Vector3(360.0f, 0.0f, 0.0f)));
				
				const RN::PhysXContactInfo &contact = physicsWorld->CastRay(GetWorldPosition(), GetWorldPosition() - RN::Vector3(0.0f, 10000.0f, 0.0f), Types::CollisionLevel);
				if(contact.position.y < -21.0f && contact.distance > 0.0f)
				{
					SetWorldPosition(contact.position + RN::Vector3(0.0f, RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomFloatRange(10.0f, -21.0f - contact.position.y), 0.0f));
				}
				else
				{
					//Retry placement around player next frame
					SetWorldPosition(RN::Vector3(0.0f, 1000.0f, 0.0f));
				}
			}
			
			GetModel()->GetSkeleton()->Update(delta * 24.0f);
			
			Rotate(RN::Vector3(_rotationSpeed * delta, 0.0f, 0.0f));
			RN::Vector3 movement = GetForward() * MAX_SWIM_SPEED * delta;
			
			std::vector<RN::PhysXContactInfo> contactInfo;
			_physicsBody->SweepTest(contactInfo, GetForward() + movement);
			if(contactInfo.size() > 0)
			{
				if(contactInfo[0].node->IsKindOfClass(Thread::GetMetaClass()))
				{
					Thread *thread = contactInfo[0].node->Downcast<Thread>();
					Catch(GetWorldPosition());
					thread->SetPrey(this);
				}
				
				_rotationChangeTimer = 0.0f;
			}
			else
			{
				_physicsBody->SetKinematicTarget(GetWorldPosition() + movement, GetWorldRotation());
			}
		}
		else
		{
			RN::Vector3 diffToTarget = _targetPosition - GetWorldPosition();
			_physicsBody->ApplyForce(diffToTarget * 10.0f);
			
			_energy = std::max(0.0f, _energy - delta * 0.1f);
			GetModel()->GetSkeleton()->Update(delta * 24.0f * _energy);
		}

		SceneNode::Update(delta);
	}

	void Prey::Catch(RN::Vector3 target)
	{
		if(!_isHooked)
		{
			_isHooked = true;
			_physicsBody->SetEnableKinematic(false);
			_energy = 2.0f;
		}
		
		_targetPosition = target;
	}

	void Prey::ReleasePrey()
	{
		_isHooked = false;
		_physicsBody->SetEnableKinematic(true);
		SetWorldRotation(RN::Vector3(0.0f, 0.0f, 0.0f));
		_rotationChangeTimer = -1.0f;
		
		_energy = 1.0f;
	}
}
