//
//  PFPlayer.h
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#include "PFPlayer.h"
#include "PFWorld.h"

#define MAX_SPEED_CRAWLING 10.0f
#define ORIGIN_TO_HEAD_OFFSET (_bodyEntity->GetScale()*RN::Vector3(0.0f, 0.0f, 0.0f))
#define ORIGIN_TO_COLLISION_OFFSET (_bodyEntity->GetScale()*RN::Vector3(0.0f, -1.0f, 0.0f))
#define LEG_LENGTH (_bodyEntity->GetScale().y*0.4f)
#define HEAD_CAGE_SIZE 1.0f

namespace PF
{
	RNDefineMeta(Player, RN::SceneNode)

	Player::Player() : _rotateTimer(0.0f), _additionalBodyRotationAngle(0.0f), _legGravity{0.0f, 0.0f, 0.0f, 0.0f}, _isSwimming(false), _wantsToSwim(false), _headCameraTilt(0.0f), _snapRotationAngle(0.0f)
	{
		_head = new RN::SceneNode();
		AddChild(_head->Autorelease());
		
		RN::PhysXMaterial *physicsMaterial = new RN::PhysXMaterial();
		_characterController = new RN::PhysXKinematicController(0.2f, 0.01f, physicsMaterial->Autorelease(), 0.1f);
		AddAttachment(_characterController->Autorelease());
		
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

		RN::Mesh *box1Mesh = RN::Mesh::WithColoredCube(RN::Vector3(0.01f, 0.05f, 0.05f), RN::Color::WithRGBA(1.0f, 0.0f, 0.0f));
		RN::Material *boxMaterial = RN::Material::WithShaders(RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Vertex, RN::Shader::Options::WithMesh(box1Mesh)), RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Fragment, RN::Shader::Options::WithMesh(box1Mesh)));
		RN::Model *box1Model = new RN::Model();
		RN::Model::LODStage *box1LODStage = box1Model->AddLODStage(100000.0f);
		box1LODStage->AddMesh(box1Mesh, boxMaterial);
		_debugBox1 = new RN::Entity(box1Model->Autorelease());
		World::GetSharedInstance()->AddLevelNode(_debugBox1->Autorelease());

		RN::Mesh *box2Mesh = RN::Mesh::WithColoredCube(RN::Vector3(0.01f, 0.05f, 0.05f), RN::Color::WithRGBA(0.0f, 1.0f, 0.0f));
		RN::Model *box2Model = new RN::Model();
		RN::Model::LODStage *box2LODStage = box2Model->AddLODStage(100000.0f);
		box2LODStage->AddMesh(box2Mesh, boxMaterial);
		_debugBox2 = new RN::Entity(box2Model->Autorelease());
		World::GetSharedInstance()->AddLevelNode(_debugBox2->Autorelease());

		_bodyEntity->AddFlags(RN::SceneNode::Flags::Hidden);
	}
	
	Player::~Player()
	{
		_head->RemoveFromParent();
	}
	
	void Player::Update(float delta)
	{
		if(delta > 0.2f)
		{
			delta = 0.2f;
		}
		
		World *world = World::GetSharedInstance();
		RN::PhysXWorld *physicsWorld = world->GetPhysicsWorld();
		
		RN::VRControllerTrackingState handController[2];
		
		RN::Camera *headCamera = world->GetHeadCamera();
		RN::VRCamera *vrCamera = world->GetVRCamera();
		RN::Vector3 localHeadToCameraPosition;
		if(vrCamera)
		{
			for(int i = 0; i < 2; i++)
			{
				handController[i] = vrCamera->GetControllerTrackingState(i);
			}
			
			//Limit head movement
			localHeadToCameraPosition = vrCamera->GetHead()->GetPosition() - _headPositionOffset;
			if(localHeadToCameraPosition.GetLength() > HEAD_CAGE_SIZE*0.5f)
			{
				_headPositionOffset += localHeadToCameraPosition.GetNormalized(localHeadToCameraPosition.GetLength() - HEAD_CAGE_SIZE*0.5f);
				localHeadToCameraPosition.Normalize(HEAD_CAGE_SIZE*0.5f);
			}
		}
		else
		{
			RN::InputManager *inputManager = RN::InputManager::GetSharedInstance();
			handController[0].thumbstick = RN::Vector2(inputManager->IsControlToggling(RNCSTR("D")) - inputManager->IsControlToggling(RNCSTR("A")), inputManager->IsControlToggling(RNCSTR("W")) - inputManager->IsControlToggling(RNCSTR("S")));
			
			if(inputManager->IsControlToggling(RNCSTR("SPACE")) && !_isSwimming)
			{
				_currentSwimDirection = _head->GetUp() * 5.0f;
				_wantsToSwim = true;
			}
		}
		
		RN::Quaternion baseRotationWithoutYaw = GetWorldRotation() * RN::Quaternion(RN::Vector3(-_snapRotationAngle-_additionalBodyRotationAngle, 0.0f, 0.0f));
		
		//Always snap turn for now
		if(!vrCamera)
		{
			RN::InputManager *inputManager = RN::InputManager::GetSharedInstance();
			_snapRotationAngle -= inputManager->GetMouseDelta().x * delta * 10.0f;
			_headCameraTilt -= inputManager->GetMouseDelta().y * delta * 10.0f;
		}
		else
		{
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
		}
		
		RN::Quaternion cameraSnapRotation(RN::Vector3(_snapRotationAngle, 0.0f, 0.0f));
		
		handController[0].rotation = cameraSnapRotation*handController[0].rotation * RN::Vector3(0.0f, -45.0f, 0.0f);
		handController[0].position = cameraSnapRotation.GetRotatedVector(handController[0].position);
		//handController[0].velocityLinear = cameraSnapRotation.GetRotatedVector(handController[0].velocityLinear);
		handController[1].rotation = cameraSnapRotation*handController[1].rotation * RN::Vector3(0.0f, -45.0f, 0.0f);
		handController[1].position = cameraSnapRotation.GetRotatedVector(handController[1].position);
		//handController[1].velocityLinear = cameraSnapRotation.GetRotatedVector(handController[1].velocityLinear);
		
		//Movement
		bool isCrawling = false;
		RN::Vector3 globalMovement;
		if(handController[0].handTrigger < 0.3f || handController[1].handTrigger < 0.3f || _isSwimming)
		{
			if(_wantsToSwim)
			{
				RNDebug("Wants to swim");
				
				_wantsToSwim = false;
				_isSwimming = true;
			}
			
			//Crawling with thumbstick
			if(!_isSwimming)
			{
				globalMovement = handController[0].rotation.GetRotatedVector(RN::Vector3(handController[0].thumbstick.x, 0.0f, -handController[0].thumbstick.y));
				globalMovement.y = 0.0f;
				globalMovement = baseRotationWithoutYaw.GetRotatedVector(globalMovement);
				globalMovement.Normalize(MAX_SPEED_CRAWLING * delta);
	
				isCrawling = (globalMovement.GetLength() > RN::k::EpsilonFloat);
			}
			else
			{
				//Handle swimming
				RN::Vector3 swimInput;
				if(!vrCamera)
				{
					swimInput = headCamera->GetForward() * handController[0].thumbstick.y * 10.0f;
					swimInput += headCamera->GetRight() * handController[0].thumbstick.x * 10.0f;
					swimInput *= delta;
				}
				else
				{
					RN::Vector3 leftHandPosition = vrCamera->GetWorldPosition() + baseRotationWithoutYaw.GetRotatedVector(handController[0].position);
					RN::Vector3 rightHandPosition = vrCamera->GetWorldPosition() + baseRotationWithoutYaw.GetRotatedVector(handController[1].position);
					
					RN::Vector3 leftHandDirection = baseRotationWithoutYaw.GetRotatedVector(_previousHandPosition[0] - handController[0].position);
					RN::Vector3 rightHandDirection = baseRotationWithoutYaw.GetRotatedVector(_previousHandPosition[1] - handController[1].position);
					
					RN::Vector3 leftHandAngleDirection = (baseRotationWithoutYaw*handController[0].rotation).GetRotatedVector(RN::Vector3(-1.0f, 0.0f, 0.0f));
					RN::Vector3 rightHandAngleDirection = (baseRotationWithoutYaw*handController[1].rotation).GetRotatedVector(RN::Vector3(1.0f, 0.0f, 0.0f));
					
					leftHandDirection *= std::max(leftHandAngleDirection.GetDotProduct(leftHandDirection.GetNormalized()), 0.0f);
					rightHandDirection *= std::max(rightHandAngleDirection.GetDotProduct(rightHandDirection.GetNormalized()), 0.0f);
					
					
					RN::Vector3 leftHandToHeadDirection = leftHandPosition - _head->GetWorldPosition();
					RN::Vector3 rightHandToHeadDirection = rightHandPosition - _head->GetWorldPosition();
					RN::Quaternion leftHandLookAtRotationCurrent = RN::Quaternion::WithLookAt(leftHandToHeadDirection, _head->GetUp());
					RN::Quaternion rightHandLookAtRotationCurrent = RN::Quaternion::WithLookAt(rightHandToHeadDirection, _head->GetUp());
					
					RN::Quaternion leftHandLookAtRotationPrevious = RN::Quaternion::WithLookAt(leftHandToHeadDirection - leftHandDirection, _head->GetUp());
					RN::Quaternion rightHandLookAtRotationPrevious = RN::Quaternion::WithLookAt(rightHandToHeadDirection - rightHandDirection, _head->GetUp());
					
					RN::Quaternion leftHandRotationDiff = leftHandLookAtRotationCurrent * leftHandLookAtRotationPrevious.GetConjugated();
					RN::Quaternion rightHandRotationDiff = rightHandLookAtRotationCurrent * rightHandLookAtRotationPrevious.GetConjugated();
					
					RN::Quaternion combinedRotation = leftHandRotationDiff * rightHandRotationDiff;
					
					//RN::Vector3 rotationAngle = combinedRotation.GetEulerAngle();
					//RNDebug("rotation: " << rotationAngle.x << ", " << rotationAngle.y << ", " << rotationAngle.z);
					
					_currentSwimRotation += combinedRotation.GetEulerAngle() / std::max(delta, RN::k::EpsilonFloat) * 0.03f;
					
					baseRotationWithoutYaw = RN::Quaternion(_currentSwimRotation*delta) * baseRotationWithoutYaw;
					
					_currentSwimRotation -= _currentSwimRotation * std::min(10.0f * delta, 1.0f);
					
					//leftHandDirection *= _head->GetForward().GetDotProduct(leftHandDirection.GetNormalized());
					//rightHandDirection *= _head->GetForward().GetDotProduct(rightHandDirection.GetNormalized());
					
					swimInput = (leftHandDirection + rightHandDirection) / std::max(delta, RN::k::EpsilonFloat) * 0.1f;
					
					//swimInput *= 200.0f;
				}
				_currentSwimDirection += swimInput;
				
				globalMovement = _currentSwimDirection * delta;
				
				_currentSwimDirection -= _currentSwimDirection * std::min(delta * 0.3f, 1.0f);
			}
		}
		else
		{
			//Jumping by grabbing air and pulling in direction while letting go of trigger
			RN::Vector3 handMovementDiff = (baseRotationWithoutYaw * cameraSnapRotation).GetRotatedVector((handController[0].velocityLinear + handController[1].velocityLinear) * -1.0f);
			_currentSwimDirection = _currentSwimDirection.GetLerp(handMovementDiff, 0.8f);
			
			_wantsToSwim = true;
		}
		
		_previousHandPosition[0] = handController[0].position;
		_previousHandPosition[1] = handController[1].position;
		
		float targetHeight = (_head->GetWorldPosition() + globalMovement).y;
		if(targetHeight > -20.5f && globalMovement.y > RN::k::EpsilonFloat)
		{
			globalMovement -= globalMovement.GetNormalized() * (targetHeight + 20.5f) / globalMovement.GetNormalized().y;
			_currentSwimDirection = RN::Vector3();
		}
		
		//Translate(globalMovement);
		_characterController->Move(globalMovement, delta);
		
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
		int closeEnoughCounter = 4;
		for(int i = 0; i < 4; i++)
		{
			traceDirection[i] = GetWorldRotation().GetRotatedVector(traceDirection[i]);
			legContactInfo[i] = physicsWorld->CastRay(traceStartPosition, traceStartPosition + traceDirection[i] * 1000.0f, Types::CollisionLevel);
			
			if(legContactInfo[i].distance > traceDirection[i].GetLength()*LEG_LENGTH)
			{
				if(!_isSwimming)
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
			}
			
			closestIndices[i] = i;
			for(int n = i-1; n >= 0; n--)
			{
				if(legContactInfo[i].distance < legContactInfo[closestIndices[n]].distance || legContactInfo[closestIndices[n]].distance < 0.0f)
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
			if(legContactInfo[i].distance > maxTraceLength || legContactInfo[i].distance < 0.0f)
			{
				legContactInfo[i].position = traceStartPosition + traceDirection[i].GetNormalized(maxTraceLength);
				closeEnoughCounter -= 1;
			}
		}
		
		bool wasSwimming = _isSwimming;
		if(closeEnoughCounter >= 3)
		{
			_isSwimming = false;
		}
		else if(!_isSwimming)
		{
			_isSwimming = true;
			if(delta > 0.0001f)
				_currentSwimDirection = globalMovement / delta;
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
			
		if(isCrawling || (wasSwimming && !_isSwimming))
		{
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
			
			RN::Vector3 averageFeetPosition = (legContactInfo[3].position + legContactInfo[2].position + legContactInfo[1].position + legContactInfo[0].position) / 4.0f;
			RN::Vector3 targetPosition = averageFeetPosition + targetNormal.GetNormalized(LEG_LENGTH);
			
			RN::Vector3 gravity = targetPosition - traceStartPosition;
			//Translate(gravity);
			_characterController->Move(gravity, delta);
			
			SetWorldRotation(targetRotation);
		}
		worldCameraPosition = GetWorldPosition() + GetWorldRotation().GetRotatedVector(ORIGIN_TO_HEAD_OFFSET + RN::Quaternion(RN::Vector3(-_additionalBodyRotationAngle, 0.0f, 0.0f)).GetRotatedVector(localHeadToCameraPosition));
		
		if(vrCamera)
		{
			vrCamera->SetWorldRotation(GetWorldRotation() * RN::Quaternion(RN::Vector3(-_additionalBodyRotationAngle, 0.0f, 0.0f)));
			vrCamera->SetWorldPosition(worldCameraPosition - vrCamera->GetWorldRotation().GetRotatedVector(vrCamera->GetHead()->GetPosition()));
			
			_debugBox1->SetWorldPosition(vrCamera->GetWorldPosition() + baseRotationWithoutYaw.GetRotatedVector(handController[0].position));
			_debugBox2->SetWorldPosition(vrCamera->GetWorldPosition() + baseRotationWithoutYaw.GetRotatedVector(handController[1].position));
			_debugBox1->SetWorldRotation(baseRotationWithoutYaw * handController[0].rotation);
			_debugBox2->SetWorldRotation(baseRotationWithoutYaw * handController[1].rotation);
		}
		else
		{
			headCamera->SetWorldRotation(GetWorldRotation() * RN::Quaternion(RN::Vector3(-_additionalBodyRotationAngle, 0.0f, 0.0f)));
			headCamera->Rotate(RN::Vector3(0.0f, _headCameraTilt, 0.0f));
			headCamera->SetWorldPosition(worldCameraPosition);
		}

		SceneNode::Update(delta);
	}
}
