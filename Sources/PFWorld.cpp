//
//  PFWorld.cpp
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#include "PFWorld.h"

namespace PF
{
	World *World::_sharedInstance = nullptr;

	World *World::GetSharedInstance()
	{
		return _sharedInstance;
	}

	void World::Exit()
	{
		if(_sharedInstance)
		{
			_sharedInstance->RemoveAttachment(_sharedInstance->_physicsWorld);

#if RN_PLATFORM_ANDROID
			if(_sharedInstance->_vrWindow)
			{
				_sharedInstance->_vrWindow->Release(); //Reference from VRCamera
				_sharedInstance->_vrWindow->Release(); //Reference from World
				_sharedInstance->_vrWindow->Release(); //Reference from Application
				//_sharedInstance->_vrWindow->StopRendering();
			}
#endif
		}
		
		exit(0);
		//RN::Kernel::GetSharedInstance()->Exit();
	}

	World::World(RN::VRWindow *vrWindow) : _vrWindow(nullptr), _physicsWorld(nullptr), _isPaused(false), _isDash(false), _shaderLibrary(nullptr)
	{
		_sharedInstance = this;

		if (vrWindow)
			_vrWindow = vrWindow->Retain();
		
		_levelNodes = new RN::Array();
		_staticAirbubbles = new RN::Array();
	}

	World::~World()
	{
		
	}

	void World::WillBecomeActive()
	{
		RN::Scene::WillBecomeActive();

		if(!RN::Renderer::IsHeadless())
		{
			RN::Renderer *activeRenderer = RN::Renderer::GetActiveRenderer();
			_shaderLibrary = activeRenderer->CreateShaderLibraryWithFile(RNCSTR("shaders/Shaders.json"));
			_cameraManager.Setup(_vrWindow);
		}

		RN::String *pvdServerIP = nullptr;
#if RN_BUILD_DEBUG
	#if RN_PLATFORM_WINDOWS
		pvdServerIP = RNCSTR("localhost");
	#else
		pvdServerIP = RNCSTR("192.168.178.22");
	#endif
#endif
		_physicsWorld = new RN::PhysXWorld(RN::Vector3(0.0f, -9.81f, 0.0f), pvdServerIP);
		AddAttachment(_physicsWorld->Autorelease());
		
		RN::PhysXMaterial *physicsMaterial = new RN::PhysXMaterial();
		_voxelOverlapShape = RN::PhysXBoxShape::WithHalfExtents(RN::Vector3(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f), physicsMaterial->Autorelease());
		_voxelOverlapShape->Retain();

		LoadLevel();
		
		_player = new Player();
		AddNode(_player->Autorelease());
		_player->SetWorldPosition(RN::Vector3(968.0f, -200.0f, -980.0f));
		_cameraManager.SetFreeCamera(false);
	}

	void World::DidBecomeActive()
	{
		RN::SceneBasic::DidBecomeActive();
		_cameraManager.SetCameraAmbientColor(RN::Color::White(), 1.0f, nullptr);
	}

	void World::WillUpdate(float delta)
	{
		RN::Scene::WillUpdate(delta);

		_isPaused = false;
		_isDash = false;
		RN::VRHMDTrackingState::Mode headsetState = _cameraManager.Update(delta);
		if(headsetState == RN::VRHMDTrackingState::Mode::Paused)
		{
			_isPaused = true;
			_isDash = true;
		}
		else if(headsetState == RN::VRHMDTrackingState::Mode::Disconnected)
		{
			Exit();
		}

		if(RN::InputManager::GetSharedInstance()->IsControlToggling(RNCSTR("ESC")))
		{
			Exit();
		}
	}

	RN::Model *World::AssignShader(RN::Model *model, Types::MaterialType materialType) const
	{
		if(RN::Renderer::IsHeadless()) return model;
		
		World *world = World::GetSharedInstance();
		RN::ShaderLibrary *shaderLibrary = world->GetShaderLibrary();

		RN::Model::LODStage *lodStage = model->GetLODStage(0);
		for(int i = 0; i < lodStage->GetCount(); i++)
		{
			RN::Material *material = lodStage->GetMaterialAtIndex(i);
			
			switch(materialType)
			{
				case Types::MaterialDefault:
				{
					material->SetDepthWriteEnabled(true);
					material->SetDepthMode(RN::DepthMode::LessOrEqual);
					material->SetAlphaToCoverage(false);
					material->SetAmbientColor(RN::Color::White());
					RN::Shader::Options *shaderOptions = RN::Shader::Options::WithMesh(lodStage->GetMeshAtIndex(i));
					shaderOptions->AddDefine(RNCSTR("PF_FOG"), RNCSTR("1"));
					shaderOptions->AddDefine(RNCSTR("PF_CAUSTICS"), RNCSTR("1"));
					material->SetVertexShader(shaderLibrary->GetShaderWithName(RNCSTR("main_vertex"), shaderOptions));
					material->SetFragmentShader(shaderLibrary->GetShaderWithName(RNCSTR("main_fragment"), shaderOptions));
					material->AddTexture(RN::Texture::WithName(RNCSTR("models/caustics.*")));
					break;
				}
					
				case Types::MaterialGround:
				{
					material->SetDepthWriteEnabled(true);
					material->SetDepthMode(RN::DepthMode::LessOrEqual);
					material->SetAlphaToCoverage(false);
					material->SetAmbientColor(RN::Color::White());
					RN::Shader::Options *shaderOptions = RN::Shader::Options::WithMesh(lodStage->GetMeshAtIndex(i));
					material->SetVertexShader(shaderLibrary->GetShaderWithName(RNCSTR("ground_vertex"), shaderOptions));
					material->SetFragmentShader(shaderLibrary->GetShaderWithName(RNCSTR("ground_fragment"), shaderOptions));
					material->AddTexture(RN::Texture::WithName(RNCSTR("models/ground_detail.*")));
					material->AddTexture(RN::Texture::WithName(RNCSTR("models/caustics.*")));
					break;
				}
					
				case Types::MaterialMoving:
				{
					material->SetDepthWriteEnabled(true);
					material->SetDepthMode(RN::DepthMode::LessOrEqual);
					material->SetAlphaToCoverage(false);
					material->SetCullMode(RN::CullMode::None);
					material->SetAmbientColor(RN::Color::White());
					RN::Shader::Options *shaderOptions = RN::Shader::Options::WithMesh(lodStage->GetMeshAtIndex(i));
					shaderOptions->AddDefine(RNCSTR("PF_FOG"), RNCSTR("1"));
					shaderOptions->AddDefine(RNCSTR("PF_CAUSTICS"), RNCSTR("1"));
					material->SetVertexShader(shaderLibrary->GetShaderWithName(RNCSTR("main_vertex"), shaderOptions));
					material->SetFragmentShader(shaderLibrary->GetShaderWithName(RNCSTR("main_fragment"), shaderOptions));
					material->AddTexture(RN::Texture::WithName(RNCSTR("models/caustics.*")));
					break;
				}
					
				case Types::MaterialWater:
				{
					material->SetDepthWriteEnabled(true);
					material->SetDepthMode(RN::DepthMode::LessOrEqual);
					material->SetAlphaToCoverage(false);
					material->SetBlendOperation(RN::BlendOperation::Add, RN::BlendOperation::Add);
					material->SetCullMode(RN::CullMode::FrontFace);
					material->SetAmbientColor(RN::Color::White());
					RN::Shader::Options *shaderOptions = RN::Shader::Options::WithMesh(lodStage->GetMeshAtIndex(i));
					material->SetVertexShader(shaderLibrary->GetShaderWithName(RNCSTR("water_vertex"), shaderOptions));
					material->SetFragmentShader(shaderLibrary->GetShaderWithName(RNCSTR("water_fragment"), shaderOptions));
					break;
				}
					
				case Types::MaterialPlayer:
				{
					material->SetDepthWriteEnabled(true);
					material->SetDepthMode(RN::DepthMode::LessOrEqual);
					material->SetAlphaToCoverage(false);
					material->SetCullMode(RN::CullMode::BackFace);
					material->SetAmbientColor(RN::Color::White());
					RN::Shader::Options *shaderOptions = RN::Shader::Options::WithMesh(lodStage->GetMeshAtIndex(i));
					shaderOptions->AddDefine(RNCSTR("PF_CAUSTICS"), RNCSTR("1"));
					material->SetVertexShader(shaderLibrary->GetShaderWithName(RNCSTR("main_vertex"), shaderOptions));
					material->SetFragmentShader(shaderLibrary->GetShaderWithName(RNCSTR("main_fragment"), shaderOptions));
					material->AddTexture(RN::Texture::WithName(RNCSTR("models/caustics.*")));
					break;
				}
					
				case Types::MaterialAirbubble:
				{
					material->SetDepthWriteEnabled(false);
					material->SetBlendOperation(RN::BlendOperation::Add, RN::BlendOperation::Add);
					material->SetDepthMode(RN::DepthMode::LessOrEqual);
					material->SetAlphaToCoverage(false);
					material->SetCullMode(RN::CullMode::BackFace);
					material->SetAmbientColor(RN::Color::WithRGBA(1.0f, 1.0f, 1.0f, 0.5f));
					RN::Shader::Options *shaderOptions = RN::Shader::Options::WithMesh(lodStage->GetMeshAtIndex(i));
					material->SetVertexShader(shaderLibrary->GetShaderWithName(RNCSTR("air_vertex"), shaderOptions));
					material->SetFragmentShader(shaderLibrary->GetShaderWithName(RNCSTR("air_fragment"), shaderOptions));
					break;
				}
			}
		}

		return model;
	}

	RN::Model *World::MakeDeepCopy(RN::Model *model) const
	{
		RN::Model *result = model->Copy();
		
		RN::Model::LODStage *lodStage = result->GetLODStage(0);
		for(int i = 0; i < lodStage->GetCount(); i++)
		{
			RN::Material *material = lodStage->GetMaterialAtIndex(i)->Copy();
			lodStage->ReplaceMaterial(material->Autorelease(), i);
		}
		
		return result->Autorelease();
	}

	void World::AddLevelNode(RN::SceneNode *node)
	{
		_levelNodes->AddObject(node);
		AddNode(node);
	}

	void World::RemoveLevelNode(RN::SceneNode *node)
	{
		_levelNodes->RemoveObject(node);
		_staticAirbubbles->RemoveObject(node);
		RemoveNode(node);
	}

	void World::AddStaticAirbubble(Airbubble *airbubble)
	{
		_staticAirbubbles->AddObject(airbubble);
	}

	void World::RemoveAllLevelNodes()
	{
		_levelNodes->Enumerate<RN::SceneNode>([&](RN::SceneNode *node, size_t index, bool &stop){
			RemoveNode(node);
		});
		
		_levelNodes->RemoveAllObjects();
		_staticAirbubbles->RemoveAllObjects();
	}

	Airbubble *World::FindClosestAirbubble(RN::Vector3 position, Airbubble *exclude)
	{
		float closestDistance = -1.0f;
		Airbubble *closestBubble = nullptr;
		_staticAirbubbles->Enumerate<Airbubble>([&](Airbubble *bubble, size_t index, bool &stop){
			if(bubble == exclude) return;
			float distance = bubble->GetWorldPosition().GetSquaredDistance(position);
			if(!closestBubble || distance < closestDistance)
			{
				closestBubble = bubble;
				closestDistance = distance;
			}
		});
		
		return closestBubble;
	}

	bool World::DoesVoxelOverlap(RN::Vector3 position, RN::Quaternion rotation)
	{
		const std::vector<RN::PhysXContactInfo> &results = _physicsWorld->CheckOverlap(_voxelOverlapShape, position, rotation);
		return results.size() > 0;
	}

	void World::LoadLevel()
	{
		RemoveAllLevelNodes();
		
		RN::Model *skyModel = RN::Model::WithName(RNCSTR("models/sky.sgm"));
		RN::Material *skyMaterial = RN::Material::WithShaders(nullptr, nullptr);
		skyMaterial->SetDepthMode(RN::DepthMode::LessOrEqual);

		RN::Shader::Options *skyShaderOptions = RN::Shader::Options::WithMesh(skyModel->GetLODStage(0)->GetMeshAtIndex(0));
		skyShaderOptions->AddDefine(RNCSTR("RN_SKY"), RNCSTR("1"));
		skyMaterial->SetVertexShader(RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Vertex, skyShaderOptions, RN::Shader::UsageHint::Default));
		skyMaterial->SetFragmentShader(RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Fragment, skyShaderOptions, RN::Shader::UsageHint::Default));
		skyMaterial->SetVertexShader(RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Vertex, skyShaderOptions, RN::Shader::UsageHint::Depth), RN::Shader::UsageHint::Depth);
		skyMaterial->SetFragmentShader(RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Fragment, skyShaderOptions, RN::Shader::UsageHint::Depth), RN::Shader::UsageHint::Depth);
		skyModel->GetLODStage(0)->ReplaceMaterial(skyMaterial, 0);
		
		RN::Entity *skyEntity = new RN::Entity(skyModel);
		skyEntity->SetScale(RN::Vector3(10.0f));
		//skyEntity->AddFlags(RN::Entity::Flags::DrawLate);
		AddLevelNode(skyEntity->Autorelease());
		
		RN::Model *levelModel = AssignShader(RN::Model::WithName(RNCSTR("models/ground.sgm")), Types::MaterialGround);
		RN::Entity *levelEntity = new RN::Entity(levelModel);
		AddLevelNode(levelEntity->Autorelease());

		RN::PhysXMaterial *levelPhysicsMaterial = new RN::PhysXMaterial();
		RN::PhysXCompoundShape *levelShape = RN::PhysXCompoundShape::WithModel(levelModel, levelPhysicsMaterial->Autorelease(), true);
		RN::PhysXStaticBody *levelBody = RN::PhysXStaticBody::WithShape(levelShape);
		levelBody->SetCollisionFilter(Types::CollisionLevel, Types::CollisionAll);
		levelEntity->AddAttachment(levelBody);
		
		RN::Model *reedModel = AssignShader(RN::Model::WithName(RNCSTR("models/reed.sgm")), Types::MaterialMoving);
		RN::Entity *reedEntity = new RN::Entity(reedModel);
		AddLevelNode(reedEntity->Autorelease());
		
		RN::PhysXCompoundShape *reedShape = RN::PhysXCompoundShape::WithModel(reedModel, levelPhysicsMaterial->Autorelease(), true, true);
		RN::PhysXStaticBody *reedBody = RN::PhysXStaticBody::WithShape(reedShape);
		reedBody->SetCollisionFilter(Types::CollisionPlants, Types::CollisionAll);
		reedEntity->AddAttachment(reedBody);
		
		RN::Model *stonesModel = AssignShader(RN::Model::WithName(RNCSTR("models/stones.sgm")), Types::MaterialDefault);
		RN::Entity *stonesEntity = new RN::Entity(stonesModel);
		AddLevelNode(stonesEntity->Autorelease());
		
		RN::PhysXCompoundShape *stonesShape = RN::PhysXCompoundShape::WithModel(stonesModel, levelPhysicsMaterial->Autorelease(), true);
		RN::PhysXStaticBody *stonesBody = RN::PhysXStaticBody::WithShape(stonesShape);
		stonesBody->SetCollisionFilter(Types::CollisionLevel, Types::CollisionAll);
		stonesEntity->AddAttachment(stonesBody);
		
		RN::Model *grassModel = AssignShader(RN::Model::WithName(RNCSTR("models/grass.sgm")), Types::MaterialMoving);
		RN::Entity *grassEntity = new RN::Entity(grassModel);
		AddLevelNode(grassEntity->Autorelease());
		
		RN::PhysXCompoundShape *grassShape = RN::PhysXCompoundShape::WithModel(grassModel, levelPhysicsMaterial->Autorelease(), true, true);
		RN::PhysXStaticBody *grassBody = RN::PhysXStaticBody::WithShape(grassShape);
		grassBody->SetCollisionFilter(Types::CollisionPlants, Types::CollisionAll);
		grassEntity->AddAttachment(grassBody);
		
		RN::Model *lilysModel = AssignShader(RN::Model::WithName(RNCSTR("models/waterlily.sgm")), Types::MaterialMoving);
		RN::Entity *lilysEntity = new RN::Entity(lilysModel);
		AddLevelNode(lilysEntity->Autorelease());
		
		RN::PhysXCompoundShape *waterlilyShape = RN::PhysXCompoundShape::WithModel(lilysModel, levelPhysicsMaterial->Autorelease(), true, true);
		RN::PhysXStaticBody *waterlilyBody = RN::PhysXStaticBody::WithShape(waterlilyShape);
		waterlilyBody->SetCollisionFilter(Types::CollisionPlants, Types::CollisionAll);
		lilysEntity->AddAttachment(waterlilyBody);
		
		RN::Model *goldfishModel = AssignShader(RN::Model::WithName(RNCSTR("models/goldfish.sgm")), Types::MaterialDefault);
		Goldfish *goldfishEntity = new Goldfish(goldfishModel);
		AddLevelNode(goldfishEntity->Autorelease());
		goldfishEntity->SetWorldPosition(RN::Vector3(616.16, -200.0, 304.364));
		
		goldfishEntity = new Goldfish(goldfishModel);
		AddLevelNode(goldfishEntity->Autorelease());
		goldfishEntity->SetWorldPosition(RN::Vector3(583.476, -100.0, 1074.92));
		
		goldfishEntity = new Goldfish(goldfishModel);
		AddLevelNode(goldfishEntity->Autorelease());
		goldfishEntity->SetWorldPosition(RN::Vector3(-522.7, -150.0, -728.885));
		
		RN::Model *waterModel = AssignShader(RN::Model::WithName(RNCSTR("models/water.sgm")), Types::MaterialWater);
		RN::Entity *waterEntity = new RN::Entity(waterModel);
		waterEntity->SetWorldPosition(RN::Vector3(0.0f, -20.0f, 0.0f));
		waterEntity->SetFlags(RN::Entity::Flags::DrawLate);
		AddLevelNode(waterEntity->Autorelease());
	}
}
