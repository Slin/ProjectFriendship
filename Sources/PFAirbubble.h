//
//  PFAirbubble.h
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#ifndef __PF_AIRBUBBLE_H_
#define __PF_AIRBUBBLE_H_

#include "Rayne.h"
#include "RNPhysXWorld.h"

namespace PF
{
	class Airbubble : public RN::SceneNode
	{
	public:
		Airbubble(RN::Vector3 scale);
		~Airbubble() override;
		
		void Update(float delta) override;
		
		bool AddAir(RN::Vector3 worldPosition, float radius);
		
		bool IsInside(RN::Vector3 worldPosition);
		bool CanEat() const { return _currentVolume > 25; }
		bool CanBreed() const { return _currentVolume > 100; }
		
	private:
		void SimulateAir();
		bool IsBlocked(int x, int y, int z);
		
		RN::uint8 _blockedGrid[15][15][15];
		
		RN::Entity *_floatingBubbleEntity;
		RN::VoxelEntity *_growableBubbleEntity;
		float _movementTimer;
		
		RN::PhysXDynamicBody *_physicsBody;
		
		int _currentVolume;
		void CalculateVolume();

		RNDeclareMeta(Airbubble)
	};
}

#endif /* defined(__PF_AIRBUBBLE_H_) */
