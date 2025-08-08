#pragma once
#include <../Graphics/Primitives/AGameObject.h>
#include <../Graphics/VertexBuffer.h>
#include <../Graphics/IndexBuffer.h>
#include <../Math/Math.h>
#include <memory>

namespace dx3d
{
    class Cylinder : public BaseGameObject
    {
    public:
        // Static methods for creating rendering resources
        static std::shared_ptr<VertexBuffer> CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc,
            ui32 segments = 32);
        static std::shared_ptr<IndexBuffer> CreateIndexBuffer(const GraphicsResourceDesc& resourceDesc,
            ui32 segments = 32);

        // Get the number of indices for a cylinder
        static ui32 GetIndexCount(ui32 segments = 32)
        {
            return segments * 12; // Top + bottom + sides
        }

        // Constructors
        Cylinder();
        Cylinder(const Vector3& position, const Vector3& rotation = Vector3(0, 0, 0), const Vector3& scale = Vector3(1, 1, 1));
        virtual ~Cylinder() = default;

        // Override virtual methods from base class if needed
        virtual void update(float deltaTime) override;

    protected:
        virtual CollisionShapeType getCollisionShapeType() const override
        {
            return CollisionShapeType::Cylinder;
        }
    };
}