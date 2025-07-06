#pragma once
#include <DX3D/Graphics/Primitives/AGameObject.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Graphics/IndexBuffer.h>
#include <memory>

namespace dx3d
{
    class CameraIcon : public AGameObject
    {
    public:
        CameraIcon(const Vector3& position = {}, const Vector3& rotation = {}, const Vector3& scale = { 1,1,1 });
        static std::shared_ptr<VertexBuffer> CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc);
        static std::shared_ptr<IndexBuffer> CreateIndexBuffer(const GraphicsResourceDesc& resourceDesc);
        static ui32 GetIndexCount() { return 6; }
    };
}