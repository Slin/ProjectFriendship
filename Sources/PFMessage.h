//
//  PFMessage.h
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#ifndef __PF_MESSAGE_H_
#define __PF_MESSAGE_H_

#include "Rayne.h"

#define STBTT_STATIC
#include "stb_truetype.h"

#include "KGMeshGeneratorLoopBlinn.h"

namespace PF
{
	class Message : public RN::SceneNode
	{
	public:
		Message();
		~Message() override;
		
		void ShowMessage(RN::String *message, float duration, const std::function<void()> &callback);
		void RemoveMessage();
		bool IsVisible() const { return !HasFlags(RN::SceneNode::Flags::Hidden); }
		
		void Update(float delta) override;
		
	private:
		RN::Entity *GetCharacterEntity(int character);
		RN::Model *ModelForTriangleMesh(const KG::TriangleMesh &triangleMesh);
		
		float _duration;
		
		std::function<void(void)> _callback;
		
		static std::map<int, RN::Model*> _characters;
		static RN::Material *_textMaterial;
		static stbtt_fontinfo _font;

		RNDeclareMeta(Message)
	};
}

#endif /* defined(__PF_MESSAGE_H_) */
