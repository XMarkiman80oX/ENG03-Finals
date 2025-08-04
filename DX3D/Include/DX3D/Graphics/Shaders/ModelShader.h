#pragma once
#include <DX3D/Graphics/Shaders/Shaders.h>
#include <DX3D/Math/Math.h>


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
                #define LIGHT_TYPE_DIRECTIONAL 0
                #define LIGHT_TYPE_POINT 1
                #define LIGHT_TYPE_SPOT 2

                struct Light
                {
                    float3 position;
                    int    type;
                    float3 direction;
                    float  intensity;
                    float3 color;
                    float  radius;
                    float  spot_angle_inner;
                    float  spot_angle_outer;
                    float  spot_falloff;
                    float  padding;
                };

                cbuffer SceneBuffer : register(b2)
                {
                    float4 camera_position;
                    float4 ambient_color;
                    uint   num_lights;
                    int    shadow_casting_light_index;
                    float2 padding;
                    Light  lights[16];
                    matrix light_view;
                    matrix light_projection;
                };
        
                cbuffer MaterialBuffer : register(b1)
                {
                    float4 diffuseColor;
                    float4 ambientColor;
                    float4 specularColor;
                    float4 emissiveColor;
                    float  specularPower;
                    float  opacity;
                    float  hasTexture;
                    float  padding_mat;
                };

                Texture2D shadowMap : register(t1);
                SamplerComparisonState shadowSampler : register(s1);
                Texture2D diffuseTexture : register(t0);
                SamplerState textureSampler : register(s0);

                struct PS_INPUT {
                    float4 position     : SV_POSITION;
                    float4 color        : COLOR;
                    float3 normal       : NORMAL;
                    float2 texCoord     : TEXCOORD0;
                    float3 worldPos     : TEXCOORD1;
                };

                float3 calculateLight(Light light, float3 pixel_world_pos, float3 normal, float3 view_dir, float shadow_factor)
                {
                    float3 light_dir;
                    float attenuation = 1.0f;
            
                    if (light.type == LIGHT_TYPE_DIRECTIONAL)
                    {
                        light_dir = normalize(-light.direction);
                    }
                    else
                    {
                        float3 distance_vec = light.position - pixel_world_pos;
                        float dist = length(distance_vec);
                        light_dir = normalize(distance_vec);
                        attenuation = saturate(1.0f - (dist / light.radius));
                        attenuation *= attenuation;
                    }

                    if (light.type == LIGHT_TYPE_SPOT)
                    {
                        float3 direction_to_pixel = normalize(pixel_world_pos - light.position);
                        float spot_factor = dot(-direction_to_pixel, normalize(light.direction));
                        float spot_effect = smoothstep(cos(light.spot_angle_outer), cos(light.spot_angle_inner), spot_factor);
                        attenuation *= spot_effect;
                    }

                    float diffuse_factor = max(dot(normal, light_dir), 0.0f);
                    float3 diffuse = diffuse_factor * light.color * light.intensity * diffuseColor.rgb;

                    float3 halfway_vec = normalize(light_dir + view_dir);
                    float specular_factor = pow(max(dot(normal, halfway_vec), 0.0f), specularPower);
                    float3 specular = specular_factor * light.color * light.intensity * specularColor.rgb;
            
                    return (diffuse + specular) * attenuation * shadow_factor;
                }

                float4 main(PS_INPUT input) : SV_TARGET 
                {
                    // --- Shadow Calculation (same as before) ---
                    float4 light_clip_pos = mul(float4(input.worldPos, 1.0f), light_view);
                    light_clip_pos = mul(light_clip_pos, light_projection);
                    float2 shadow_tex_coord = 0.5f * light_clip_pos.xy / light_clip_pos.w + 0.5f;
                    shadow_tex_coord.y = 1.0f - shadow_tex_coord.y;
                    float depth_from_light = light_clip_pos.z / light_clip_pos.w;
                    float shadow_value = shadowMap.SampleCmp(shadowSampler, shadow_tex_coord, depth_from_light - 0.0005f);

                    float3 normal = normalize(input.normal);
                    float3 view_dir = normalize(camera_position.xyz - input.worldPos);
            
                    float4 finalColor = ambientColor * ambient_color;

                    for (uint i = 0; i < num_lights; i++)
                    {
                        float shadow_factor_for_this_light = (i == shadow_casting_light_index) ? shadow_value : 1.0f;
                        finalColor.rgb += calculateLight(lights[i], input.worldPos, normal, view_dir, shadow_factor_for_this_light);
                    }

                    finalColor.rgb += emissiveColor.rgb;

                    if (hasTexture > 0.5f) {
                        finalColor.rgb *= diffuseTexture.Sample(textureSampler, input.texCoord).rgb;
                    }

                    finalColor.a = diffuseColor.a * opacity;
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
        float hasTexture;
        float padding;
    };
}