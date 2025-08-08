#include <../Graphics/Shaders/Shaders.h>
#include <../Graphics/Shaders/ParticleShader.h>
#include <../Particles/ParticleEmitter.h> // For ParticleInstanceData
#include <d3d11.h>
#include <d3dcompiler.h>
#include <cstddef> // For offsetof

namespace dx3d
{
    // Specialized function to create particle vertex shader with proper input layout
    std::shared_ptr<VertexShader> createParticleVertexShader(const GraphicsResourceDesc& desc)
    {
        // First compile the shader
        Microsoft::WRL::ComPtr<ID3DBlob> blob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

        HRESULT hr = D3DCompile(
            ParticleShader::GetVertexShaderCode(),
            strlen(ParticleShader::GetVertexShaderCode()),
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
            throw std::runtime_error("Failed to compile particle vertex shader");
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
            throw std::runtime_error("Failed to create particle vertex shader");
        }

        // Define input layout for particles
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            // Per-vertex data
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },

            // Per-instance data
            { "POSITION", 1, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "POSITION", 2, DXGI_FORMAT_R32_FLOAT, 1, 12, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "COLOR", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "POSITION", 3, DXGI_FORMAT_R32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 }
        };

        // Create input layout
        Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
        hr = desc.device.CreateInputLayout(
            layout,
            6, // 6 elements in the layout
            blob->GetBufferPointer(),
            blob->GetBufferSize(),
            &inputLayout
        );

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create particle input layout");
        }

        // Create a wrapper that holds the compiled shader and input layout
        class ParticleVertexShaderWrapper : public VertexShader
        {
        public:
            ParticleVertexShaderWrapper(const GraphicsResourceDesc& desc,
                Microsoft::WRL::ComPtr<ID3D11VertexShader> shader,
                Microsoft::WRL::ComPtr<ID3D11InputLayout> layout,
                Microsoft::WRL::ComPtr<ID3DBlob> blob)
                : VertexShader(desc, nullptr) // Skip base initialization
            {
                // Directly set the members (we need to make them protected in Shaders.h)
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

        return std::make_shared<ParticleVertexShaderWrapper>(desc, vertexShader, inputLayout, blob);
    }
}