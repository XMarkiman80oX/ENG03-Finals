#pragma once
#include <DX3D/Graphics/Shaders/Shaders.h>

namespace dx3d
{
    class CustomTriangleShader
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
                    float2 worldPos : TEXCOORD0;
                };
                
                VS_OUTPUT main(VS_INPUT input) {
                    VS_OUTPUT output;
                    output.position = float4(input.position, 1.0f);
                    output.worldPos = input.position.xy;
                    return output;
                }
            )";
        }

        static const char* GetPixelShaderCode()
        {
            return R"(
                struct PS_INPUT {
                    float4 position : SV_POSITION;
                    float2 worldPos : TEXCOORD0;
                };
                
                // HSV to RGB conversion
                float3 hsv2rgb(float3 c) {
                    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
                    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
                    return c.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
                }
                
                float4 main(PS_INPUT input) : SV_TARGET {
                    float2 pos = input.worldPos;
                    
                    // Create gradient based on position to match your images
                    // Yellow at top-left, red at bottom-left, blue at right
                    
                    // Calculate hue based on position
                    float angle = atan2(pos.y, pos.x + 0.5); // Offset to center the gradient
                    float distance = length(pos);
                    
                    // Map angle to hue (0-1)
                    float hue = (angle + 3.14159) / (2.0 * 3.14159); // Convert from [-π,π] to [0,1]
                    
                    // Adjust hue to match your color scheme
                    // Yellow (0.15) -> Red (0.0) -> Blue (0.67)
                    hue = fmod(hue * 1.5 + 0.15, 1.0); // Shift and scale for better colors
                    
                    // High saturation and brightness for vivid colors
                    float saturation = 0.9 + 0.1 * sin(distance * 10.0); // Slight variation
                    float brightness = 0.95;
                    
                    float3 color = hsv2rgb(float3(hue, saturation, brightness));
                    
                    // Boost intensity to match your vibrant images
                    color = saturate(color * 1.3);
                    
                    return float4(color, 1.0);
                }
            )";
        }
    };
}
