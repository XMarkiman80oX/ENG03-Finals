#pragma once
#include <DX3D/Graphics/Shaders/Shaders.h>

namespace dx3d
{
    class TextureShader
    {
    public:
        static const char* GetVertexShaderCode()
        {
            return R"(
                cbuffer TransformBuffer : register(b0) { matrix world; matrix view; matrix projection; };

                struct VS_INPUT {
                    float3 position : POSITION;
                    float4 color    : COLOR; // Read texture coords from the color semantic
                };
                
                struct VS_OUTPUT { float4 position : SV_POSITION; float2 texcoord : TEXCOORD; };
                
                VS_OUTPUT main(VS_INPUT input) {
                    VS_OUTPUT output;
                    output.position = mul(float4(input.position, 1.0f), world);
                    output.position = mul(output.position, view);
                    output.position = mul(output.position, projection);
                    output.texcoord = input.color.xy; // Pass the U,V coords to the pixel shader
                    return output;
                }
            )";
        }

        static const char* GetPixelShaderCode()
        {
            return R"(
                Texture2D ObjTexture;
                SamplerState ObjSampler;

                struct PS_INPUT {
                    float4 position : SV_POSITION;
                    float2 texcoord : TEXCOORD;
                };
                
                float4 main(PS_INPUT input) : SV_TARGET {
                    // Sample the texture at the given texture coordinates.
                    return ObjTexture.Sample(ObjSampler, input.texcoord);
                }
            )";
        }
    };
}