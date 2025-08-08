#pragma once
#include <../Graphics/Primitives/AGameObject.h>
#include <../Graphics/VertexBuffer.h>
#include <../Graphics/IndexBuffer.h>
#include <../Math/Math.h>
#include <memory>

namespace dx3d
{
    class Sphere : public BaseGameObject
    {
    public:
        static std::shared_ptr<VertexBuffer> CreateVertexBuffer(const GraphicsResourceDesc& resourceDesc,
            ui32 latitudeSegments = 16, ui32 longitudeSegments = 32);
        static std::shared_ptr<IndexBuffer> CreateIndexBuffer(const GraphicsResourceDesc& resourceDesc,
            ui32 latitudeSegments = 16, ui32 longitudeSegments = 32);

        static ui32 GetIndexCount(ui32 latitudeSegments = 16, ui32 longitudeSegments = 32)
        {
            return latitudeSegments * longitudeSegments * 6;
        }

        Sphere();
        Sphere(const Vector3& position, const Vector3& rotation = Vector3(0, 0, 0), const Vector3& scale = Vector3(1, 1, 1));
        virtual ~Sphere() = default;

        virtual void update(float deltaTime) override;

    protected:
        virtual CollisionShapeType getCollisionShapeType() const override
        {
            return CollisionShapeType::Sphere;
        }
    };
}