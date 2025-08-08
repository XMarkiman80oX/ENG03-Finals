#pragma once
#include <../Graphics/Shaders/Shaders.h>

namespace dx3d
{
    class SimpleBlendedShader
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
                    float2 screenPos : TEXCOORD0;
                };
                
                VS_OUTPUT main(VS_INPUT input) {
                    VS_OUTPUT output;
                    output.position = float4(input.position, 1.0f);
                    output.screenPos = input.position.xy;
                    return output;
                }
            )";
        }

        static const char* GetPixelShaderCode()
        {
            return R"(
                struct PS_INPUT {
                    float4 position : SV_POSITION;
                    float2 screenPos : TEXCOORD0;
                };
                
                // HSV to RGB conversion for rainbow
                float3 hsv2rgb(float3 c) {
                    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
                    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
                    return c.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
                }
                
                float4 main(PS_INPUT input) : SV_TARGET {
                    // Green base color (like your second image)
                    float3 baseColor = float3(0.2, 0.8, 0.4);
                    
                    // Rainbow color calculation
                    float normalizedX = (input.screenPos.x + 1.0) * 0.5;
                    float hue = fmod(normalizedX * 2.0, 1.0);
                    float3 rainbowColor = hsv2rgb(float3(hue, 0.9, 0.95));
                    
                    // You can modify this blend factor externally by changing shader constants
                    // For now, we'll use a time-based blend
                    // This will be controlled from the C++ code by switching shaders
                    return float4(baseColor, 1.0);
                }
            )";
        }
    };
}