#pragma once
#include <DX3D/Graphics/Shaders/Shaders.h>

namespace dx3d
{
    class ModelShader
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
                    float3 normal : NORMAL;
                    float2 texCoord : TEXCOORD;
                };

                struct VS_OUTPUT {
                    float4 position : SV_POSITION;
                    float4 color : COLOR;
                    float3 normal : NORMAL;
                    float2 texCoord : TEXCOORD0;
                    float3 worldPos : TEXCOORD1;
                };

                VS_OUTPUT main(VS_INPUT input) {
                    VS_OUTPUT output;

                    // Transform position to world space
                    float4 worldPosition = mul(float4(input.position, 1.0f), world);
                    output.worldPos = worldPosition.xyz;

                    // Transform to view space
                    float4 viewPosition = mul(worldPosition, view);

                    // Transform to projection space
                    output.position = mul(viewPosition, projection);

                    // Transform normal to world space
                    output.normal = normalize(mul(input.normal, (float3x3)world));

                    // Pass through color and texture coordinates
                    output.color = input.color;
                    output.texCoord = input.texCoord;

                    return output;
                }
            )";
        }

        static const char* GetPixelShaderCode()
        {
            return R"(
                struct PS_INPUT {
                    float4 position : SV_POSITION;
                    float4 color : COLOR;
                    float3 normal : NORMAL;
                    float2 texCoord : TEXCOORD0;
                    float3 worldPos : TEXCOORD1;
                };

                cbuffer MaterialBuffer : register(b1)
                {
                    float4 diffuseColor;
                    float4 ambientColor;
                    float4 specularColor;
                    float4 emissiveColor;
                    float specularPower;
                    float opacity;
                    bool hasTexture;
                    float padding;
                };

                Texture2D diffuseTexture : register(t0);
                SamplerState textureSampler : register(s0);

                float4 main(PS_INPUT input) : SV_TARGET {
                    float4 finalColor;

                    if (hasTexture) {
                        // Sample texture and combine with material color
                        float4 textureColor = diffuseTexture.Sample(textureSampler, input.texCoord);
                        finalColor = textureColor * diffuseColor;
                    } else {
                        // Use material color only
                        finalColor = diffuseColor;
                    }

                    // Apply vertex color if needed
                    finalColor *= input.color;

                    // Apply opacity
                    finalColor.a *= opacity;

                    return finalColor;
                }
            )";
        }
    };

    // Material constant buffer structure for models
    struct ModelMaterialConstants
    {
        Vector4 diffuseColor;
        Vector4 ambientColor;
        Vector4 specularColor;
        Vector4 emissiveColor;
        float specularPower;
        float opacity;
        bool hasTexture;
        float padding;
    };
}