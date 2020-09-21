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

namespace PF
{
	class Player : public RN::SceneNode
	{
	public:
		Player();
		~Player() override;
		
		void Update(float delta) override;
		
	private:
		RN::SceneNode *_head;
		RN::Entity *_bodyEntity;
		RN::Model *_bodyModel;
		float _headCameraTilt;
		
		RN::Vector3 _headPositionOffset;
		
		float _rotateTimer;
		float _snapRotationAngle;
		float _additionalBodyRotationAngle;
		float _legGravity[4];
		
		RN::Vector3 _currentSwimDirection;
		bool _isSwimming;
		bool _wantsToSwim;
		RN::Vector3 _previousHandPosition[2];
		
		RN::Entity *_debugBox1;
		RN::Entity *_debugBox2;
		
		RN::PhysXKinematicController *_characterController;

		RNDeclareMeta(Player)
	};
}

#endif /* defined(__PF_PLAYER_H_) */
