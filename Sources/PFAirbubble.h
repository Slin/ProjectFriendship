//
//  PFGoldfish.h
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#ifndef __PF_GOLDFISH_H_
#define __PF_GOLDFISH_H_

#include "Rayne.h"
#include "RNPhysXWorld.h"

namespace PF
{
	class Goldfish : public RN::Entity
	{
	public:
		Goldfish(RN::Model *model);
		~Goldfish() override;
		
		void Update(float delta) override;
		
	private:
		float _rotationChangeTimer;
		float _rotationSpeed;
		RN::PhysXKinematicController *_characterController;

		RNDeclareMeta(Goldfish)
	};
}

#endif /* defined(__PF_GOLDFISH_H_) */
