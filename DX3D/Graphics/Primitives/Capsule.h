#pragma once
#include <../Graphics/Primitives/AGameObject.h>
#include <../Graphics/VertexBuffer.h>
#include <../Graphics/IndexBuffer.h>
#include <../Math/Math.h>
#include <memory>

namespace dx3d
{
    class Capsule : public BaseGameObject
    {
    public:
        // Static methods for creating rendering resources
        static std::shared_ptr<VertexBuffer> CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc,
            ui32 segments = 16, ui32 rings = 8);
        static std::shared_ptr<IndexBuffer> CreateIndexBuffer(const GraphicsResourceDesc& resourceDesc,
            ui32 segments = 16, ui32 rings = 8);

        // Get the number of indices for a capsule
        static ui32 GetIndexCount(ui32 segments = 16, ui32 rings = 8)
        {
            // Two hemispheres + cylinder body
            return segments * rings * 12 + segments * 6;
        }

        // Constructors
        Capsule();
        Capsule(const Vector3& position, const Vector3& rotation = Vector3(0, 0, 0), const Vector3& scale = Vector3(1, 1, 1));
        virtual ~Capsule() = default;

        // Override virtual methods from base class if needed
        virtual void update(float deltaTime) override;

    protected:
        virtual CollisionShapeType getCollisionShapeType() const override
        {
            return CollisionShapeType::Capsule;
        }
    };
}