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
		
	private:
		RN::Entity *_floatingBubbleEntity;
		float _movementTimer;
		
		RN::PhysXDynamicBody *_physicsBody;

		RNDeclareMeta(Airbubble)
	};
}

#endif /* defined(__PF_AIRBUBBLE_H_) */
