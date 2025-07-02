#include <DX3D/Graphics/Primitives/CameraObject.h>

using namespace dx3d;

CameraObject::CameraObject()
    : AGameObject()
{
    m_camera = std::make_unique<Camera>();
    syncCameraTransform();
}

CameraObject::CameraObject(const Vector3& position, const Vector3& rotation)
    : AGameObject(position, rotation)
{
    m_camera = std::make_unique<Camera>();
    syncCameraTransform();
}

void CameraObject::update(float deltaTime)
{
    AGameObject::update(deltaTime);
    syncCameraTransform();
}

void CameraObject::syncCameraTransform()
{
    m_camera->setPosition(m_transform.position);

    float yaw = m_transform.rotation.y;
    float pitch = m_transform.rotation.x;

    Vector3 forward;
    forward.x = std::sin(yaw) * std::cos(pitch);
    forward.y = std::sin(pitch);
    forward.z = std::cos(yaw) * std::cos(pitch);

    Vector3 target = m_transform.position + forward;
    m_camera->lookAt(target);
}

Matrix4x4 CameraObject::getProjectionMatrix(float aspectRatio) const
{
    return Matrix4x4::CreatePerspectiveFovLH(m_fov, aspectRatio, m_nearPlane, m_farPlane);
}

void CameraObject::alignWithView(const Camera& viewCamera)
{
    m_transform.position = viewCamera.getPosition();
    m_transform.rotation.y = viewCamera.getYaw();
    m_transform.rotation.x = viewCamera.getPitch();
    syncCameraTransform();
}