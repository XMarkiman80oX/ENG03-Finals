#include <DX3D/Graphics/Primitives/AGameObject.h>
#include <DirectXMath.h>

using namespace dx3d;
using namespace DirectX;

AGameObject::AGameObject()
{
}

AGameObject::AGameObject(const Vector3& position, const Vector3& rotation, const Vector3& scale)
{
    m_transform.position = position;
    m_transform.rotation = rotation;
    m_transform.scale = scale;
}

void AGameObject::rotate(const Vector3& deltaRotation)
{
    m_transform.rotation += deltaRotation;
}

void AGameObject::translate(const Vector3& deltaPosition)
{
    m_transform.position += deltaPosition;
}

Matrix4x4 AGameObject::Transform::getWorldMatrix() const
{
    // Create individual transformation matrices
    Matrix4x4 scaleMatrix = Matrix4x4::CreateScale(scale);
    Matrix4x4 rotationX = Matrix4x4::CreateRotationX(rotation.x);
    Matrix4x4 rotationY = Matrix4x4::CreateRotationY(rotation.y);
    Matrix4x4 rotationZ = Matrix4x4::CreateRotationZ(rotation.z);
    Matrix4x4 translationMatrix = Matrix4x4::CreateTranslation(position);

    // Combine transformations: Scale -> Rotate -> Translate
    Matrix4x4 result = scaleMatrix * rotationZ * rotationY * rotationX * translationMatrix;
    return result;
}