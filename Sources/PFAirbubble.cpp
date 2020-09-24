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

	Airbubble::Airbubble(RN::Vector3 scale) : _movementTimer(0.0f)
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
		
		_previousPosition = GetWorldPosition();
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
			_physicsBody->ApplyForce(RN::Vector3(0.0f, 15.0f, 0.0f));
			_movementTimer += delta;
			
			if(GetWorldPosition().GetSquaredDistance(_previousPosition) < 0.1f && _movementTimer > 5.0f)
			{
				RemoveAttachment(_physicsBody);
				_physicsBody = nullptr;
			}
			_previousPosition = GetWorldPosition();
		}
		
		if(GetWorldPosition().y > -20.0f)
		{
			World::GetSharedInstance()->RemoveLevelNode(this);
		}

		SceneNode::Update(delta);
	}
}
