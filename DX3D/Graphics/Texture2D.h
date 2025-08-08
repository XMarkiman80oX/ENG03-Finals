#pragma once
#include <../Graphics/GraphicsResource.h>
#include <string>

namespace dx3d
{
    class Texture2D final : public GraphicsResource
    {
    public:
        Texture2D(const std::string& filePath, const GraphicsResourceDesc& desc);
        ~Texture2D();

        ID3D11ShaderResourceView* getShaderResourceView() const { return m_shaderResourceView.Get(); }
        ID3D11SamplerState* getSamplerState() const { return m_samplerState.Get(); }

        ui32 getWidth() const { return m_width; }
        ui32 getHeight() const { return m_height; }

        const std::string& getFilePath() const { return m_filePath; }

    private:
        void loadFromFile(const std::string& filePath);
        void createSamplerState();

    private:
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_texture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shaderResourceView;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState;

        std::string m_filePath;
        ui32 m_width;
        ui32 m_height;
    };
}