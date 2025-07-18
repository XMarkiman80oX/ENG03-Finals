#pragma once
#include <DX3D/Core/Base.h>
#include <DX3D/ECS/Entity.h>
#include <DX3D/ECS/Components/PhysicsComponent.h>
#include <reactphysics3d/reactphysics3d.h>
#include <memory>

namespace dx3d
{
    class PhysicsSystem : public Base
    {
    public:
        static PhysicsSystem& getInstance()
        {
            static PhysicsSystem instance;
            return instance;
        }

        void initialize(const BaseDesc& desc);
        void shutdown();

        // Physics world management
        rp3d::PhysicsWorld* getPhysicsWorld() { return m_physicsWorld; }

        // Component management
        void addPhysicsComponent(EntityID entity, const PhysicsComponent& component);
        void removePhysicsComponent(EntityID entity);
        void updatePhysicsComponent(EntityID entity, const PhysicsComponent& component);

        // Shape creation helpers
        rp3d::CollisionShape* createCollisionShape(CollisionShapeType type, const PhysicsComponent& component);

        // Physics simulation
        void update(float deltaTime);
        void setFixedTimeStep(float timeStep) { m_fixedTimeStep = timeStep; }

        // Utility functions
        static rp3d::Vector3 toReactVector(const Vector3& vec);
        static Vector3 fromReactVector(const rp3d::Vector3& vec);
        static rp3d::Quaternion toReactQuaternion(const Vector3& eulerAngles);
        static Vector3 fromReactQuaternion(const rp3d::Quaternion& quat);

    private:
        PhysicsSystem() = default;
        ~PhysicsSystem() = default;
        PhysicsSystem(const PhysicsSystem&) = delete;
        PhysicsSystem& operator=(const PhysicsSystem&) = delete;

        void initializePhysicsBody(EntityID entity, PhysicsComponent& component);
        void syncTransformFromPhysics(EntityID entity, const PhysicsComponent& component);

    private:
        rp3d::PhysicsCommon m_physicsCommon;
        rp3d::PhysicsWorld* m_physicsWorld = nullptr;

        float m_fixedTimeStep = 1.0f / 60.0f; // 60 FPS
        float m_accumulator = 0.0f;

        bool m_initialized = false;
    };
}