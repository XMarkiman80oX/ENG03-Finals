#pragma once
#include <../Graphics/Primitives/AGameObject.h>
#include <../Graphics/Light.h>

namespace dx3d
{
    class LightObject : public BaseGameObject
    {
    public:
        LightObject() = default;

        Light& getLightData() { return m_lightData; }
        const Light& getLightData() const { return m_lightData; }

        virtual void update(float deltaTime) override;

    protected:
        virtual CollisionShapeType getCollisionShapeType() const override { return CollisionShapeType::Box; }
        Light m_lightData;
    };

    class DirectionalLight : public LightObject
    {
    public:
        DirectionalLight();
    };

    class PointLight : public LightObject
    {
    public:
        PointLight();
    };

    class SpotLight : public LightObject
    {
    public:
        SpotLight();
    };
}