#pragma once
#include <../Graphics/Shaders/Shaders.h>

namespace dx3d
{
    class GradientCubeShader
    {
    public:
        static const char* GetVertexShaderCode()
        {
            return R"(
                cbuffer TransformBuffer : register(b0)
                {
                    matrix world;
                    matrix view;
                    matrix projection;
                };

                struct VS_INPUT {
                    float3 position : POSITION;
                    float4 color : COLOR;
                };

                struct VS_OUTPUT {
                    float4 position : SV_POSITION;
                    float4 worldPos : TEXCOORD0; // Pass world position to pixel shader
                };

                VS_OUTPUT main(VS_INPUT input) {
                    VS_OUTPUT output;

                    // Transform the position from object space to world space
                    float4 worldPosition = mul(float4(input.position, 1.0f), world);
                    output.worldPos = worldPosition;

                    // Transform from world space to view space
                    float4 viewPosition = mul(worldPosition, view);

                    // Transform from view space to projection space
                    output.position = mul(viewPosition, projection);

                    return output;
                }
            )";
        }

        static const char* GetPixelShaderCode()
        {
            return R"(
                struct PS_INPUT {
                    float4 position : SV_POSITION;
                    float4 worldPos : TEXCOORD0;
                };

                float4 main(PS_INPUT input) : SV_TARGET {
                    // Normalize the world position to a [0, 1] range for color mapping
                    // Assumes the cluster is roughly within a -5 to +5 world space boundary
                    float3 normalizedPos = (input.worldPos.xyz / 5.0f) * 0.5f + 0.5f;

                    // Map the normalized position to an orange/yellow/cyan gradient
                    float r = normalizedPos.x * 0.8f + 0.2f;         // Mix in red/orange based on X
                    float g = normalizedPos.y * 0.6f + 0.1f;         // Mix in yellow/green based on Y
                    float b = (1.0f - normalizedPos.x) * 0.7f; // Mix in cyan based on inverse of X

                    return float4(r, g, b, 1.0);
                }
            )";
        }
    };
}