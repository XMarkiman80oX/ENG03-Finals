#include <DX3D/Math/Math.h>

using namespace dx3d;
using namespace DirectX;

Matrix4x4 Matrix4x4::operator*(const Matrix4x4& other) const
{
    XMMATRIX a = toXMMatrix();
    XMMATRIX b = other.toXMMatrix();
    XMMATRIX result = XMMatrixMultiply(a, b);
    return fromXMMatrix(result);
}

Matrix4x4 Matrix4x4::CreateTranslation(const Vector3& translation)
{
    XMMATRIX xmMatrix = XMMatrixTranslation(translation.x, translation.y, translation.z);
    return fromXMMatrix(xmMatrix);
}

Matrix4x4 Matrix4x4::CreateRotationX(float angle)
{
    XMMATRIX xmMatrix = XMMatrixRotationX(angle);
    return fromXMMatrix(xmMatrix);
}

Matrix4x4 Matrix4x4::CreateRotationY(float angle)
{
    XMMATRIX xmMatrix = XMMatrixRotationY(angle);
    return fromXMMatrix(xmMatrix);
}

Matrix4x4 Matrix4x4::CreateRotationZ(float angle)
{
    XMMATRIX xmMatrix = XMMatrixRotationZ(angle);
    return fromXMMatrix(xmMatrix);
}

Matrix4x4 Matrix4x4::CreateScale(const Vector3& scale)
{
    XMMATRIX xmMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);
    return fromXMMatrix(xmMatrix);
}

Matrix4x4 Matrix4x4::CreatePerspectiveFovLH(float fovY, float aspectRatio, float nearPlane, float farPlane)
{
    XMMATRIX xmMatrix = XMMatrixPerspectiveFovLH(fovY, aspectRatio, nearPlane, farPlane);
    return fromXMMatrix(xmMatrix);
}

Matrix4x4 Matrix4x4::CreateLookAtLH(const Vector3& eye, const Vector3& target, const Vector3& up)
{
    XMVECTOR eyeVec = XMVectorSet(eye.x, eye.y, eye.z, 1.0f);
    XMVECTOR targetVec = XMVectorSet(target.x, target.y, target.z, 1.0f);
    XMVECTOR upVec = XMVectorSet(up.x, up.y, up.z, 0.0f);

    XMMATRIX xmMatrix = XMMatrixLookAtLH(eyeVec, targetVec, upVec);
    return fromXMMatrix(xmMatrix);
}

DirectX::XMMATRIX Matrix4x4::toXMMatrix() const
{
    return XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(this));
}

Matrix4x4 Matrix4x4::fromXMMatrix(const DirectX::XMMATRIX& xmMatrix)
{
    Matrix4x4 result;
    XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(&result), xmMatrix);
    return result;
}