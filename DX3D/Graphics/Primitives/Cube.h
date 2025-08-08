#pragma once
#include <../Graphics/Primitives/AGameObject.h>
#include <../Graphics/VertexBuffer.h>
#include <../Graphics/IndexBuffer.h>
#include <../Math/Math.h>
#include <memory>

namespace dx3d
{
    class Cube : public AGameObject
    {
    public:
        // Static methods for creating rendering resources
        static std::shared_ptr<VertexBuffer> CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc);
        static std::shared_ptr<IndexBuffer> CreateIndexBuffer(const GraphicsResourceDesc& resourceDesc);

        // Get the number of indices for a cube
        static ui32 GetIndexCount() { return 36; } // 6 faces * 2 triangles * 3 vertices

        // Constructors
        Cube();
        Cube(const Vector3& position, const Vector3& rotation = Vector3(0, 0, 0), const Vector3& scale = Vector3(1, 1, 1));
        virtual ~Cube() = default;

        // Override virtual methods from base class if needed
        virtual void update(float deltaTime) override;

    protected:
        virtual CollisionShapeType getCollisionShapeType() const override
        {
            return CollisionShapeType::Box;
        }
    };
}