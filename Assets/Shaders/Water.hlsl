//
//  Water.hlsl
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

//[[vk::binding(3)]] SamplerState linearRepeatSampler : register(s0);
//[[vk::binding(4)]] Texture2D texture0 : register(t0);

[[vk::binding(1)]] cbuffer vertexUniforms : register(b0)
{
	matrix modelMatrix;
	matrix modelViewProjectionMatrix;

	float4 ambientColor;
	float4 diffuseColor;
	//float time;
};

[[vk::binding(2)]] cbuffer fragmentUniforms : register(b1)
{
	float4 cameraAmbientColor;
	float3 cameraPosition;
};

struct InputVertex
{
	[[vk::location(0)]] float3 position : POSITION;
	[[vk::location(1)]] float3 normal : NORMAL;
};

struct FragmentVertex
{
	float4 position : SV_POSITION;
	half4 color : TEXCOORD0;
	half4 causticCoords : TEXCOORD1;
	half causticBlend : TEXCOORD2;
	half3 worldPosition : TEXCOORD3;
};

FragmentVertex water_vertex(InputVertex vert)
{
	FragmentVertex result;

	result.position = mul(modelViewProjectionMatrix, float4(vert.position, 1.0));
	result.worldPosition = mul(modelMatrix, float4(vert.position, 1.0)).xyz;
	result.color = diffuseColor * ambientColor;

	return result;
}


half4 water_fragment(FragmentVertex vert) : SV_TARGET
{
	half4 color = vert.color;

	//color *= texture0.Sample(linearRepeatSampler, vert.detailCoords).rgba;

	half3 cameraDir = vert.worldPosition - cameraPosition;
	half fogFactor = saturate(dot(cameraDir, cameraDir) * 0.000005);

	color.rgb = lerp(color.rgb, half3(0.0, 0.1, 0.09), fogFactor);

	color.a = saturate(0.5 + fogFactor);

	return color * cameraAmbientColor;
}
