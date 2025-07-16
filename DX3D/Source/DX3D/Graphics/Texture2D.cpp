#include <DX3D/Graphics/Texture2D.h>
#include <wincodec.h>
#include <filesystem>

#pragma comment(lib, "windowscodecs.lib")

using namespace dx3d;

Texture2D::Texture2D(const std::string& filePath, const GraphicsResourceDesc& desc)
    : GraphicsResource(desc), m_filePath(filePath), m_width(0), m_height(0)
{
    loadFromFile(filePath);
    createSamplerState();
}

Texture2D::~Texture2D()
{
}

void Texture2D::loadFromFile(const std::string& filePath)
{
    // Check if file exists
    if (!std::filesystem::exists(filePath))
    {
        DX3DLogError(("Texture file not found: " + filePath).c_str());
        // Create a 1x1 white texture as fallback
        m_width = 1;
        m_height = 1;

        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = 1;
        textureDesc.Height = 1;
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        uint32_t whitePixel = 0xFFFFFFFF;
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = &whitePixel;
        initData.SysMemPitch = 4;

        DX3DGraphicsLogErrorAndThrow(
            m_device.CreateTexture2D(&textureDesc, &initData, &m_texture),
            "Failed to create fallback texture"
        );

        DX3DGraphicsLogErrorAndThrow(
            m_device.CreateShaderResourceView(m_texture.Get(), nullptr, &m_shaderResourceView),
            "Failed to create fallback shader resource view"
        );
        return;
    }

    // Initialize COM
    HRESULT hr = CoInitialize(nullptr);

    // Create WIC factory
    Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory;
    hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&wicFactory)
    );

    if (FAILED(hr))
    {
        DX3DLogError("Failed to create WIC factory");
        return;
    }

    // Convert string to wide string for WIC
    std::wstring wideFilePath(filePath.begin(), filePath.end());

    // Create decoder
    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    hr = wicFactory->CreateDecoderFromFilename(
        wideFilePath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &decoder
    );

    if (FAILED(hr))
    {
        DX3DLogError(("Failed to create decoder for: " + filePath).c_str());
        return;
    }

    // Get first frame
    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr))
    {
        DX3DLogError("Failed to get frame from image");
        return;
    }

    // Convert to RGBA format
    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    hr = wicFactory->CreateFormatConverter(&converter);
    if (FAILED(hr))
    {
        DX3DLogError("Failed to create format converter");
        return;
    }

    hr = converter->Initialize(
        frame.Get(),
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0,
        WICBitmapPaletteTypeCustom
    );

    if (FAILED(hr))
    {
        DX3DLogError("Failed to initialize format converter");
        return;
    }

    // Get image dimensions
    converter->GetSize(&m_width, &m_height);

    // Calculate stride and total size
    UINT stride = m_width * 4; // 4 bytes per pixel (RGBA)
    UINT imageSize = stride * m_height;

    // Allocate buffer and copy pixel data
    std::vector<uint8_t> pixels(imageSize);
    hr = converter->CopyPixels(nullptr, stride, imageSize, pixels.data());
    if (FAILED(hr))
    {
        DX3DLogError("Failed to copy pixels");
        return;
    }

    // Create D3D11 texture
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = m_width;
    textureDesc.Height = m_height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = pixels.data();
    initData.SysMemPitch = stride;

    DX3DGraphicsLogErrorAndThrow(
        m_device.CreateTexture2D(&textureDesc, &initData, &m_texture),
        ("Failed to create texture for: " + filePath).c_str()
    );

    // Create shader resource view
    DX3DGraphicsLogErrorAndThrow(
        m_device.CreateShaderResourceView(m_texture.Get(), nullptr, &m_shaderResourceView),
        ("Failed to create shader resource view for: " + filePath).c_str()
    );

    DX3DLogInfo(("Texture loaded successfully: " + filePath).c_str());
    CoUninitialize();
}

void Texture2D::createSamplerState()
{
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.BorderColor[0] = 0.0f;
    samplerDesc.BorderColor[1] = 0.0f;
    samplerDesc.BorderColor[2] = 0.0f;
    samplerDesc.BorderColor[3] = 0.0f;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    DX3DGraphicsLogErrorAndThrow(
        m_device.CreateSamplerState(&samplerDesc, &m_samplerState),
        "Failed to create sampler state"
    );
}