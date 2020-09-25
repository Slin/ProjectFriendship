//
//  PFWorld.h
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#ifndef __PF_WORLD_H_
#define __PF_WORLD_H_

#include <Rayne.h>

#include "RNPhysXWorld.h"

#include "PFTypes.h"
#include "PFCameraManager.h"
#include "PFPlayer.h"
#include "PFGoldfish.h"
#include "PFThread.h"
#include "PFAirbubble.h"

namespace PF
{
	class World : public RN::SceneBasic
	{
	public:
		static World *GetSharedInstance();
		static void Exit();

		World(RN::VRWindow *vrWindow);
		~World();

		RN::PhysXWorld *GetPhysicsWorld() const { return _physicsWorld; }
		RN::ShaderLibrary *GetShaderLibrary() const { return _shaderLibrary; }
		RN::VRCamera *GetVRCamera() const { return _cameraManager.GetVRCamera(); }
		RN::Camera *GetHeadCamera() const { return _cameraManager.GetHeadCamera(); }
		RN::Camera *GetPreviewCamera() const { return _cameraManager.GetPreviewCamera(); }
		CameraManager &GetCameraManager() { return _cameraManager; }
		
		Player *GetPlayer() const { return _player; }

		RN::Model *AssignShader(RN::Model *model, Types::MaterialType materialType) const;
		RN::Model *MakeDeepCopy(RN::Model *model) const;
		
		void AddLevelNode(RN::SceneNode *node);
		void RemoveLevelNode(RN::SceneNode *node);
		void RemoveAllLevelNodes();
		
		void AddStaticAirbubble(Airbubble *airbubble);
		Airbubble *FindClosestAirbubble(RN::Vector3 position, Airbubble *exclude);
		bool DoesVoxelOverlap(RN::Vector3 position, RN::Quaternion rotation);
		
		bool GetIsDash() const { return _isDash; }
		
		void LoadLevel();

	protected:
		void WillBecomeActive() override;
		void DidBecomeActive() override;

		void WillUpdate(float delta) override;

		CameraManager _cameraManager;
		RN::VRWindow *_vrWindow;
		
		RN::ShaderLibrary *_shaderLibrary;
		RN::Array *_levelNodes;
		RN::Array *_staticAirbubbles;
		Player *_player;

		RN::PhysXWorld *_physicsWorld;
		RN::PhysXBoxShape *_voxelOverlapShape;
		
		bool _isPaused;
		bool _isDash;

		static World *_sharedInstance;
	};
}


#endif /* __PF_WORLD_H_ */
