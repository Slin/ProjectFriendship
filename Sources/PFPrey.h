//
//  PFPrey.h
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#ifndef __PF_PREY_H_
#define __PF_PREY_H_

#include "Rayne.h"
#include "RNPhysXWorld.h"

namespace PF
{
	class Thread;
	class Prey : public RN::Entity
	{
	public:
		Prey(RN::Model *model);
		~Prey() override;
		
		void Update(float delta) override;
		
		void Catch(RN::Vector3 target, Thread *thread);
		void ReleasePrey();
		
	private:
		float _rotationChangeTimer;
		float _rotationSpeed;
		RN::PhysXDynamicBody *_physicsBody;
		bool _isHooked;
		Thread *_holdingThread;
		
		float _energy;
		
		RN::Vector3 _targetPosition;

		RNDeclareMeta(Prey)
	};
}

#endif /* defined(__PF_PREY_H_) */
