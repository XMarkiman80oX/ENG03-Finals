#pragma once
#include <../Graphics/Shaders/Shaders.h>

namespace dx3d
{
    class RainbowShader
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
                    // Pass screen position for rainbow calculation
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
                
                // Enhanced HSV to RGB conversion for more vibrant colors
                float3 hsv2rgb(float3 c) {
                    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
                    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
                    return c.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
                }
                
                float4 main(PS_INPUT input) : SV_TARGET {
                    // Create more obvious rainbow based on screen position
                    // Map from [-1,1] to [0,1] and then multiply by 2 for more color cycles
                    float normalizedX = (input.screenPos.x + 1.0) * 0.5;
                    
                    // Create multiple rainbow cycles for more vibrant effect
                    float hue = fmod(normalizedX * 3.0, 1.0); // 3 complete rainbow cycles
                    
                    // Max saturation and brightness for vivid colors
                    float saturation = 1.0;
                    float value = 0.95; // Slightly less than 1.0 to avoid pure white
                    
                    float3 rainbowColor = hsv2rgb(float3(hue, saturation, value));
                    
                    // Boost the colors even more for clarity
                    rainbowColor = saturate(rainbowColor * 1.1);
                    
                    return float4(rainbowColor, 1.0);
                }
            )";
        }
    };
}