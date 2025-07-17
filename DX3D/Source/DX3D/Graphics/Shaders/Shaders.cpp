#pragma comment(lib, "d3dcompiler.lib")
#include <DX3D/Graphics/Shaders/Shaders.h>

dx3d::Shader::Shader(const GraphicsResourceDesc& desc)
    : GraphicsResource(desc)
{
}

dx3d::Shader::~Shader()
{
}

Microsoft::WRL::ComPtr<ID3DBlob> dx3d::Shader::compileShader(
    const char* shaderCode,
    const char* entryPoint,
    const char* shaderModel)
{
    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3DCompile(
        shaderCode,
        strlen(shaderCode),
        nullptr,
        nullptr,
        nullptr,
        entryPoint,
        shaderModel,
        D3DCOMPILE_DEBUG,
        0,
        &blob,
        &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            DX3DLogError(static_cast<const char*>(errorBlob->GetBufferPointer()));
        }
        DX3DLogErrorAndThrow("Failed to compile shader");
    }

    return blob;
}


dx3d::VertexShader::VertexShader(const GraphicsResourceDesc& desc, const char* shaderCode)
    : Shader(desc)
{
    try {
        DX3DLogInfo("Compiling vertex shader...");
        m_blob = compileShader(shaderCode, "main", "vs_5_0");
        DX3DLogInfo("Vertex shader compiled successfully.");

        // Create the vertex shader
        DX3DGraphicsLogErrorAndThrow(
            m_device.CreateVertexShader(
                m_blob->GetBufferPointer(),
                m_blob->GetBufferSize(),
                nullptr,
                &m_shader
            ),
            "Failed to create vertex shader"
        );
        DX3DLogInfo("Vertex shader created successfully.");

        // Define input layout - UPDATED to include all 4 fields
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        // Create input layout - UPDATED element count to 4
        DX3DGraphicsLogErrorAndThrow(
            m_device.CreateInputLayout(
                layout,
                4,  // Changed from 2 to 4
                m_blob->GetBufferPointer(),
                m_blob->GetBufferSize(),
                &m_inputLayout
            ),
            "Failed to create input layout"
        );
        DX3DLogInfo("Input layout created successfully.");
    }
    catch (const std::exception& e) {
        DX3DLogError(e.what());
        throw;
    }
}

dx3d::VertexShader::~VertexShader()
{
}

ID3D11VertexShader* dx3d::VertexShader::getShader() const
{
    return m_shader.Get();
}

ID3D11InputLayout* dx3d::VertexShader::getInputLayout() const
{
    return m_inputLayout.Get();
}

// PixelShader implementation
dx3d::PixelShader::PixelShader(const GraphicsResourceDesc& desc, const char* shaderCode)
    : Shader(desc)
{
    m_blob = compileShader(shaderCode, "main", "ps_5_0");

    // Create the pixel shader
    DX3DGraphicsLogErrorAndThrow(
        m_device.CreatePixelShader(
            m_blob->GetBufferPointer(),
            m_blob->GetBufferSize(),
            nullptr,
            &m_shader
        ),
        "Failed to create pixel shader"
    );
}

dx3d::PixelShader::~PixelShader()
{
}

ID3D11PixelShader* dx3d::PixelShader::getShader() const
{
    return m_shader.Get();
}