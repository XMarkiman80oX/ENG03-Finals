#pragma once
#include <DX3D/Graphics/GraphicsResource.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include <string>

namespace dx3d
{
    class Shader : public GraphicsResource
    {
    public:
        Shader(const GraphicsResourceDesc& desc);
        virtual ~Shader();

    protected:
        Microsoft::WRL::ComPtr<ID3DBlob> compileShader(
            const char* shaderCode,
            const char* entryPoint,
            const char* shaderModel);

    protected:
        Microsoft::WRL::ComPtr<ID3DBlob> m_blob{};
    };

    class VertexShader : public Shader
    {
    public:
        VertexShader(const GraphicsResourceDesc& desc, const char* shaderCode);
        VertexShader(const GraphicsResourceDesc& desc, const char* shaderCode, D3D11_INPUT_ELEMENT_DESC* layout, ui32 layoutSize);
        ~VertexShader();

        virtual ID3D11VertexShader* getShader() const;
        virtual ID3D11InputLayout* getInputLayout() const;

    private:
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_shader{};
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout{};
    };

    class PixelShader : public Shader
    {
    public:
        PixelShader(const GraphicsResourceDesc& desc, const char* shaderCode);
        ~PixelShader();

        ID3D11PixelShader* getShader() const;

    private:
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_shader{};
    };
}