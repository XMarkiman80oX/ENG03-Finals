#pragma once
#include <../Graphics/Shaders/Shaders.h>

namespace dx3d
{
    class TransitionShader
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
                    float4 worldPos : TEXCOORD1;
                };
                
                VS_OUTPUT main(VS_INPUT input) {
                    VS_OUTPUT output;
                    output.position = float4(input.position, 1.0f);
                    output.screenPos = input.position.xy;
                    output.worldPos = float4(input.position, 1.0f);
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
                    float4 worldPos : TEXCOORD1;
                };
                
                // HSV to RGB conversion for rainbow
                float3 hsv2rgb(float3 c) {
                    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
                    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
                    return c.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
                }
                
                float4 main(PS_INPUT input) : SV_TARGET {
                    // Create yellow-to-blue gradient (like your reference image)
                    float gradientX = (input.screenPos.x + 1.0) * 0.5; // Map from [-1,1] to [0,1]
                    float3 yellowColor = float3(1.0, 1.0, 0.0);  // Bright yellow
                    float3 blueColor = float3(0.0, 0.3, 1.0);    // Deep blue
                    
                    // Create the yellow-to-blue base gradient
                    float3 baseGradient = lerp(yellowColor, blueColor, gradientX);
                    
                    // Enhanced rainbow color calculation
                    float normalizedX = (input.screenPos.x + 1.0) * 0.5;
                    
                    // Rainbow cycles
                    float hue = fmod(normalizedX * 3.0, 1.0);
                    
                    // Vivid rainbow colors
                    float saturation = 1.0;
                    float brightness = 1.0;
                    
                    float3 rainbowColor = hsv2rgb(float3(hue, saturation, brightness));
                    
                    // Boost rainbow intensity
                    rainbowColor = saturate(rainbowColor * 1.2);
                    
                    // Calculate smooth transition factor based on X position
                    float transitionFactor = (input.worldPos.x + 1.0) * 0.5;
                    transitionFactor = clamp(transitionFactor, 0.0, 1.0);
                    
                    // Apply smooth transition curve
                    transitionFactor = transitionFactor * transitionFactor * (3.0 - 2.0 * transitionFactor); // smoothstep
                    
                    // Smooth crossfade from yellow-blue gradient to rainbow
                    float3 finalColor = lerp(baseGradient, rainbowColor, transitionFactor);
                    
                    return float4(finalColor, 1.0);
                }
            )";
        }
    };
}