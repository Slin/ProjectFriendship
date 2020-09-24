//
//  Base.hlsl
//  ProjectFriendship
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

[[vk::binding(1)]] cbuffer vertexUniforms : register(b0)
{
	matrix modelMatrix;
	matrix modelViewProjectionMatrix;

	float4 ambientColor;
	float4 diffuseColor;

	float3 cameraPosition;
};

[[vk::binding(2)]] cbuffer fragmentUniforms : register(b1)
{
	float4 cameraAmbientColor;
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
	half3 fogDir : TEXCOORD1;
};

FragmentVertex air_vertex(InputVertex vert)
{
	FragmentVertex result;

	result.position = mul(modelViewProjectionMatrix, float4(vert.position, 1.0));
	half3 worldPosition = mul(modelMatrix, float4(vert.position, 1.0)).xyz;

	half3 cameraDir = worldPosition - cameraPosition;
	result.fogDir = cameraDir;
	half correctionDistance = max(worldPosition.y + 20.0, 0.0);
	result.fogDir -= result.fogDir * (correctionDistance / result.fogDir.y);

	float3 worldNormal = normalize(mul(modelMatrix, float4(vert.normal, 0.0)).xyz);
	half light = worldNormal.y + 1.5;
	result.color.rgb = diffuseColor.rgb * ambientColor.rgb * light;
	result.color.a = saturate(1.0 + dot(worldNormal, normalize(cameraDir)));
	result.color.a *= result.color.a * 0.5;

	return result;
}


half4 air_fragment(FragmentVertex vert) : SV_TARGET
{
	half4 color = vert.color;

	half fogFactor = saturate(dot(vert.fogDir, vert.fogDir) * 0.000005);
	color.rgb = lerp(color.rgb, half3(0.0, 0.1, 0.09), fogFactor);

	color.rgb *= cameraAmbientColor.rgb;
	return color;
}
