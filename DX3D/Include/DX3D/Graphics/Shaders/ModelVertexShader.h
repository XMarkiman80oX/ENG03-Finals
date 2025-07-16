#pragma once
#include <DX3D/Graphics/Shaders/Shaders.h>
#include <DX3D/Graphics/Shaders/ModelShader.h>
#include <d3d11.h>
#include <d3dcompiler.h>

namespace dx3d
{
    // Specialized function to create model vertex shader with extended input layout
    std::shared_ptr<VertexShader> createModelVertexShader(const GraphicsResourceDesc& desc)
    {
        // First compile the shader
        Microsoft::WRL::ComPtr<ID3DBlob> blob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

        HRESULT hr = D3DCompile(
            ModelShader::GetVertexShaderCode(),
            strlen(ModelShader::GetVertexShaderCode()),
            nullptr,
            nullptr,
            nullptr,
            "main",
            "vs_5_0",
            D3DCOMPILE_DEBUG,
            0,
            &blob,
            &errorBlob
        );

        if (FAILED(hr)) {
            if (errorBlob) {
                throw std::runtime_error(static_cast<const char*>(errorBlob->GetBufferPointer()));
            }
            throw std::runtime_error("Failed to compile model vertex shader");
        }

        // Create vertex shader object
        Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
        hr = desc.device.CreateVertexShader(
            blob->GetBufferPointer(),
            blob->GetBufferSize(),
            nullptr,
            &vertexShader
        );

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create model vertex shader");
        }

        // Define input layout for extended vertex format
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        // Create input layout
        Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
        hr = desc.device.CreateInputLayout(
            layout,
            4, // 4 elements in the layout
            blob->GetBufferPointer(),
            blob->GetBufferSize(),
            &inputLayout
        );

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create model input layout");
        }

        // Create a wrapper that holds the compiled shader and input layout
        class ModelVertexShaderWrapper : public VertexShader
        {
        public:
            ModelVertexShaderWrapper(const GraphicsResourceDesc& desc,
                Microsoft::WRL::ComPtr<ID3D11VertexShader> shader,
                Microsoft::WRL::ComPtr<ID3D11InputLayout> layout,
                Microsoft::WRL::ComPtr<ID3DBlob> blob)
                : VertexShader(desc, nullptr) // Skip base initialization
            {
                m_customShader = shader;
                m_customInputLayout = layout;
                m_blob = blob;
            }

            ID3D11VertexShader* getShader() const override { return m_customShader.Get(); }
            ID3D11InputLayout* getInputLayout() const override { return m_customInputLayout.Get(); }

        private:
            Microsoft::WRL::ComPtr<ID3D11VertexShader> m_customShader;
            Microsoft::WRL::ComPtr<ID3D11InputLayout> m_customInputLayout;
        };

        return std::make_shared<ModelVertexShaderWrapper>(desc, vertexShader, inputLayout, blob);
    }
}