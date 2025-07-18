#pragma once
#include <DX3D/Graphics/Primitives/AGameObject.h>
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Graphics/IndexBuffer.h>
#include <DX3D/Math/Math.h>
#include <memory>

namespace dx3d
{
    class Plane : public AGameObject
    {
    public:
        // Static methods for creating rendering resources
        static std::shared_ptr<VertexBuffer> CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc);
        static std::shared_ptr<IndexBuffer> CreateIndexBuffer(const GraphicsResourceDesc& resourceDesc);

        // Static method to create a plane with custom dimensions
        static std::shared_ptr<VertexBuffer> CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc, float width, float height);

        // Static method to create a white plane (all vertices white color)
        static std::shared_ptr<VertexBuffer> CreateWhiteVertexBuffer(const GraphicsResourceDesc& resourceDesc, float width, float height);

        // Get the number of indices for a plane (2 triangles = 6 indices)
        static ui32 GetIndexCount() { return 6; }

        // Constructors
        Plane();
        Plane(const Vector3& position, const Vector3& rotation = Vector3(0, 0, 0), const Vector3& scale = Vector3(1, 1, 1));
        virtual ~Plane() = default;

        // Override virtual methods from base class if needed
        virtual void update(float deltaTime) override;

    protected:
        virtual CollisionShapeType getCollisionShapeType() const override
        {
            return CollisionShapeType::Plane;
        }
    };
}
