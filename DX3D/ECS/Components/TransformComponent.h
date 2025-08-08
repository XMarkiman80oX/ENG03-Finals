#pragma once
#include "../Math/Math.h"

namespace dx3d
{
    struct TransformComponent
    {
        Vector3 position{ 0.0f, 0.0f, 0.0f };
        Vector3 rotation{ 0.0f, 0.0f, 0.0f }; // Euler angles in radians
        Vector3 scale{ 1.0f, 1.0f, 1.0f };

        Matrix4x4 getWorldMatrix() const
        {
            Matrix4x4 scaleMatrix = Matrix4x4::CreateScale(scale);
            Matrix4x4 rotationX = Matrix4x4::CreateRotationX(rotation.x);
            Matrix4x4 rotationY = Matrix4x4::CreateRotationY(rotation.y);
            Matrix4x4 rotationZ = Matrix4x4::CreateRotationZ(rotation.z);
            Matrix4x4 translationMatrix = Matrix4x4::CreateTranslation(position);

            return scaleMatrix * rotationZ * rotationY * rotationX * translationMatrix;
        }
    };
}
