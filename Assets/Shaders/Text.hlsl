//
//  Text.hlsl
//  Schriften
//
//  Copyright 2020 by SlinDev. All rights reserved.
//

#ifndef RN_UV0
#define RN_UV0 0
#endif

#ifndef RN_COLOR
#define RN_COLOR 0
#endif

[[vk::binding(1)]] cbuffer vertexUniforms : register(b0)
{
	matrix modelViewProjectionMatrix;
	float4 diffuseColor;
};

struct InputVertex
{
	[[vk::location(0)]] float2 position : POSITION;

#if RN_COLOR
	[[vk::location(3)]] float4 color : COLOR;
#endif

#if RN_UV0
	[[vk::location(5)]] float3 texCoords : TEXCOORD0;
#endif
};

struct FragmentVertex
{
	float4 position : SV_POSITION;
	half4 color : TEXCOORD1;

#if RN_UV0
	half3 texCoords : TEXCOORD0;
#endif
};

FragmentVertex text_vertex(InputVertex vert)
{
	FragmentVertex result;

#if RN_UV0
	result.texCoords = vert.texCoords;
#endif

	result.position = mul(modelViewProjectionMatrix, float4(vert.position, 0.0, 1.0));

#if RN_COLOR
	result.color = vert.color * diffuseColor;
#else
	result.color = diffuseColor;
#endif

	return result;
}


half4 text_fragment(FragmentVertex vert) : SV_TARGET
{
	half4 color = vert.color;

#if RN_UV0
	half curve = (vert.texCoords.x * vert.texCoords.x - vert.texCoords.y);

	half px = ddx(curve);
	half py = ddy(curve);
	half dist = curve / sqrt(px * px + py * py); //Normalize to pixelsize for anti aliasing

	color.a = saturate(0.5 - dist * vert.texCoords.z);
#endif

	return color;
}
