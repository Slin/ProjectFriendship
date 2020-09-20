//
//  PFPlayer.h
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#ifndef __PF_PLAYER_H_
#define __PF_PLAYER_H_

#include "Rayne.h"

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
		
		RN::Vector3 _headPositionOffset;
		
		float _rotateTimer;
		float _snapRotationAngle;
		float _additionalBodyRotationAngle;
		float _legGravity[4];
		
		RN::Vector3 _currentJumpDirection;
		RN::Quaternion _jumpTargetRotation;
		RN::Quaternion _jumpStartRotation;
		bool _isJumping;
		bool _wantsToJump;
		float _jumpDistance;
		float _currentJumpDistance;
		
		RN::Vector3 _previousTrackedHandPosition[2];
		float _movementSpeed;
		RN::uint32 _wasMovingFrame;
		bool _isFirstFrame;
		
/*		RN::Entity *_debugBox1;
		RN::Entity *_debugBox2;*/

		RNDeclareMeta(Player)
	};
}

#endif /* defined(__PF_PLAYER_H_) */
