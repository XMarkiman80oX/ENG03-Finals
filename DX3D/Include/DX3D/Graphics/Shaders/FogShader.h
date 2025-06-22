#pragma once
#include <DX3D/Math/Math.h> // required because FogShaderConstants uses Vector3 and Vector4

namespace dx3d
{
	__declspec(align(16))
	struct FogShaderConstants
	{
		Vector4 fogColor;
		Vector3 cameraPosition;
		float fogStart;
		float fogEnd;
		float fogEnabled;
		float padding[2];
	};

	struct FogMaterialConstants
	{
		Vector4 baseColor;
		bool useVertexColor;
		float padding[3];
	};

	class FogShader
	{
	public:
		static const char* GetVertexShaderCode()
		{
			return R"(
				struct VS_INPUT {
					float4 position : POSITION;
					float4 color    : COLOR;
				};
				struct VS_OUTPUT {
					float4 position       : SV_POSITION;
					float4 color          : COLOR;
					float3 world_position : TEXCOORD0;
				};
				cbuffer TransformConstantBuffer : register(b0) {
					matrix world;
					matrix view;
					matrix projection;
				};
				VS_OUTPUT main(VS_INPUT input) {
					VS_OUTPUT output;
					output.position = mul(input.position, world);
					output.world_position = output.position.xyz;
					output.position = mul(output.position, view);
					output.position = mul(output.position, projection);
					output.color = input.color;
					return output;
				}
			)";
		}

		static const char* GetPixelShaderCode()
		{
			return R"(
				struct PS_INPUT {
					float4 position       : SV_POSITION;
					float4 color          : COLOR;
					float3 world_position : TEXCOORD0;
				};
				
				cbuffer FogSceneConstantBuffer : register(b1) {
					float4 fogColor;
					float3 cameraPosition;
					float fogStart;
					float fogEnd;
					bool fogEnabled;
				};

				cbuffer FogMaterialConstantBuffer : register(b2) {
					float4 baseColor;
					bool useVertexColor;
				};

				float4 main(PS_INPUT input) : SV_TARGET {
					float4 objectColor;

					if (useVertexColor) {
						// For rainbow objects, use the color from the vertex buffer
						objectColor = input.color;
					}
					else {
						// For the plane, use the solid color we passed in
						objectColor = baseColor;
					}

					if (fogEnabled) {
						float dist = distance(input.world_position, cameraPosition);
						float fogFactor = saturate((dist - fogStart) / (fogEnd - fogStart));
						objectColor = lerp(objectColor, fogColor, fogFactor);
					}
					return objectColor;
				}
			)";
		}
	};
}