//
//  PFPlayer.h
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#ifndef __PF_PLAYER_H_
#define __PF_PLAYER_H_

#include "Rayne.h"
#include "RNPhysXWorld.h"
#include "PFThread.h"

namespace PF
{
	class Player : public RN::SceneNode
	{
	public:
		Player();
		~Player() override;
		
		void Update(float delta) override;
		
		void ResetThread(Thread *thread);
		void Eat();
		
	private:
		bool _isFirstFrame;
		RN::SceneNode *_head;
		RN::Entity *_bodyEntity;
		RN::Model *_bodyModel;
		float _headCameraTilt;
		
		RN::Entity *_airBubbleEntity;
		float _airBubbleSize;
		
		RN::Entity *_eggsEntity;
		RN::uint32 _eggCounter;
		
		float _rotateTimer;
		float _snapRotationAngle;
		
		RN::Vector3 _currentSwimDirection;
		RN::Vector3 _currentSwimRotation;
		bool _isSwimming;
		
		RN::Vector3 _previousHeadPosition;
		RN::Vector3 _previousHandPosition[2];
		
		RN::Entity *_debugBox1;
		RN::Entity *_debugBox2;
		
		Thread *_activeThread[2];
		
		RN::PhysXKinematicController *_characterController;

		RNDeclareMeta(Player)
	};
}

#endif /* defined(__PF_PLAYER_H_) */
