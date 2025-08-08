#pragma once
#include <../Graphics/Shaders/Shaders.h>

namespace dx3d
{
    class DepthShader
    {
    public:
        static const char* GetVertexShaderCode()
        {
            return R"(
                cbuffer LightTransformBuffer : register(b0)
                {
                    matrix light_view;
                    matrix light_projection;
                    matrix world;
                };

                struct VS_INPUT {
                    float3 position : POSITION;
                };

                float4 main(VS_INPUT input) : SV_POSITION
                {
                    float4 world_pos = mul(float4(input.position, 1.0f), world);
                    float4 view_pos = mul(world_pos, light_view);
                    float4 clip_pos = mul(view_pos, light_projection);
                    return clip_pos;
                }
            )";
        }
    };
}