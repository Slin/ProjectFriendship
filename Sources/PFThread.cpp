//
//  PFThread.h
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#include "PFThread.h"
#include "PFWorld.h"

namespace PF
{
	RNDefineMeta(Thread, RN::Entity)

	Thread::Thread() : _floatingTimer(0.0f), _stickyTimer{0.0f, 0.0f}, _isAnchored{true, true}, _physicsBody(nullptr), _isOwnedByPlayer(true)
	{
		World *world = World::GetSharedInstance();
		RN::Model *model = world->AssignShader(RN::Model::WithName(RNCSTR("models/thread.sgm")), Types::MaterialDefault);
		SetModel(model);
		
		SetPriority(Priority::UpdateLate);
	}
	
	Thread::~Thread()
	{
		
	}
	
	void Thread::Update(float delta)
	{
		if(delta > 0.2f)
		{
			delta = 0.2f;
		}
		
		RN::PhysXWorld *physicsWorld = World::GetSharedInstance()->GetPhysicsWorld();
		for(int i = 0; i < 2; i++)
		{
			_stickyTimer[i] += delta;
			if(!_isAnchored[i] && _stickyTimer[i] < 2.0f)
			{
				const RN::PhysXContactInfo &contact = physicsWorld->CastRay(_position[i], _position[i] + _movement[i] * delta, Types::CollisionAll);
				if(contact.distance >= 0.0f)
				{
					_isAnchored[i] = true;
					_movement[i] = RN::Vector3();
					_position[i] = contact.position;
				}
			}
		}
		
		_position[0] += _movement[0] * delta;
		_position[1] += _movement[1] * delta;
		
		_movement[0] *= std::max((1.0f - delta * 1.0f), 0.0f);
		_movement[1] *= std::max((1.0f - delta * 1.0f), 0.0f);
		
		SetWorldPosition(_position[0]);
		LookAt(_position[1], false);
		SetScale(RN::Vector3(1.0f, 1.0f, _position[0].GetDistance(_position[1])));
		
		if(!_isOwnedByPlayer && _isAnchored[0] && _isAnchored[1] && !_physicsBody && GetScale().z > RN::k::EpsilonFloat)
		{
			RN::PhysXMaterial *physicsMaterial = new RN::PhysXMaterial();
			_physicsBody = RN::PhysXStaticBody::WithShape(RN::PhysXCapsuleShape::WithRadius(0.04f, GetScale().z, physicsMaterial->Autorelease()));
			_physicsBody->SetCollisionFilter(Types::CollisionThread, Types::CollisionAll);
			_physicsBody->SetPositionOffset(RN::Vector3(0.0f, 0.0f, GetScale().z * 0.5f));
			_physicsBody->SetRotationOffset(RN::Vector3(90.0f, 90.0f, 0.0f));
			AddAttachment(_physicsBody);
		}
		
		if(!_isOwnedByPlayer && (!_isAnchored[0] || !_isAnchored[1]))
		{
			_floatingTimer += delta;
			if(_floatingTimer > 5.0f)
			{
				Destroy();
			}
		}

		SceneNode::Update(delta);
	}

	void Thread::SetPosition(RN::Vector3 position, bool isEnd)
	{
		if(isEnd)
		{
			_position[1] = position;
		}
		else
		{
			_position[0] = position;
		}
	}

	void Thread::Shoot(RN::Vector3 speed, bool isEnd)
	{
		if(isEnd)
		{
			_stickyTimer[1] = 0.0f;
			_movement[1] = speed;
			_isAnchored[1] = false;
			_isOwnedByPlayer = false;
			_floatingTimer = 0.0f;
		}
		else
		{
			_stickyTimer[0] = 0.0f;
			_movement[0] = speed;
			_isAnchored[0] = false;
		}
	}

	void Thread::Destroy()
	{
		World::GetSharedInstance()->GetPlayer()->ResetThread(this);
		World::GetSharedInstance()->RemoveLevelNode(this);
	}
}
