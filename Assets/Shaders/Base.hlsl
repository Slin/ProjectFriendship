//
//  Base.hlsl
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#ifndef PF_FOG
#define PF_FOG 0
#endif

#ifndef PF_CAUSTICS
#define PF_CAUSTICS 0
#endif

#include "rayne.hlsl"

#if RN_UV0 || PF_CAUSTICS
SamplerState linearRepeatSampler;
Texture2D texture0;

#if RN_UV0 && PF_CAUSTICS
	Texture2D texture1;
#endif
#endif

cbuffer vertexUniforms
{
	matrix modelMatrix;
	matrix modelViewProjectionMatrix;

	RN_ANIMATION_VERTEX_UNIFORMS

	float4 ambientColor;
	float4 diffuseColor;

#if PF_FOG
	float3 cameraPosition;
#endif

#if PF_CAUSTICS
	float time;
#endif
};

cbuffer fragmentUniforms
{
	float4 cameraAmbientColor;
};

struct InputVertex
{
	[[vk::location(0)]] float3 position : POSITION;
	[[vk::location(1)]] float3 normal : NORMAL;

#if RN_COLOR
	[[vk::location(3)]] float4 color : COLOR;
#endif

#if RN_UV0
	[[vk::location(5)]] float2 texCoords : TEXCOORD0;
#endif

	RN_ANIMATION_VERTEX_DATA
};

struct FragmentVertex
{
	float4 position : SV_POSITION;
	half4 color : TEXCOORD1;

#if PF_CAUSTICS
	half4 causticCoords : TEXCOORD3;
	half causticBlend : TEXCOORD4;
#endif

#if PF_FOG || PF_CAUSTICS
	half3 worldPosition : TEXCOORD2;
#endif

#if PF_FOG
	half3 fogDir : TEXCOORD5;
#endif

#if RN_UV0
	half2 texCoords : TEXCOORD0;
#endif
};

FragmentVertex main_vertex(InputVertex vert)
{
	FragmentVertex result;

#if RN_UV0
	result.texCoords = vert.texCoords;
#endif

	float4 position = RN_ANIMATION_TRANSFORM(float4(vert.position, 1.0), vert)
	float4 normal = RN_ANIMATION_TRANSFORM(float4(vert.normal, 0.0), vert)

#if PF_CAUSTICS || PF_FOG
	result.worldPosition = mul(modelMatrix, position).xyz;
#endif

#if PF_FOG
	result.fogDir = result.worldPosition - cameraPosition;
	half correctionDistance = max(result.worldPosition.y + 20.0, 0.0);
	result.fogDir -= result.fogDir * (correctionDistance / result.fogDir.y);
#endif

#if PF_CAUSTICS
	result.causticCoords.xy = result.worldPosition.xz * 0.01 + time * 0.01;
	result.causticCoords.zw = -result.worldPosition.xz * 0.01 + time * 0.01;
	result.causticBlend = saturate(vert.normal.y) * saturate((200.0 + result.worldPosition.y)/200.0);
#endif

	result.position = mul(modelViewProjectionMatrix, position);

	half light = normal.y + 1.5;
#if RN_COLOR
	result.color = vert.color * vert.color * diffuseColor * ambientColor * light;
#else
	result.color = diffuseColor * ambientColor * light;
#endif

	return result;
}


half4 main_fragment(FragmentVertex vert) : SV_TARGET
{
	half4 color = vert.color;

#if RN_UV0
	color *= texture0.Sample(linearRepeatSampler, vert.texCoords).rgba;
	#if PF_CAUSTICS
		half caustics = texture1.Sample(linearRepeatSampler, vert.causticCoords.xy).r;
	caustics += texture1.Sample(linearRepeatSampler, vert.causticCoords.zw).r;
	#endif
#elif PF_CAUSTICS
	half caustics = texture0.Sample(linearRepeatSampler, vert.causticCoords.xy).r;
	caustics += texture0.Sample(linearRepeatSampler, vert.causticCoords.zw).r;
#endif
#if PF_CAUSTICS
	if(vert.worldPosition.y < -20.0) color += caustics * vert.causticBlend;
#endif

#if PF_FOG
	half fogFactor = saturate(dot(vert.fogDir, vert.fogDir) * 0.000005);
	color.rgb = lerp(color.rgb, half3(0.0, 0.1, 0.09), fogFactor);
#endif

	color.rgb *= cameraAmbientColor.rgb;
	color.a = 1.0;
	return color;
}
