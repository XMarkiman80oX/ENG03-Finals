#pragma once
#include <DX3D/Graphics/Shaders/Shaders.h>

namespace dx3d
{
    class ParticleShader
    {
    public:
        static const char* GetVertexShaderCode()
        {
            return R"(
                cbuffer ViewProjBuffer : register(b0)
                {
                    matrix view;
                    matrix projection;
                    float3 cameraRight;
                    float _pad0;
                    float3 cameraUp;
                    float _pad1;
                };

                struct VS_INPUT
                {
                    // Per-vertex data (position and color/UV packed)
                    float3 position : POSITION;
                    float4 color : COLOR; // Using first 2 components for UV
                    
                    // Per-instance data
                    float3 instancePosition : POSITION1;
                    float instanceSize : POSITION2;
                    float4 instanceColor : COLOR1;
                    float instanceRotation : POSITION3;
                };

                struct VS_OUTPUT
                {
                    float4 position : SV_POSITION;
                    float2 texCoord : TEXCOORD0;
                    float4 color : COLOR;
                };

                VS_OUTPUT main(VS_INPUT input)
                {
                    VS_OUTPUT output;
                    
                    // Extract UV from color field
                    float2 texCoord = input.color.xy;
                    
                    // Create rotation matrix for billboard rotation
                    float cosRot = cos(input.instanceRotation);
                    float sinRot = sin(input.instanceRotation);
                    float2x2 rotMatrix = float2x2(cosRot, -sinRot, sinRot, cosRot);
                    
                    // Rotate the vertex position
                    float2 rotatedPos = mul(rotMatrix, input.position.xy);
                    
                    // Billboard calculation - make quad face camera
                    float3 worldPos = input.instancePosition;
                    worldPos += cameraRight * rotatedPos.x * input.instanceSize;
                    worldPos += cameraUp * rotatedPos.y * input.instanceSize;
                    
                    // Transform to clip space
                    float4 viewPos = mul(float4(worldPos, 1.0f), view);
                    output.position = mul(viewPos, projection);
                    
                    // Pass through texture coordinates and color
                    output.texCoord = texCoord;
                    output.color = input.instanceColor;
                    
                    return output;
                }
            )";
        }

        static const char* GetPixelShaderCode()
        {
            return R"(
                struct PS_INPUT
                {
                    float4 position : SV_POSITION;
                    float2 texCoord : TEXCOORD0;
                    float4 color : COLOR;
                };

                float4 main(PS_INPUT input) : SV_TARGET
                {
                    // Simple circular particle shape
                    float2 centerDist = input.texCoord - float2(0.5, 0.5);
                    float dist = length(centerDist);
                    
                    // Soft circular falloff
                    float alpha = saturate(1.0 - dist * 2.0);
                    alpha = smoothstep(0.0, 1.0, alpha);
                    
                    // Apply color with alpha
                    float4 finalColor = input.color;
                    finalColor.a *= alpha;
                    
                    // Discard fully transparent pixels for better performance
                    if (finalColor.a < 0.01)
                        discard;
                    
                    return finalColor;
                }
            )";
        }
    };
}