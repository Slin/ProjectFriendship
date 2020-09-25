//
//  PFMessage.h
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#include "PFMessage.h"
#include "PFWorld.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

#define FONT_SIZE 0.2f
#define TEXT_DISTANCE 3.0f

namespace PF
{
	RNDefineMeta(Message, RN::SceneNode)

	Message::Message()
	{
		RN::Shader::Options *shaderOptions = RN::Shader::Options::WithNone();
		shaderOptions->AddDefine(RNCSTR("RN_UV0"), RNCSTR("1"));
		shaderOptions->EnableAlpha();
		RN::Shader *vertexShader = World::GetSharedInstance()->GetShaderLibrary()->GetShaderWithName(RNCSTR("text_vertex"), shaderOptions);
		RN::Shader *fragmentShader = World::GetSharedInstance()->GetShaderLibrary()->GetShaderWithName(RNCSTR("text_fragment"), shaderOptions);
		
		_textMaterial = RN::Material::WithShaders(vertexShader, fragmentShader)->Retain();
		_textMaterial->SetAlphaToCoverage(true);
		_textMaterial->SetCullMode(RN::CullMode::None);
		
		RN::Data *fontData = RN::Data::WithContentsOfFile(RNCSTR("fonts/NotoSans-Regular.ttf"));
		fontData->Retain();
		stbtt_InitFont(&_font, fontData->GetBytes<unsigned char>(), stbtt_GetFontOffsetForIndex(fontData->GetBytes<unsigned char>(), 0));
		
		AddFlags(RN::SceneNode::Flags::Hidden);
	}
	
	Message::~Message()
	{
		
	}
	
	void Message::Update(float delta)
	{
		if(delta > 0.2f)
		{
			delta = 0.2f;
		}
		
		_duration -= delta;
		if(_duration <= 0.0f && !HasFlags(RN::SceneNode::Flags::Hidden))
		{
			AddFlags(RN::SceneNode::Flags::Hidden);
			RemoveMessage();
			
			if(_callback)
			{
				_callback();
			}
		}
		
		SceneNode::Update(delta);
	}

	void Message::RemoveMessage()
	{
		RN::Array *children = GetChildren()->Copy();
		children->Enumerate<RN::SceneNode>([&](RN::SceneNode *node, size_t index, bool &stop){
			RemoveChild(node);
		});
	}

	void Message::ShowMessage(RN::String *message, float duration, const std::function<void()> &callback)
	{
		RemoveMessage();
		
		int xpos = 0;
		for(int i = 0; i < message->GetLength(); i++)
		{
			RN::Entity *character = GetCharacterEntity(message->GetCharacterAtIndex(i));
			if(character)
			{
				AddChild(character);
				character->SetScale(FONT_SIZE / 1024.0f);
				character->SetPosition(RN::Vector3(FONT_SIZE*xpos/1024.0f, 0.0f, 0.0f));
			}
			
			int advance, lsb;
			stbtt_GetCodepointHMetrics(&_font, message->GetCharacterAtIndex(i), &advance, &lsb);
			xpos += advance - lsb;
			if(i < message->GetLength()-1)
				xpos += stbtt_GetCodepointKernAdvance(&_font, message->GetCharacterAtIndex(i), message->GetCharacterAtIndex(i+1));
		}
		
		Player *player = World::GetSharedInstance()->GetPlayer();
		RN::SceneNode *head = player->GetHead();
		SetPosition(RN::Vector3(-0.5f*FONT_SIZE*xpos/1024.0f, -0.5f * FONT_SIZE, -TEXT_DISTANCE));
		
		RemoveFlags(RN::SceneNode::Flags::Hidden);
		_duration = duration;
		_callback = callback;
	}

	RN::Entity *Message::GetCharacterEntity(int character)
	{
		RN::Model *characterModel = nullptr;
		if(_characters.count(character) > 0) characterModel = _characters[character];
		
		if(!characterModel)
		{
			stbtt_vertex *shapeVertices;
			int shapeVertexCount = stbtt_GetCodepointShape(&_font, character, &shapeVertices);
			
			KG::PathCollection paths;
			KG::Path mypath;
			KG::Vector2 from;
			for(int i = 0; i < shapeVertexCount; i++)
			{
				stbtt_vertex vertex = shapeVertices[i];
				if(vertex.type == STBTT_vmove)
				{
					if(mypath.segments.size() > 0)
					{
						paths.paths.push_back(mypath);
						mypath.segments.clear();
					}
					
					from.x = vertex.x;
					from.y = vertex.y;
				}
				else if(vertex.type == STBTT_vline)
				{
					KG::PathSegment segment;
					
					KG::Vector2 to;
					to.x = vertex.x;
					to.y = vertex.y;
					
					segment.type = KG::PathSegment::TypeLine;
					segment.controlPoints.push_back(from);
					segment.controlPoints.push_back(to);
					
					mypath.segments.push_back(segment);
					
					from = to;
				}
				else if(vertex.type == STBTT_vcurve)
				{
					KG::PathSegment segment;
					segment.type = KG::PathSegment::TypeBezierQuadratic;
					
					KG::Vector2 to;
					to.x = vertex.x;
					to.y = vertex.y;
					
					KG::Vector2 control;
					control.x = vertex.cx;
					control.y = vertex.cy;
					
					segment.controlPoints.push_back(from);
					segment.controlPoints.push_back(control);
					segment.controlPoints.push_back(to);
					
					mypath.segments.push_back(segment);
					
					from = to;
				}
				else if(vertex.type == STBTT_vcubic)
				{
					KG::PathSegment segment;
					segment.type = KG::PathSegment::TypeBezierCubic;
					
					KG::Vector2 to;
					to.x = vertex.x;
					to.y = vertex.y;
					
					KG::Vector2 control[2];
					control[0].x = vertex.cx;
					control[0].y = vertex.cy;
					control[1].x = vertex.cx1;
					control[1].y = vertex.cy1;
					
					segment.controlPoints.push_back(from);
					segment.controlPoints.push_back(control[0]);
					segment.controlPoints.push_back(control[1]);
					segment.controlPoints.push_back(to);
					
					mypath.segments.push_back(segment);
					
					from = to;
				}
			}
			
			if(mypath.segments.size() > 0)
			{
				paths.paths.push_back(mypath);
			}
			
			if(paths.paths.size() == 0)
			{
				return nullptr;
			}
			
			KG::TriangleMesh triangleMesh = KG::MeshGeneratorLoopBlinn::GetMeshForPathCollection(paths, false);
			
			characterModel = ModelForTriangleMesh(triangleMesh);
			_characters[character] = characterModel->Retain();
		}
		
		RN::Entity *entity = new RN::Entity(characterModel);
		
		return entity->Autorelease();
	}

	RN::Model *Message::ModelForTriangleMesh(const KG::TriangleMesh &triangleMesh)
	{
		std::vector<RN::Mesh::VertexAttribute> attributes;
		attributes.emplace_back(RN::Mesh::VertexAttribute::Feature::Indices, RN::PrimitiveType::Uint32);

		RN::int32 vertexFloatCount = 0;
		for(KG::TriangleMesh::VertexFeature feature : triangleMesh.features)
		{
			switch(feature)
			{
				case KG::TriangleMesh::VertexFeaturePosition:
					attributes.emplace_back(RN::Mesh::VertexAttribute::Feature::Vertices, RN::PrimitiveType::Vector2);
					vertexFloatCount += 2;
					break;
					
				case KG::TriangleMesh::VertexFeatureUV:
					attributes.emplace_back(RN::Mesh::VertexAttribute::Feature::UVCoords0, RN::PrimitiveType::Vector3);
					vertexFloatCount += 3;
					break;
					
				case KG::TriangleMesh::VertexFeatureColor:
					attributes.emplace_back(RN::Mesh::VertexAttribute::Feature::Color0, RN::PrimitiveType::Vector4);
					vertexFloatCount += 4;
					break;
			}
		}

		RN::uint32 verticesCount = triangleMesh.vertices.size() / vertexFloatCount;
		RN::Mesh *mesh = new RN::Mesh(attributes, verticesCount, triangleMesh.indices.size());
		mesh->BeginChanges();

		float *buildBuffer = new float[verticesCount * 4];
		
		#define CopyVertexData(elementSize, offset, feature) \
		{ \
			RN::uint32 dataIndex = 0; \
			RN::uint32 buildBufferIndex = 0; \
			for(size_t i = 0; i < verticesCount; i ++) \
			{ \
				std::copy(&triangleMesh.vertices[dataIndex + offset], &triangleMesh.vertices[dataIndex + offset + elementSize], &buildBuffer[buildBufferIndex]); \
				buildBufferIndex += elementSize; \
				dataIndex += vertexFloatCount; \
			} \
			mesh->SetElementData(feature, buildBuffer); \
		}
		
		RN::uint32 offset = 0;
		for(KG::TriangleMesh::VertexFeature feature : triangleMesh.features)
		{
			switch(feature)
			{
				case KG::TriangleMesh::VertexFeaturePosition:
					CopyVertexData(2, offset, RN::Mesh::VertexAttribute::Feature::Vertices);
					offset += 2;
					break;
					
				case KG::TriangleMesh::VertexFeatureUV:
					CopyVertexData(3, offset, RN::Mesh::VertexAttribute::Feature::UVCoords0);
					offset += 3;
					break;
					
				case KG::TriangleMesh::VertexFeatureColor:
					CopyVertexData(4, offset, RN::Mesh::VertexAttribute::Feature::Color0);
					offset += 4;
					break;
			}
		}

		delete[] buildBuffer;
		mesh->SetElementData(RN::Mesh::VertexAttribute::Feature::Indices, triangleMesh.indices.data());
		mesh->EndChanges();
		
		RN::Model *model = new RN::Model();
		model->AddLODStage(0.05f)->AddMesh(mesh->Autorelease(), _textMaterial);
		
		model->CalculateBoundingVolumes();
		
		return model->Autorelease();
	}
}
