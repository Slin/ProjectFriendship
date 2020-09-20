//
//  Base.hlsl
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#ifndef RN_UV0
#define RN_UV0 0
#endif

#ifndef RN_COLOR
#define RN_COLOR 0
#endif

#ifndef PF_FOG
#define PF_FOG 0
#endif

#ifndef RN_ANIMATIONS
#define RN_ANIMATIONS 0
#endif

#ifndef PF_CAUSTICS
#define PF_CAUSTICS 0
#endif

#ifndef RN_MAX_BONES
#define RN_MAX_BONES 100
#endif

#if RN_UV0 || PF_CAUSTICS
[[vk::binding(3)]] SamplerState linearRepeatSampler : register(s0);
[[vk::binding(4)]] Texture2D texture0 : register(t0);

#if RN_UV0 && PF_CAUSTICS
	[[vk::binding(5)]] Texture2D texture1 : register(t0);
#endif
#endif

[[vk::binding(1)]] cbuffer vertexUniforms : register(b0)
{
	matrix modelMatrix;
	matrix modelViewProjectionMatrix;

#if RN_ANIMATIONS
	matrix boneMatrices[RN_MAX_BONES];
#endif

	float4 ambientColor;
	float4 diffuseColor;

#if PF_CAUSTICS
	float time;
#endif
};

[[vk::binding(2)]] cbuffer fragmentUniforms : register(b1)
{
	float4 cameraAmbientColor;

#if PF_FOG
	float3 cameraPosition;
#endif
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

#if RN_ANIMATIONS
	[[vk::location(7)]] float4 boneWeights : BONEWEIGHTS;
	[[vk::location(8)]] float4 boneIndices : BONEINDICES;
#endif
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

#if RN_UV0
	half2 texCoords : TEXCOORD0;
#endif
};

#if RN_ANIMATIONS
float4 getAnimatedPosition(float4 position, float4 weights, float4 indices)
{
	float4 pos1 = mul(boneMatrices[int(indices.x)], position);
	float4 pos2 = mul(boneMatrices[int(indices.y)], position);
	float4 pos3 = mul(boneMatrices[int(indices.z)], position);
	float4 pos4 = mul(boneMatrices[int(indices.w)], position);

	float4 pos = pos1 * weights.x + pos2 * weights.y + pos3 * weights.z + pos4 * weights.w;
	pos.w = position.w;

	return pos;
}
#endif

FragmentVertex main_vertex(InputVertex vert)
{
	FragmentVertex result;

#if RN_UV0
	result.texCoords = vert.texCoords;
#endif

#if RN_ANIMATIONS
	float4 position = getAnimatedPosition(float4(vert.position, 1.0), vert.boneWeights, vert.boneIndices);
	float4 normal = getAnimatedPosition(float4(vert.normal, 0.0), vert.boneWeights, vert.boneIndices);
#else
	float4 position = float4(vert.position, 1.0);
	float4 normal = float4(vert.normal, 0.0);
#endif

#if PF_CAUSTICS || PF_FOG
	result.worldPosition = mul(modelMatrix, position).xyz;
#endif

#if PF_CAUSTICS
	result.causticCoords.xy = result.worldPosition.xz * 0.1 + time * 0.01;
	result.causticCoords.zw = -result.worldPosition.xz * 0.1 + time * 0.01;
	result.causticBlend = saturate(vert.normal.y) * saturate(1.0 + vert.position.y);
#endif

	result.position = mul(modelViewProjectionMatrix, position);

	half light = normal.y * 0.5 + 1.5;
#if RN_COLOR
	result.color = vert.color * diffuseColor * ambientColor * light;
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
	if(vert.worldPosition.y < -2.0) color += caustics * vert.causticBlend;
#endif

#if PF_FOG
	half3 cameraDir = vert.worldPosition - cameraPosition;
	half correctionDistance = max(vert.worldPosition.y + 2.0, 0.0);
	cameraDir -= cameraDir * (correctionDistance / cameraDir.y);
	half fogFactor = saturate(dot(cameraDir, cameraDir) * 0.0005);
	color.rgb = lerp(color.rgb, half3(0.0, 0.1, 0.09), fogFactor);
#endif

	color.rgb *= cameraAmbientColor.rgb;
	return color;
}
