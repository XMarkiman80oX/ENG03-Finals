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
                    float3 basePosition : POSITION;
                    float4 baseColor    : COLOR; 
                    
                    float3 instancePosition : POSITION1;
                    float  instanceSize     : POSITION2;
                    float4 instanceColor    : COLOR1;
                    float  instanceRotation : POSITION3;
                };

                struct VS_OUTPUT
                {
                    float4 clipPosition : SV_POSITION;
                    float2 texCoord     : TEXCOORD0;
                    float4 color        : COLOR;
                };

                VS_OUTPUT main(VS_INPUT input)
                {
                    VS_OUTPUT output;
                    
                    float3 centerWorldPos = input.instancePosition;

                    float cosRot = cos(input.instanceRotation);
                    float sinRot = sin(input.instanceRotation);
                    
                    float2 rotatedPos;
                    rotatedPos.x = input.basePosition.x * cosRot - input.basePosition.y * sinRot;
                    rotatedPos.y = input.basePosition.x * sinRot + input.basePosition.y * cosRot;

                    float3 finalWorldPos = centerWorldPos 
                                         + cameraRight * rotatedPos.x * input.instanceSize
                                         + cameraUp * rotatedPos.y * input.instanceSize;
                    
                           
                    float4 finalPosition = float4(finalWorldPos, 1.0f);
                    finalPosition = mul(finalPosition, view);
                    finalPosition = mul(finalPosition, projection);

                    output.clipPosition = finalPosition;

                    output.color = input.instanceColor;
                    output.texCoord = input.baseColor.xy; 
                    
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
                    // Simple circular particle shape using texture coordinates.
                    float2 centerDist = input.texCoord - float2(0.5, 0.5);
                    float dist = length(centerDist);
                    
                    // Create a soft circular falloff.
                    float alpha = 1.0 - saturate(dist * 2.0);
                    alpha = smoothstep(0.0, 1.0, alpha);
                    
                    // Apply the particle's color with the calculated alpha.
                    float4 finalColor = input.color;
                    finalColor.a *= alpha;
                    
                    // Discard pixels that are fully transparent for performance.
                    if (finalColor.a < 0.01)
                        discard;
                    
                    return finalColor;
                }
            )";
        }
    };
}
