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
                    // Per-vertex data (the corners of our quad)
                    float3 basePosition : POSITION;
                    float4 baseColor    : COLOR; // Using .xy for UVs
                    
                    // Per-instance data (unique to each particle)
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
                    
                    // --- Billboarding Logic ---
                    // This creates a quad in world space that always faces the camera.
                    
                    // Start with the particle's center position in the world.
                    float3 centerWorldPos = input.instancePosition;

                    // Calculate the vertex's offset from the center, scaled and rotated.
                    float cosRot = cos(input.instanceRotation);
                    float sinRot = sin(input.instanceRotation);
                    
                    // Rotate the quad corner's local position (which is just a corner of a 2D quad).
                    float2 rotatedPos;
                    rotatedPos.x = input.basePosition.x * cosRot - input.basePosition.y * sinRot;
                    rotatedPos.y = input.basePosition.x * sinRot + input.basePosition.y * cosRot;

                    // Apply the billboard orientation using camera vectors and add to the particle's world position.
                    float3 finalWorldPos = centerWorldPos 
                                         + cameraRight * rotatedPos.x * input.instanceSize
                                         + cameraUp * rotatedPos.y * input.instanceSize;
                    
                    // --- Transformation to Clip Space ---
                    // Combine the transformations in the correct order: World -> View -> Projection.
                    // HLSL's mul() intrinsic handles this naturally when chained.
                    // We treat the billboarding result as our "world" matrix transformation.
                    
                    float4 finalPosition = float4(finalWorldPos, 1.0f);
                    finalPosition = mul(finalPosition, view);
                    finalPosition = mul(finalPosition, projection);

                    output.clipPosition = finalPosition;

                    // Pass color and texture coordinates to the pixel shader.
                    output.color = input.instanceColor;
                    output.texCoord = input.baseColor.xy; // Pass the UV coordinates.
                    
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
