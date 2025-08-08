#pragma once
#include <../Graphics/Shaders/Shaders.h>

namespace dx3d
{
    class Rainbow3DShader
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
                    float3 worldPos : TEXCOORD0;
                };
                
                VS_OUTPUT main(VS_INPUT input) {
                    VS_OUTPUT output;
                    
                    // Transform the position from object space to world space
                    float4 worldPosition = mul(float4(input.position, 1.0f), world);
                    output.worldPos = worldPosition.xyz;
                    
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
                    float3 worldPos : TEXCOORD0;
                };
                
                // Enhanced HSV to RGB conversion for vibrant rainbow colors
                float3 hsv2rgb(float3 c) {
                    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
                    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
                    return c.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
                }
                
                float4 main(PS_INPUT input) : SV_TARGET {
                    // Create rainbow based on world position
                    float3 pos = input.worldPos;
                    
                    // Calculate hue based on X and Z position
                    float hue = (pos.x + pos.z) * 0.1 + length(pos) * 0.05;
                    hue = fmod(hue, 1.0); // Keep hue in [0,1] range
                    
                    // High saturation and brightness for vivid colors
                    float saturation = 0.9;
                    float brightness = 0.95;
                    
                    float3 rainbowColor = hsv2rgb(float3(hue, saturation, brightness));
                    
                    // Boost the colors for maximum vibrancy
                    rainbowColor = saturate(rainbowColor * 1.2);
                    
                    return float4(rainbowColor, 1.0);
                }
            )";
        }
    };
}