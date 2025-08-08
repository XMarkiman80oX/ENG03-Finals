#pragma once
#include <../Graphics/Shaders/Shaders.h>

namespace dx3d
{
    class GreenShader
    {
    public:
        static const char* GetVertexShaderCode()
        {
            return R"(
                struct VS_INPUT {
                    float3 position : POSITION;
                    float4 color : COLOR;
                };
                
                struct VS_OUTPUT {
                    float4 position : SV_POSITION;
                };
                
                VS_OUTPUT main(VS_INPUT input) {
                    VS_OUTPUT output;
                    output.position = float4(input.position, 1.0f);
                    return output;
                }
            )";
        }

        static const char* GetPixelShaderCode()
        {
            return R"(
                struct PS_INPUT {
                    float4 position : SV_POSITION;
                };
                
                float4 main(PS_INPUT input) : SV_TARGET {
                    return float4(0.0, 1.0, 0.0, 1.0); // Pure green
                }
            )";
        }
    };
}