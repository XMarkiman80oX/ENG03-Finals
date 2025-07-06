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
                cbuffer TransformBuffer : register(b0)
                {
                    matrix world;
                    matrix view;
                    matrix projection;
                };

                struct VS_INPUT {
                    float3 position : POSITION;
                    float2 texcoord : TEXCOORD;
                };
                
                struct VS_OUTPUT {
                    float4 position : SV_POSITION;
                    float2 texcoord : TEXCOORD;
                };
                
                VS_OUTPUT main(VS_INPUT input) {
                    VS_OUTPUT output;
                    
                    // Transform the position from object space to world space, then to view, and finally to projection space.
                    output.position = mul(float4(input.position, 1.0f), world);
                    output.position = mul(output.position, view);
                    output.position = mul(output.position, projection);
                    
                    // Pass the texture coordinates to the pixel shader.
                    output.texcoord = input.texcoord;
                    
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