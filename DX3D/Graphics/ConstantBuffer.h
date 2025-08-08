#pragma once
#include <../Graphics/GraphicsResource.h>

namespace dx3d
{
    class DeviceContext;

    class ConstantBuffer final : public GraphicsResource
    {
    public:
        ConstantBuffer(ui32 bufferSize, const GraphicsResourceDesc& desc);
        ~ConstantBuffer();

        void update(DeviceContext& deviceContext, const void* data);
        ID3D11Buffer* getBuffer() const noexcept { return m_buffer.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_buffer{};
        ui32 m_bufferSize{};
    };
}