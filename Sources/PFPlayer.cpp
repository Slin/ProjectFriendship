//
//  PFPlayer.h
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#include "PFPlayer.h"
#include "PFWorld.h"

#define MAX_SPEED_CRAWLING 5.0f
#define ORIGIN_TO_HEAD_OFFSET (_bodyEntity->GetScale()*RN::Vector3(0.0f, 0.0f, 0.0f))
#define ORIGIN_TO_COLLISION_OFFSET (_bodyEntity->GetScale()*RN::Vector3(0.0f, -0.2f, 0.0f))
#define LEG_LENGTH (_bodyEntity->GetScale().y*0.6f)
#define HEAD_CAGE_SIZE 1.0f

namespace PF
{
	RNDefineMeta(Player, RN::SceneNode)

	Player::Player() : _rotateTimer(0.0f), _additionalBodyRotationAngle(0.0f), _legGravity{0.0f, 0.0f, 0.0f, 0.0f}, _isJumping(false), _wantsToJump(false), _jumpDistance(0.0f), _currentJumpDistance(0.0f), _movementSpeed(0.0f), _wasMovingFrame(10), _isFirstFrame(true)
	{
		_head = new RN::SceneNode();
		AddChild(_head->Autorelease());
		
		//Create the body entity
		_bodyModel = RN::Model::WithName(RNCSTR("models/player.sgm"));
		_bodyModel = World::GetSharedInstance()->AssignShader(_bodyModel, Types::MaterialPlayer);
		//_bodyModel->SetSkeleton(_bodyModel->GetSkeleton()->Copy());
		_bodyEntity = new RN::Entity(_bodyModel);
		AddChild(_bodyEntity->Autorelease());

		//_bodyEntity->SetScale(RN::Vector3(20.0f, 20.0f, 20.0f));
		//_bodyEntity->GetModel()->GetSkeleton()->SetAnimation(RNCSTR("walk_fixed"));
		
		World *world = World::GetSharedInstance();
		RN::VRCamera *vrCamera = world->GetVRCamera();
		RN::Camera *headCamera = world->GetHeadCamera();
		if(vrCamera)
		{
			_headPositionOffset = vrCamera->GetHead()->GetPosition();
			vrCamera->SetWorldPosition(GetWorldPosition() + ORIGIN_TO_HEAD_OFFSET - _headPositionOffset);
			vrCamera->SetWorldRotation(GetWorldRotation());
			vrCamera->GetHead()->AddChild(_head);
		}
		else if(headCamera)
		{
			headCamera->SetWorldPosition(GetWorldPosition());
			headCamera->SetRotation(RN::Quaternion());
			_headPositionOffset = headCamera->GetPosition();
		}

/*		RN::Mesh *box1Mesh = RN::Mesh::WithColoredCube(RN::Vector3(0.005f, 0.005f, 0.005f), RN::Color::WithRGBA(1.0f, 0.0f, 0.0f));
		RN::Material *boxMaterial = RN::Material::WithShaders(RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Vertex, RN::Shader::Options::WithMesh(box1Mesh)), RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Fragment, RN::Shader::Options::WithMesh(box1Mesh)));
		RN::Model *box1Model = new RN::Model();
		RN::Model::LODStage *box1LODStage = box1Model->AddLODStage(100000.0f);
		box1LODStage->AddMesh(box1Mesh, boxMaterial);
		_debugBox1 = new RN::Entity(box1Model->Autorelease());
		World::GetSharedInstance()->AddLevelNode(_debugBox1->Autorelease());

		RN::Mesh *box2Mesh = RN::Mesh::WithColoredCube(RN::Vector3(0.005f, 0.005f, 0.005f), RN::Color::WithRGBA(0.0f, 1.0f, 0.0f));
		RN::Model *box2Model = new RN::Model();
		RN::Model::LODStage *box2LODStage = box2Model->AddLODStage(100000.0f);
		box2LODStage->AddMesh(box2Mesh, boxMaterial);
		_debugBox2 = new RN::Entity(box2Model->Autorelease());
		World::GetSharedInstance()->AddLevelNode(_debugBox2->Autorelease());*/

		//_bodyEntity->AddFlags(RN::SceneNode::Flags::Hidden);
	}
	
	Player::~Player()
	{
		_head->RemoveFromParent();
	}
	
	void Player::Update(float delta)
	{
		World *world = World::GetSharedInstance();
		RN::VRCamera *vrCamera = world->GetVRCamera();
		if(!vrCamera) return;
		
		if(delta > 0.2f)
		{
			delta = 0.2f;
		}
		
		RN::PhysXWorld *physicsWorld = world->GetPhysicsWorld();
		
		RN::VRControllerTrackingState handController[2];
		RN::Vector3 rawControllerPosition[2];
		RN::Quaternion rawControllerRotation[2];
		for(int i = 0; i < 2; i++)
		{
			handController[i] = vrCamera->GetControllerTrackingState(i);
			rawControllerPosition[i] = handController[i].position;
			rawControllerRotation[i] = handController[i].rotation;
		}
		
		if(_isFirstFrame)
		{
			_previousTrackedHandPosition[0] = rawControllerPosition[0];
			_previousTrackedHandPosition[1] = rawControllerPosition[1];
			_isFirstFrame = false;
		}
		
		//Limit head movement
		RN::Vector3 localHeadToCameraPosition = vrCamera->GetHead()->GetPosition() - _headPositionOffset;
		if(localHeadToCameraPosition.GetLength() > HEAD_CAGE_SIZE*0.5f)
		{
			_headPositionOffset += localHeadToCameraPosition.GetNormalized(localHeadToCameraPosition.GetLength() - HEAD_CAGE_SIZE*0.5f);
			localHeadToCameraPosition.Normalize(HEAD_CAGE_SIZE*0.5f);
		}
		
		RN::Quaternion baseRotationWithoutYaw = GetWorldRotation() * RN::Quaternion(RN::Vector3(-_snapRotationAngle-_additionalBodyRotationAngle, 0.0f, 0.0f));
		
		//Always snap turn for now
		if(true)
		{
			//Snap turning
			RN::Vector3 snapTurnMovement;
			if(std::abs(handController[1].thumbstick.x) > 0.3f)
			{
				if(_rotateTimer <= RN::k::EpsilonFloat)
				{
					float sign = handController[1].thumbstick.x > 0.0 ? -1.0 : 1.0;
					_snapRotationAngle += 45.0f * sign;
					_rotateTimer = 0.25f;
				}
			}
			else
			{
				_rotateTimer = 0.0f;
			}
			_rotateTimer = std::max(_rotateTimer - delta, 0.0f);
		}
		else
		{
			//Smooth turning
			_snapRotationAngle -= handController[1].thumbstick.x * delta * 120.0f;
		}
		
		RN::Quaternion cameraSnapRotation(RN::Vector3(_snapRotationAngle, 0.0f, 0.0f));
		
		handController[0].rotation = cameraSnapRotation*handController[0].rotation * RN::Vector3(0.0f, -45.0f, 0.0f);
		handController[0].position = cameraSnapRotation.GetRotatedVector(handController[0].position);
		//handController[0].velocityLinear = cameraSnapRotation.GetRotatedVector(handController[0].velocityLinear);
		handController[1].rotation = cameraSnapRotation*handController[1].rotation * RN::Vector3(0.0f, -45.0f, 0.0f);
		handController[1].position = cameraSnapRotation.GetRotatedVector(handController[1].position);
		//handController[1].velocityLinear = cameraSnapRotation.GetRotatedVector(handController[1].velocityLinear);
		
		bool wasAlreadyJumping = _isJumping;
		
		//Movement
		RN::Vector3 globalMovement;
		if(handController[0].handTrigger < 0.3f || handController[1].handTrigger < 0.3f || _isJumping)
		{
			if(_wantsToJump)
			{
				RNDebug("Wants to jump");
				
				_wantsToJump = false;
				_isJumping = true;
				
				RN::Vector3 traceStartPosition = GetWorldPosition() + GetWorldRotation().GetRotatedVector(ORIGIN_TO_COLLISION_OFFSET);
				RN::PhysXContactInfo jumpContactInfo = physicsWorld->CastRay(traceStartPosition, traceStartPosition + _currentJumpDirection * 1000.0f, Types::CollisionLevel);
				_currentJumpDistance = _jumpDistance = jumpContactInfo.distance;
				
				RN::Vector3 forwardDirection = GetForward();
				RN::Vector3 rightDirection = jumpContactInfo.normal.GetCrossProduct(forwardDirection);
				forwardDirection = jumpContactInfo.normal.GetCrossProduct(rightDirection);
				_jumpTargetRotation = RN::Quaternion::WithNormalizedVectors(forwardDirection, -rightDirection, jumpContactInfo.normal) * RN::Quaternion(RN::Vector3(-_snapRotationAngle-_additionalBodyRotationAngle, 0.0f, 0.0f));
				_jumpStartRotation = baseRotationWithoutYaw;
			}
			
			if(!_isJumping)
			{
				_currentJumpDirection = RN::Vector3();
				_jumpDistance = 0.0f;
				_currentJumpDistance = 0.0f;
				
				//Crawling by arm swinging
				RN::Vector3 averageControllerDirection;
				_wasMovingFrame += 1;
				for(int i = 0; i < 2; i++)
				{
					RN::Vector3 controllerDirection = rawControllerRotation[i].GetRotatedVector(RN::Vector3(0.0f, 0.0f, -1.0f));
					RN::Vector3 handMovementDiff = _previousTrackedHandPosition[i] - rawControllerPosition[i];
					
					float angleToRotation = controllerDirection.GetDotProduct(handMovementDiff);
					float handSpeed = handMovementDiff.GetLength();
					if(angleToRotation > 0.0f && handSpeed > 0.0005)
					{
						_movementSpeed += handSpeed * 5.0f;
						_wasMovingFrame = 0;
					}
					
					controllerDirection.y = 0.0f;
					averageControllerDirection += controllerDirection.GetNormalized();
				}
				
				_movementSpeed = std::min(_movementSpeed, MAX_SPEED_CRAWLING);
				if(_wasMovingFrame > 3) _movementSpeed = std::max(_movementSpeed - 50.0f * delta, 0.0f);
				
				globalMovement = averageControllerDirection.GetNormalized();
				globalMovement = (baseRotationWithoutYaw * cameraSnapRotation).GetRotatedVector(globalMovement);
				globalMovement.Normalize(_movementSpeed * delta);
			}
			else
			{
				globalMovement = _currentJumpDirection * delta;
				_currentJumpDistance -= globalMovement.GetLength();
				
				float rotationFactor = (_currentJumpDistance - LEG_LENGTH) / (_jumpDistance - LEG_LENGTH);
				baseRotationWithoutYaw = _jumpTargetRotation.GetLerpSpherical(_jumpStartRotation, rotationFactor);
				
				if(_currentJumpDistance < LEG_LENGTH)
				{
					_isJumping = false;
				}
			}
		}
		else
		{
			//Jumping by grabbing air and pulling in direction while letting go of trigger
			RN::Vector3 handMovementDiff = (baseRotationWithoutYaw * cameraSnapRotation).GetRotatedVector((handController[0].velocityLinear + handController[1].velocityLinear) * -1.0f);
			_currentJumpDirection = _currentJumpDirection.GetLerp(handMovementDiff, 0.8f);
			
			_wantsToJump = true;
		}
		
		for(int i = 0; i < 2; i++)
		{
			_previousTrackedHandPosition[i] = rawControllerPosition[i];
		}
		
		Translate(globalMovement);
		
		//Rotation
		RN::Vector3 worldCameraPosition = GetWorldPosition() + GetWorldRotation().GetRotatedVector(ORIGIN_TO_HEAD_OFFSET + RN::Quaternion(RN::Vector3(-_additionalBodyRotationAngle, 0.0f, 0.0f)).GetRotatedVector(localHeadToCameraPosition));

		//This used to work fine and suddenly something broke o.O didn't find any obvious changes...
/*		float rotationDirection = vrCamera->GetHead()->GetRotation().GetEulerAngle().x - _additionalBodyRotationAngle;
		if(rotationDirection > 180.0f) rotationDirection -= 360.0f;
		if(rotationDirection < -180.0f) rotationDirection += 360.0f;
		_additionalBodyRotationAngle += std::min(std::abs(delta * 2.0f * rotationDirection), std::abs(rotationDirection)) * ((0 < rotationDirection) - (rotationDirection < 0));
		while(_additionalBodyRotationAngle >= 360.0f) _additionalBodyRotationAngle -= 360.0f;
		while(_additionalBodyRotationAngle < 0.0f) _additionalBodyRotationAngle += 360.0f;*/
		
		RN::Vector3 rotatedCameraToHeadPosition = (baseRotationWithoutYaw * cameraSnapRotation).GetRotatedVector(-localHeadToCameraPosition);
		RN::Vector3 worldHeadPosition = worldCameraPosition + rotatedCameraToHeadPosition;
		RN::Vector3 rotatedHeadToBodyPosition = (baseRotationWithoutYaw * cameraSnapRotation * RN::Vector3(_additionalBodyRotationAngle, 0.0f, 0.0f)).GetRotatedVector(-ORIGIN_TO_HEAD_OFFSET);
		RN::Vector3 worldBodyPosition = worldHeadPosition + rotatedHeadToBodyPosition;
		SetWorldPosition(worldBodyPosition);
		SetWorldRotation(baseRotationWithoutYaw * cameraSnapRotation * RN::Vector3(_additionalBodyRotationAngle, 0.0f, 0.0f));
		

		RN::Vector3 traceDirection[4];
		traceDirection[0] = RN::Vector3(-1.0f, -1.0f, -1.0f); //front-left
		traceDirection[1] = RN::Vector3(1.0f, -1.0f, -1.0f); //front-right
		traceDirection[2] = RN::Vector3(-1.0f, -1.0f, 1.0f); //back-left
		traceDirection[3] = RN::Vector3(1.0f, -1.0f, 1.0f); //back-right
		RN::Vector3 traceStartPosition = GetWorldPosition() + GetWorldRotation().GetRotatedVector(ORIGIN_TO_COLLISION_OFFSET);
		RN::PhysXContactInfo legContactInfo[4];
		int closestIndices[4] = {0, 0, 0, 0};
		for(int i = 0; i < 4; i++)
		{
			traceDirection[i] = GetWorldRotation().GetRotatedVector(traceDirection[i]);
			legContactInfo[i] = physicsWorld->CastRay(traceStartPosition, traceStartPosition + traceDirection[i] * 1000.0f, Types::CollisionLevel);
			if(legContactInfo[i].distance > traceDirection[i].GetLength()*LEG_LENGTH)
			{
				if(!_isJumping)
				{
					_legGravity[i] += /*9.81f*/ 30.0f * delta;
				}
				else
				{
					
				}
			}
			else
			{
				_legGravity[i] = 0.0f;
				
				if(!wasAlreadyJumping)
				{
					_isJumping = false;
				}
			}
			
			closestIndices[i] = i;
			for(int n = i-1; n >= 0; n--)
			{
				if(legContactInfo[i].distance < legContactInfo[closestIndices[n]].distance)
				{
					closestIndices[n+1] = closestIndices[n];
					closestIndices[n] = i;
				}
				else
				{
					break;
				}
			}
			
			float maxTraceLength = traceDirection[i].GetLength() * LEG_LENGTH + _legGravity[i] * delta;
			if(legContactInfo[i].distance > maxTraceLength)
			{
				legContactInfo[i].position = traceStartPosition + traceDirection[i].GetNormalized(maxTraceLength);
			}
		}
		
		RN::Vector3 leftRightDirection;
		RN::Vector3 backFrontDirection;
		
		//Ignore a raycast in the back
		if(closestIndices[3] > 1)
		{
			leftRightDirection = legContactInfo[1].position - legContactInfo[0].position;
			
			//Ignore a raycast in the back-left
			if(closestIndices[3] == 2)
			{
				backFrontDirection = legContactInfo[1].position - legContactInfo[3].position;
			}
			//Ignore a raycast in the back-right
			else
			{
				backFrontDirection = legContactInfo[0].position - legContactInfo[2].position;
			}
		}
		//Ignore a raycast in the front
		else
		{
			leftRightDirection = legContactInfo[3].position - legContactInfo[2].position;
			
			//Ignore a raycast in the front-left
			if(closestIndices[3] == 0)
			{
				backFrontDirection = legContactInfo[1].position - legContactInfo[3].position;
			}
			//Ignore a raycast in the front-right
			else
			{
				backFrontDirection = legContactInfo[0].position - legContactInfo[2].position;
			}
		}
		
		RN::Vector3 targetNormal = leftRightDirection.GetCrossProduct(backFrontDirection).GetNormalized();
		
		//TODO: make this more stable
		if(std::abs(globalMovement.GetNormalized().GetDotProduct(GetRight())) > 0.5f)
		{
			leftRightDirection = -targetNormal.GetCrossProduct(backFrontDirection).GetNormalized();
			backFrontDirection = -targetNormal.GetCrossProduct(leftRightDirection).GetNormalized();
		}
		else
		{
			backFrontDirection = -targetNormal.GetCrossProduct(leftRightDirection).GetNormalized();
			leftRightDirection = targetNormal.GetCrossProduct(backFrontDirection).GetNormalized();
		}
		RN::Quaternion targetRotation = RN::Quaternion::WithNormalizedVectors(backFrontDirection, leftRightDirection, targetNormal);
		
		//RNDebug("hanging leg: " << closestIndices[3]);
		
		RN::Vector3 averageFeetPosition = (legContactInfo[3].position + legContactInfo[2].position + legContactInfo[1].position + legContactInfo[0].position) / 4.0f;
		RN::Vector3 targetPosition = averageFeetPosition + targetNormal.GetNormalized(LEG_LENGTH);
		
		RN::Vector3 gravity = targetPosition - traceStartPosition;
		Translate(gravity);
		
		SetWorldRotation(targetRotation);
		
		worldCameraPosition = GetWorldPosition() + GetWorldRotation().GetRotatedVector(ORIGIN_TO_HEAD_OFFSET + RN::Quaternion(RN::Vector3(-_additionalBodyRotationAngle, 0.0f, 0.0f)).GetRotatedVector(localHeadToCameraPosition));
		vrCamera->SetWorldRotation(GetWorldRotation() * RN::Quaternion(RN::Vector3(-_additionalBodyRotationAngle, 0.0f, 0.0f)));
		vrCamera->SetWorldPosition(worldCameraPosition - vrCamera->GetWorldRotation().GetRotatedVector(vrCamera->GetHead()->GetPosition()));
		
		SceneNode::Update(delta);
	}
}
