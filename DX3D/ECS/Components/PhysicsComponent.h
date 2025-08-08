#pragma once
#include "../Math/Math.h"
#include <reactphysics3d/reactphysics3d.h>
#include <memory>

namespace dx3d
{
    enum class PhysicsBodyType
    {
        Static,    
        Kinematic, 
        Dynamic   
    };

    enum class CollisionShapeType
    {
        Box,
        Sphere,
        Cylinder,
        Capsule,
        Plane
    };

    struct PhysicsComponent
    {
        rp3d::RigidBody* rigidBody = nullptr;
        rp3d::Collider* collider = nullptr;

        PhysicsBodyType bodyType = PhysicsBodyType::Dynamic;
        CollisionShapeType shapeType = CollisionShapeType::Box;

        // Shape parameters
        Vector3 boxHalfExtents{ 0.5f, 0.5f, 0.5f };
        float sphereRadius = 0.5f;
        float cylinderRadius = 0.5f;
        float cylinderHeight = 1.0f;
        float capsuleRadius = 0.5f;
        float capsuleHeight = 1.0f;

        // Physics properties
        float mass = 1.0f;
        float restitution = 0.3f;  // Bounciness (0-1)
        float friction = 0.5f;     // Surface friction (0-1)

        bool isInitialized = false;
    };
}