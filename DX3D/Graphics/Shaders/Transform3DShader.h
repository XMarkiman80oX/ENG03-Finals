#pragma once
#include <../Graphics/Shaders/Shaders.h>

namespace dx3d
{
    class Transform3DShader
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
                    float4 color : COLOR;
                };
                
                VS_OUTPUT main(VS_INPUT input) {
                    VS_OUTPUT output;
                    
                    // Transform the position from object space to world space
                    float4 worldPosition = mul(float4(input.position, 1.0f), world);
                    
                    // Transform from world space to view space
                    float4 viewPosition = mul(worldPosition, view);
                    
                    // Transform from view space to projection space
                    output.position = mul(viewPosition, projection);
                    
                    // Pass the color through
                    output.color = input.color;
                    
                    return output;
                }
            )";
        }

        static const char* GetPixelShaderCode()
        {
            return R"(
                struct PS_INPUT {
                    float4 position : SV_POSITION;
                    float4 color : COLOR;
                };
                
                float4 main(PS_INPUT input) : SV_TARGET {
                    return input.color;
                }
            )";
        }
    };
}