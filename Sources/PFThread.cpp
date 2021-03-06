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

	Thread::Thread() : _floatingTimer(0.0f), _stickyTimer{0.0f, 0.0f}, _isAnchored{true, true}, _physicsBody(nullptr), _isOwnedByPlayer(true), _anchoredPrey(nullptr)
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
				const RN::PhysXContactInfo &contact = physicsWorld->CastRay(_position[i], _position[i] + _movement[i] * delta, Types::CollisionThreadMask);
				if(contact.distance >= 0.0f)
				{
					RN::OpenALSource *source = new RN::OpenALSource(RN::AudioAsset::WithName(RNCSTR("audio/impact.ogg")));
					source->SetSelfdestruct(true);
					source->SetPitch(RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomFloatRange(0.7, 1.3));
					source->SetWorldPosition(_position[i] + _movement[i] * delta);
					World::GetSharedInstance()->AddNode(source->Autorelease());
					source->Play();
					
					_isAnchored[i] = true;
					_movement[i] = RN::Vector3();
					_position[i] = contact.position;
					
					if(contact.node && contact.node->IsKindOfClass(Prey::GetMetaClass()))
					{
						Prey *prey = contact.node->Downcast<Prey>();
						_anchoredPrey = prey;
					}
				}
			}
		}
		
		_position[0] += _movement[0] * delta;
		_position[1] += _movement[1] * delta;
		
		_movement[0] *= std::max((1.0f - delta * 1.0f), 0.0f);
		_movement[1] *= std::max((1.0f - delta * 1.0f), 0.0f);
		
		if(_anchoredPrey)
		{
			_position[0] = _anchoredPrey->GetWorldPosition();
			_anchoredPrey->Catch(_position[1], this);
		}
		
		SetWorldPosition(_position[0]);
		LookAt(_position[1], false);
		SetScale(RN::Vector3(1.0f, 1.0f, _position[0].GetDistance(_position[1])));
		
		if(!_isOwnedByPlayer && _isAnchored[0] && _isAnchored[1] && !_physicsBody && GetScale().z > RN::k::EpsilonFloat && !_anchoredPrey)
		{
			RN::PhysXMaterial *physicsMaterial = new RN::PhysXMaterial();
			_physicsBody = RN::PhysXStaticBody::WithShape(RN::PhysXCapsuleShape::WithRadius(0.04f, GetScale().z, physicsMaterial->Autorelease()));
			_physicsBody->SetCollisionFilter(Types::CollisionThread, Types::CollisionAll);
			_physicsBody->SetPositionOffset(RN::Vector3(0.0f, 0.0f, GetScale().z * 0.5f));
			_physicsBody->SetRotationOffset(RN::Vector3(90.0f, 90.0f, 0.0f));
			AddAttachment(_physicsBody);
		}
		
		if(!_isOwnedByPlayer && ((!_isAnchored[0] || !_isAnchored[1]) || (_anchoredPrey && _isAnchored[1])))
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
			
			RN::OpenALSource *source = new RN::OpenALSource(RN::AudioAsset::WithName(RNCSTR("audio/shoot.ogg")));
			source->SetSelfdestruct(true);
			source->SetPitch(RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomFloatRange(0.7, 1.3));
			source->SetWorldPosition(_position[1]);
			World::GetSharedInstance()->AddNode(source->Autorelease());
			source->Play();
		}
		else
		{
			_stickyTimer[0] = 0.0f;
			_movement[0] = speed;
			_isAnchored[0] = false;
			
			RN::OpenALSource *source = new RN::OpenALSource(RN::AudioAsset::WithName(RNCSTR("audio/shoot.ogg")));
			source->SetSelfdestruct(true);
			source->SetPitch(RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomFloatRange(0.7, 1.3));
			source->SetWorldPosition(_position[0]);
			World::GetSharedInstance()->AddNode(source->Autorelease());
			source->Play();
		}
	}

	void Thread::Destroy()
	{
		if(_anchoredPrey) _anchoredPrey->ReleasePrey();
		World::GetSharedInstance()->GetPlayer()->ResetThread(this);
		World::GetSharedInstance()->RemoveLevelNode(this);
	}

	void Thread::SetPrey(Prey *prey)
	{
		if(!_isAnchored[0] && _stickyTimer[0] < 2.0f)
		{
			_anchoredPrey = prey;
		}
		
		if(!prey)
		{
			_anchoredPrey = prey;
		}
	}
}
