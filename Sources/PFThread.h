//
//  PFThread.h
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#ifndef __PF_THREAD_H_
#define __PF_THREAD_H_

#include "Rayne.h"
#include "RNPhysXWorld.h"

namespace PF
{
	class Prey;
	class Thread : public RN::Entity
	{
	public:
		Thread();
		~Thread() override;
		
		void Update(float delta) override;
		
		void SetPosition(RN::Vector3 position, bool isEnd);
		void Shoot(RN::Vector3 speed, bool isEnd);
		
		bool CanPull() const { return _isAnchored[0]; }
		void SetPrey(Prey *prey);
		
		void Destroy();
		
	private:
		float _stickyTimer[2];
		float _floatingTimer;
		RN::Vector3 _movement[2];
		RN::Vector3 _position[2];
		bool _isAnchored[2];
		bool _isOwnedByPlayer;
		
		Prey *_anchoredPrey;
		
		RN::PhysXStaticBody *_physicsBody;

		RNDeclareMeta(Thread)
	};
}

#endif /* defined(__PF_THREAD_H_) */
