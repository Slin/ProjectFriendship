//
//  Ground.hlsl
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

[[vk::binding(3)]] SamplerState linearRepeatSampler : register(s0);
[[vk::binding(4)]] Texture2D texture0 : register(t0);
[[vk::binding(5)]] Texture2D texture1 : register(t1);

[[vk::binding(1)]] cbuffer vertexUniforms : register(b0)
{
	matrix modelMatrix;
	matrix modelViewProjectionMatrix;

	float4 ambientColor;
	float4 diffuseColor;
	float time;
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
	[[vk::location(3)]] float4 color : COLOR;
};

struct FragmentVertex
{
	float4 position : SV_POSITION;
	half4 color : TEXCOORD0;
	half2 detailCoords : TEXCOORD1;
	half4 causticCoords : TEXCOORD2;
	half causticBlend : TEXCOORD3;
	half3 worldPosition : TEXCOORD4;
};

FragmentVertex ground_vertex(InputVertex vert)
{
	FragmentVertex result;

	result.position = mul(modelViewProjectionMatrix, float4(vert.position, 1.0));
	float light = vert.normal.y + 0.5;

	result.worldPosition = mul(modelMatrix, float4(vert.position, 1.0)).xyz;
	result.detailCoords = result.worldPosition.xz * 1.0;
	result.causticCoords.xy = result.worldPosition.xz * 0.1 + time * 0.01;
	result.causticCoords.zw = -result.worldPosition.xz * 0.1 + time * 0.01;
	result.causticBlend = saturate(vert.normal.y) * saturate((20.0 + result.worldPosition.y)/20.0);

	if(result.worldPosition.y > -2.0) result.causticBlend = 0.0;

	result.color = vert.color * diffuseColor * ambientColor * light;

	return result;
}


half4 ground_fragment(FragmentVertex vert) : SV_TARGET
{
	half4 color = vert.color;

	color *= texture0.Sample(linearRepeatSampler, vert.detailCoords).rgba;

	half caustics = texture1.Sample(linearRepeatSampler, vert.causticCoords.xy).r;
	caustics += texture1.Sample(linearRepeatSampler, vert.causticCoords.zw).r;
	color += caustics * vert.causticBlend;

	half3 cameraDir = vert.worldPosition - cameraPosition;
	half fogFactor = saturate(dot(cameraDir, cameraDir) * 0.0005);

	color.rgb = lerp(color.rgb, half3(0.0, 0.1, 0.09), fogFactor);

	return color * cameraAmbientColor;
}
