//
//  PFAirbubble.h
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#include "PFAirbubble.h"
#include "PFWorld.h"

namespace PF
{
	RNDefineMeta(Airbubble, RN::SceneNode)

	Airbubble::Airbubble(RN::Vector3 scale) : _movementTimer(0.0f), _currentVolume(0)
	{
		RN::Model *bubbleModel = World::GetSharedInstance()->AssignShader(RN::Model::WithName(RNCSTR("models/airbubble.sgm")), Types::MaterialAirbubble);
		_floatingBubbleEntity = new RN::Entity(bubbleModel);
		_floatingBubbleEntity->AddFlags(RN::Entity::Flags::DrawLater);
		_floatingBubbleEntity->SetScale(scale);
		AddChild(_floatingBubbleEntity->Autorelease());
		
		RN::PhysXMaterial *physicsMaterial = new RN::PhysXMaterial();
		_physicsBody = RN::PhysXDynamicBody::WithShape(RN::PhysXSphereShape::WithRadius(scale.x * 0.5f, physicsMaterial->Autorelease()), 1.0f);
		_physicsBody->SetCollisionFilter(Types::CollisionAirbubble, Types::CollisionAll);
		_physicsBody->SetEnableCCD(true);
		AddAttachment(_physicsBody);
		
		memset(_blockedGrid, 0, 15*15*15);
	}
	
	Airbubble::~Airbubble()
	{
		
	}
	
	void Airbubble::Update(float delta)
	{
		if(delta > 0.2f)
		{
			delta = 0.2f;
		}
		
		if(_physicsBody)
		{
			_physicsBody->ApplyForce(RN::Vector3(0.0f, 6.0f, 0.0f));
			_movementTimer += delta;
			
			if(_physicsBody->GetLinearVelocity().GetLength() < 0.1f && _movementTimer > 2.0f)
			{
				RemoveAttachment(_physicsBody);
				_physicsBody = nullptr;
				
				_growableBubbleEntity = new RN::VoxelEntity(15, 15, 15);
				_growableBubbleEntity->SetMaterial(_floatingBubbleEntity->GetModel()->GetLODStage(0)->GetMaterialAtIndex(0));
				_growableBubbleEntity->SetSphereLocal(RN::Vector3(7.5f, 7.5f, 7.5f), _floatingBubbleEntity->GetScale().x * 2.5f);
				_growableBubbleEntity->SetScale(1.0f/3.0f);
				_growableBubbleEntity->UpdateMesh();
				_growableBubbleEntity->AddFlags(RN::SceneNode::Flags::DrawLater);
				AddChild(_growableBubbleEntity->Autorelease());
				
				World::GetSharedInstance()->AddStaticAirbubble(this);
				
				RemoveChild(_floatingBubbleEntity);
				_floatingBubbleEntity = nullptr;
			}
			else
			{
				Airbubble *otherBubble = World::GetSharedInstance()->FindClosestAirbubble(GetWorldPosition(), this);
				if(otherBubble)
				{
					float distance = otherBubble->GetWorldPosition().GetDistance(GetWorldPosition());
					if(distance < 3.0f)
					{
						if(otherBubble->AddAir(GetWorldPosition(), _floatingBubbleEntity->GetScale().x * 0.8f))
						{
							World::GetSharedInstance()->RemoveLevelNode(this);
						}
					}
				}
			}
		}
		
		if(GetWorldPosition().y > -20.0f)
		{
			World::GetSharedInstance()->RemoveLevelNode(this);
		}

		SceneNode::Update(delta);
	}

	bool Airbubble::AddAir(RN::Vector3 worldPosition, float radius)
	{
		if(!_growableBubbleEntity) return false;
		
		RN::OpenALSource *source = new RN::OpenALSource(RN::AudioAsset::WithName(RNCSTR("audio/pop.ogg")));
		source->SetSelfdestruct(true);
		source->SetPitch(RN::RandomNumberGenerator::GetSharedGenerator()->GetRandomFloatRange(0.7, 1.3));
		source->SetWorldPosition(GetWorldPosition());
		World::GetSharedInstance()->AddNode(source->Autorelease());
		source->Play();
		
		RN::Vector3 offset(_growableBubbleEntity->GetResolutionX() * 0.5f, _growableBubbleEntity->GetResolutionY() * 0.5f, _growableBubbleEntity->GetResolutionZ() * 0.5f);
		RN::Vector3 position = _growableBubbleEntity->GetWorldRotation().Conjugate().GetRotatedVector(worldPosition - _growableBubbleEntity->GetWorldPosition()) / _growableBubbleEntity->GetWorldScale();
		position += offset;
		radius = radius / GetWorldScale().GetMin();
		
		position.x = std::fminf(std::fmaxf(position.x, 0.0f), _growableBubbleEntity->GetResolutionX());
		position.y = std::fminf(std::fmaxf(position.y, 0.0f), _growableBubbleEntity->GetResolutionY());
		position.z = std::fminf(std::fmaxf(position.z, 0.0f), _growableBubbleEntity->GetResolutionZ());
		
		for(RN::uint32 x = position.x - radius; x < position.x + radius; x++)
		{
			for(RN::uint32 y = position.y - radius; y < position.y + radius; y++)
			{
				for(RN::uint32 z = position.z - radius; z < position.z + radius; z++)
				{
					if(_growableBubbleEntity->GetVoxel(x, y, z) > 127)
					{
						_growableBubbleEntity->SetSphere(worldPosition, radius);
						_growableBubbleEntity->UpdateMesh();
						
						CalculateVolume();
						
						return true;
					}
				}
			}
		}
		
		return false;
	}

	void Airbubble::SimulateAir()
	{
		
	}

	bool Airbubble::IsBlocked(int x, int y, int z)
	{
		if(_blockedGrid[x][y][z])
		{
			return true;
		}
		
		RN::Vector3 offset(_growableBubbleEntity->GetResolutionX() * 0.5f, _growableBubbleEntity->GetResolutionY() * 0.5f, _growableBubbleEntity->GetResolutionZ() * 0.5f);
		RN::Vector3 worldPosition(x, y, z);
		worldPosition -= offset;
		worldPosition *= GetWorldScale();
		worldPosition = GetWorldRotation().GetRotatedVector(worldPosition);
		worldPosition += GetWorldPosition();
		
		_blockedGrid[x][y][z] = World::GetSharedInstance()->DoesVoxelOverlap(worldPosition, GetWorldRotation());
		
		return _blockedGrid[x][y][z];
	}

	void Airbubble::CalculateVolume()
	{
		if(!_growableBubbleEntity) return;
		
		int counter = 0;
		for(int x = 0; x < _growableBubbleEntity->GetResolutionX(); x++)
		{
			for(int y = 0; y < _growableBubbleEntity->GetResolutionY(); y++)
			{
				for(int z = 0; z < _growableBubbleEntity->GetResolutionZ(); z++)
				{
					if(_growableBubbleEntity->GetVoxel(x, y, z) > 127)
					{
						counter += 1;
					}
				}
			}
		}
		
		_currentVolume = counter;
	}

	bool Airbubble::IsInside(RN::Vector3 worldPosition)
	{
		if(!_growableBubbleEntity) return false;
		
		RN::Vector3 offset(_growableBubbleEntity->GetResolutionX() * 0.5f, _growableBubbleEntity->GetResolutionY() * 0.5f, _growableBubbleEntity->GetResolutionZ() * 0.5f);
		RN::Vector3 position = GetWorldRotation().Conjugate().GetRotatedVector(worldPosition - GetWorldPosition()) / GetWorldScale();
		position += offset;
		RN::uint8 value = _growableBubbleEntity->GetVoxel(position);
		
		return value > 127;
	}
}
