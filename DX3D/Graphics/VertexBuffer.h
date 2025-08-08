#pragma once
#include <../Graphics/GraphicsResource.h>
#include <../Graphics/Vertex.h>

namespace dx3d
{
    class VertexBuffer final : public GraphicsResource
    {
    public:
        VertexBuffer(const void* vertexList, ui32 vertexSize, ui32 vertexCount, const GraphicsResourceDesc& gDesc);
        ui32 getVertexCount() const noexcept;
        ID3D11Buffer* getBuffer() const noexcept;
    private:
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_buffer{};
        ui32 m_vertexCount = 0;
    };
}