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

	Player::Player() : _isFirstFrame(true), _rotateTimer(0.0f), _isSwimming(false), _headCameraTilt(0.0f), _snapRotationAngle(0.0f), _activeThread{nullptr, nullptr}, _airBubbleSize(0.5f), _eggCounter(0)
	{
		_head = new RN::SceneNode();
		AddChild(_head->Autorelease());
		
		RN::PhysXMaterial *physicsMaterial = new RN::PhysXMaterial();
		_characterController = new RN::PhysXKinematicController(0.2f, 0.01f, physicsMaterial->Autorelease(), 0.1f);
		_characterController->SetCollisionFilter(Types::CollisionPlayer, Types::CollisionPlayerMask);
		AddAttachment(_characterController->Autorelease());
		
		//Create the body entity
		_bodyModel = RN::Model::WithName(RNCSTR("models/player.sgm"));
		_bodyModel = World::GetSharedInstance()->AssignShader(_bodyModel, Types::MaterialPlayer);
		//_bodyModel->SetSkeleton(_bodyModel->GetSkeleton()->Copy());
		_bodyEntity = new RN::Entity(_bodyModel);
		AddChild(_bodyEntity->Autorelease());

		//_bodyEntity->SetScale(RN::Vector3(20.0f, 20.0f, 20.0f));
		//_bodyEntity->GetModel()->GetSkeleton()->SetAnimation(RNCSTR("walk_fixed"));
		
		_airBubbleEntity = new RN::Entity(World::GetSharedInstance()->AssignShader(RN::Model::WithName(RNCSTR("models/airbubble.sgm")), Types::MaterialAirbubble));
		_bodyEntity->AddChild(_airBubbleEntity->Autorelease());
		_airBubbleEntity->SetPosition(RN::Vector3(0.0f, -0.8f, 0.25f));
		_airBubbleEntity->AddFlags(RN::Entity::Flags::DrawLate);
		_airBubbleEntity->SetScale(RN::Vector3(_airBubbleSize, _airBubbleSize, _airBubbleSize));
		
		_eggsEntity = new RN::Entity(World::GetSharedInstance()->AssignShader(RN::Model::WithName(RNCSTR("models/eggs.sgm")), Types::MaterialPlayer));
		_bodyEntity->AddChild(_eggsEntity->Autorelease());
		_eggsEntity->SetPosition(RN::Vector3(0.0f, -0.8f, 0.25f));
		
		World *world = World::GetSharedInstance();
		RN::VRCamera *vrCamera = world->GetVRCamera();
		RN::Camera *headCamera = world->GetHeadCamera();
		if(vrCamera)
		{
			_previousHeadPosition = vrCamera->GetHead()->GetPosition();
			vrCamera->SetWorldPosition(GetWorldPosition() - vrCamera->GetHead()->GetPosition());
			vrCamera->SetWorldRotation(GetWorldRotation());
			vrCamera->GetHead()->AddChild(_head);
		}
		else if(headCamera)
		{
			headCamera->SetWorldPosition(GetWorldPosition());
			headCamera->SetRotation(RN::Quaternion());
			_previousHeadPosition = headCamera->GetPosition();
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
		if(vrCamera)
		{
			for(int i = 0; i < 2; i++)
			{
				handController[i] = vrCamera->GetControllerTrackingState(i);
			}
		}
		else
		{
			RN::InputManager *inputManager = RN::InputManager::GetSharedInstance();
			handController[0].thumbstick = RN::Vector2(inputManager->IsControlToggling(RNCSTR("D")) - inputManager->IsControlToggling(RNCSTR("A")), inputManager->IsControlToggling(RNCSTR("W")) - inputManager->IsControlToggling(RNCSTR("S")));
			
			if(inputManager->IsControlToggling(RNCSTR("SPACE")) && !_isSwimming)
			{
				_currentSwimDirection = _head->GetUp() * 5.0f;
				_isSwimming = true;
			}
			
			handController[0].indexTrigger = inputManager->IsControlToggling(RNCSTR("E"))? 1.0f:0.0f;
			handController[0].button[RN::VRControllerTrackingState::Button::AX] = inputManager->IsControlToggling(RNCSTR("Q"));
			handController[0].button[RN::VRControllerTrackingState::Button::BY] = inputManager->IsControlToggling(RNCSTR("F"));
		}
		
		RN::Quaternion baseRotationWithoutYaw = GetWorldRotation() * RN::Quaternion(RN::Vector3(-_snapRotationAngle, 0.0f, 0.0f));
		
		bool didSnapTurn = false;
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
						didSnapTurn = true;
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
		
		bool wasFirstFrame = _isFirstFrame;
		if(_isFirstFrame || didSnapTurn)
		{
			_previousHandPosition[0] = handController[0].position;
			_previousHandPosition[1] = handController[1].position;
			_isFirstFrame = false;
		}
		
		//Movement
		bool isCrawling = false;
		RN::Vector3 globalMovement;
		if(handController[0].handTrigger < 0.3f || handController[1].handTrigger < 0.3f || _isSwimming)
		{
			//Crawling with thumbstick
			if(!_isSwimming)
			{
				_currentSwimDirection = RN::Vector3();
				
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
				bool isPulling = false;
				if(!vrCamera)
				{
					swimInput = headCamera->GetForward() * handController[0].thumbstick.y * 10.0f;
					swimInput += headCamera->GetRight() * handController[0].thumbstick.x * 10.0f;
					swimInput *= delta;
					
					if(handController[0].button[RN::VRControllerTrackingState::AX] && _activeThread[0])
					{
						swimInput += (_activeThread[0]->GetWorldPosition() - headCamera->GetWorldPosition()).GetNormalized(delta * 20.0f);
						isPulling = true;
					}
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
					
					_currentSwimRotation += combinedRotation.GetEulerAngle() / std::max(delta, RN::k::EpsilonFloat) * 0.03f;
					_currentSwimRotation.y = 0.0f;
					_currentSwimRotation.z = 0.0f;
					
					baseRotationWithoutYaw = RN::Quaternion(_currentSwimRotation*delta) * baseRotationWithoutYaw;
					
					_currentSwimRotation -= _currentSwimRotation * std::min(10.0f * delta, 1.0f);
					
					swimInput = (leftHandDirection + rightHandDirection) / std::max(delta, RN::k::EpsilonFloat) * 0.1f;
					
					if(handController[0].button[RN::VRControllerTrackingState::AX] && _activeThread[0] && _activeThread[0]->CanPull())
					{
						swimInput += (_activeThread[0]->GetWorldPosition() - leftHandPosition).GetNormalized(delta * 8.0f);
						isPulling = true;
					}
					if(handController[1].button[RN::VRControllerTrackingState::AX] && _activeThread[1] && _activeThread[1]->CanPull())
					{
						swimInput += (_activeThread[1]->GetWorldPosition() - leftHandPosition).GetNormalized(delta * 8.0f);
						isPulling = true;
					}
				}
				_currentSwimDirection += swimInput;
				
				globalMovement = _currentSwimDirection * delta;
				
				_currentSwimDirection -= _currentSwimDirection * std::min(delta * 0.3f, 1.0f);
				
				if(swimInput.y > RN::k::EpsilonFloat && _head->GetWorldPosition().y > -21.0f)
				{
					_airBubbleSize += swimInput.y * delta;
					_airBubbleSize = std::min(_airBubbleSize, 1.3f);
					
					_airBubbleEntity->SetScale(RN::Vector3(_airBubbleSize, _airBubbleSize, _airBubbleSize) * _eggsEntity->GetScale().x);
				}
				
				if(!isPulling)
				{
					_currentSwimDirection.y += (_airBubbleSize - 0.5f) * delta * 10.0f;
				}
			}
		}
		else
		{
			//Jumping by grabbing air and pulling in direction while letting go of trigger
			RN::Vector3 handMovementDiff = (baseRotationWithoutYaw * cameraSnapRotation).GetRotatedVector((handController[0].velocityLinear + handController[1].velocityLinear) * -1.0f);
			_currentSwimDirection = _currentSwimDirection.GetLerp(handMovementDiff, 0.8f);
			_isSwimming = true;
		}
		
		_previousHandPosition[0] = handController[0].position;
		_previousHandPosition[1] = handController[1].position;
		
		float targetHeight = (_head->GetWorldPosition() + globalMovement).y;
		if(targetHeight > -20.5f && globalMovement.y > RN::k::EpsilonFloat)
		{
			globalMovement -= globalMovement.GetNormalized() * (targetHeight + 20.5f) / globalMovement.GetNormalized().y;
			_currentSwimDirection = RN::Vector3();
		}
		
		RN::Vector3 localMovement;
		if(vrCamera)
		{
			localMovement = vrCamera->GetHead()->GetPosition() - _previousHeadPosition;
			localMovement = vrCamera->GetWorldRotation().GetRotatedVector(localMovement);
			_previousHeadPosition = vrCamera->GetHead()->GetPosition();
		}
		
		RN::Vector3 gravity;
		const RN::PhysXContactInfo &gravityContact = physicsWorld->CastRay(GetWorldPosition(), GetWorldPosition() - GetUp() * 100.0f, Types::CollisionGravityMask);
		if(gravityContact.distance >= 0.0f)
		{
			if(vrCamera)
			{
				if(_isSwimming && gravityContact.distance <= vrCamera->GetHead()->GetPosition().y)
				{
					_isSwimming = false;
				}
				
				if(!_isSwimming)
				{
					gravity = GetUp() * (vrCamera->GetHead()->GetPosition().y - gravityContact.distance);
				}
			}
			else
			{
				if(_isSwimming && gravityContact.distance <= 1.8f)
				{
					_isSwimming = false;
				}
				
				if(!_isSwimming)
				{
					gravity = GetUp() * (1.8f - gravityContact.distance);
				}
			}
		}
		
		_characterController->Move(globalMovement + localMovement + gravity, delta);
		SetWorldRotation(baseRotationWithoutYaw * cameraSnapRotation);
		
		if(vrCamera)
		{
			vrCamera->SetWorldRotation(GetWorldRotation());
			vrCamera->SetWorldPosition(GetWorldPosition() - vrCamera->GetWorldRotation().GetRotatedVector(vrCamera->GetHead()->GetPosition()));
			
			_debugBox1->SetWorldPosition(vrCamera->GetWorldPosition() + baseRotationWithoutYaw.GetRotatedVector(handController[0].position));
			_debugBox2->SetWorldPosition(vrCamera->GetWorldPosition() + baseRotationWithoutYaw.GetRotatedVector(handController[1].position));
			_debugBox1->SetWorldRotation(baseRotationWithoutYaw * handController[0].rotation);
			_debugBox2->SetWorldRotation(baseRotationWithoutYaw * handController[1].rotation);
		}
		else
		{
			headCamera->SetWorldRotation(GetWorldRotation());
			headCamera->Rotate(RN::Vector3(0.0f, _headCameraTilt, 0.0f));
			headCamera->SetWorldPosition(GetWorldPosition());
		}
		
		for(int i = 0; i < 2; i++)
		{
			RN::Vector3 handPosition;
			RN::Quaternion handRotation;
			if(vrCamera)
			{
				handPosition = vrCamera->GetWorldPosition() + baseRotationWithoutYaw.GetRotatedVector(handController[i].position);
				handRotation = baseRotationWithoutYaw * handController[i].rotation;
			}
			else
			{
				handPosition = GetWorldPosition();
				handRotation = headCamera->GetWorldRotation();
			}
			
			RN::Vector3 handForward = handRotation.GetRotatedVector(RN::Vector3(0.0f, 0.0f, -1.0f));
			if(handController[i].indexTrigger > 0.3f)
			{
				if(!_activeThread[i])
				{
					_activeThread[i] = new Thread();
					world->AddLevelNode(_activeThread[i]);
					_activeThread[i]->SetPosition(handPosition, false);
					_activeThread[i]->Shoot(handForward*10.0f + _currentSwimDirection, false);
				}
				
				_activeThread[i]->SetPosition(handPosition, true);
			}
			else if(_activeThread[i])
			{
				_activeThread[i]->Shoot(handForward*10.0f + _currentSwimDirection, true);
				_activeThread[i] = nullptr;
			}
		}
		
		if(handController[0].button[RN::VRControllerTrackingState::BY] || handController[1].button[RN::VRControllerTrackingState::BY])
		{
			if(_airBubbleSize > 0.6f)
			{
				Airbubble *worldBubble = new Airbubble(_airBubbleEntity->GetWorldScale());
				world->AddLevelNode(worldBubble->Autorelease());
				worldBubble->SetWorldPosition(_airBubbleEntity->GetWorldPosition());
				
				_airBubbleSize = 0.5f;
				_airBubbleEntity->SetScale(RN::Vector3(_airBubbleSize, _airBubbleSize, _airBubbleSize));
			}
		}
/*		else
		{
			//TODO: Remove, only here for testing!
			_airBubbleSize = 1.3f;
			_airBubbleEntity->SetScale(RN::Vector3(_airBubbleSize, _airBubbleSize, _airBubbleSize));
		}*/
		
		if(_airBubbleSize > 0.6f)
		{
			Airbubble *airbubble = world->FindClosestAirbubble(_airBubbleEntity->GetWorldPosition(), nullptr);
			if(airbubble)
			{
				float distance = airbubble->GetWorldPosition().GetDistance(_airBubbleEntity->GetWorldPosition());
				if(distance < 3.0f)
				{
					if(airbubble->AddAir(_airBubbleEntity->GetWorldPosition(), _airBubbleSize))
					{
						_airBubbleSize = 0.5f;
						_airBubbleEntity->SetScale(RN::Vector3(_airBubbleSize, _airBubbleSize, _airBubbleSize));
					}
				}
			}
		}
		
		if(_eggCounter >= 5)
		{
			Airbubble *airbubble = world->FindClosestAirbubble(_airBubbleEntity->GetWorldPosition(), nullptr);
			if(airbubble && airbubble->CanBreed())
			{
				float distance = airbubble->GetWorldPosition().GetDistance(_airBubbleEntity->GetWorldPosition());
				if(distance < 3.0f && airbubble->IsInside(GetWorldPosition()))
				{
					_eggCounter = 0;
					_eggsEntity->SetScale(RN::Vector3(1.0f, 1.0f, 1.0f));
					
					Message *message = world->GetMessage();
					message->ShowMessage(RNCSTR("The end."), 5.0f, [&](){
						message->ShowMessage(RNCSTR("Keep playing if you want or just quit the game."), 5.0f, [&](){
							
						});
					});
				}
			}
		}
		
		_bodyEntity->SetWorldRotation(RN::Vector3(_head->GetWorldEulerAngle().x, 0.0f, 0.0f));

		SceneNode::Update(delta);
	}

	void Player::ResetThread(Thread *thread)
	{
		if(thread == _activeThread[0])
		{
			_activeThread[0] = nullptr;
		}
		else if(thread == _activeThread[1])
		{
			_activeThread[1] = nullptr;
		}
	}

	void Player::Eat()
	{
		if(_eggCounter >= 5) return;
		
		_eggCounter += 1;
		
		float size = _eggCounter * 0.2f + 1.0f;
		_eggsEntity->SetScale(RN::Vector3(size, size, size));
		
		if(_eggCounter == 5)
		{
			Message *message = World::GetSharedInstance()->GetMessage();
			message->ShowMessage(RNCSTR("You need a big air bubble to lay eggs in it."), 10.0f, [&](){
				
			});
		}
	}
}
