#include <../Particles/Particle.h>

using namespace dx3d;

Particle::Particle()
    : m_position(0, 0, 0)
    , m_velocity(0, 0, 0)
    , m_acceleration(0, 0, 0)
    , m_color(1, 1, 1, 1)
    , m_size(1.0f)
    , m_rotation(0.0f)
    , m_rotationSpeed(0.0f)
    , m_age(0.0f)
    , m_lifetime(1.0f)
    , m_alive(false)
{
}

void Particle::initialize(const Vector3& position, const Vector3& velocity)
{
    m_position = position;
    m_velocity = velocity;
    m_age = 0.0f;
    m_alive = true;
}

bool Particle::update(float deltaTime)
{
    if (!m_alive)
        return false;

    // Update age
    m_age += deltaTime;

    // Check if particle has expired
    if (m_age >= m_lifetime)
    {
        m_alive = false;
        return false;
    }

    // Update physics
    m_velocity += m_acceleration * deltaTime;
    m_position += m_velocity * deltaTime;

    // Update rotation
    m_rotation += m_rotationSpeed * deltaTime;

    // Keep rotation in [0, 2π] range
    const float twoPi = 6.28318530718f;
    if (m_rotation > twoPi)
        m_rotation -= twoPi;
    else if (m_rotation < 0.0f)
        m_rotation += twoPi;

    return true;
}

void Particle::reset()
{
    m_position = Vector3(0, 0, 0);
    m_velocity = Vector3(0, 0, 0);
    m_acceleration = Vector3(0, 0, 0);
    m_color = Vector4(1, 1, 1, 1);
    m_size = 1.0f;
    m_rotation = 0.0f;
    m_rotationSpeed = 0.0f;
    m_age = 0.0f;
    m_lifetime = 1.0f;
    m_alive = false;
}