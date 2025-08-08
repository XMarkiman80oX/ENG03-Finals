#include <../Particles/ParticleEffects/SnowParticle.h>
#include <cmath>
#include <random>

using namespace dx3d;

SnowParticle::SnowParticle()
    : Particle()
    , m_swayAmount(0.5f)
    , m_swaySpeed(2.0f)
    , m_swayPhase(0.0f)
    , m_fallSpeedMultiplier(1.0f)
{
}

void SnowParticle::initialize(const Vector3& position, const Vector3& velocity)
{
    Particle::initialize(position, velocity);

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    m_swayAmount = 0.3f + dist(gen) * 0.4f; 
    m_swaySpeed = 1.5f + dist(gen) * 2.0f;  
    m_swayPhase = dist(gen) * 6.28318530718f;
    m_fallSpeedMultiplier = 0.5f + dist(gen) * 0.5f; 
    m_rotation = dist(gen) * 6.28318530718f;
    m_rotationSpeed = (dist(gen) - 0.5f) * 2.0f; 
    m_velocity.y *= m_fallSpeedMultiplier;
}

bool SnowParticle::update(float deltaTime)
{
    if (!m_alive)
        return false;

    m_age += deltaTime;

    if (m_age >= m_lifetime)
    {
        m_alive = false;
        return false;
    }

    m_swayPhase += m_swaySpeed * deltaTime;

    float swayX = std::sin(m_swayPhase) * m_swayAmount;
    float swayZ = std::cos(m_swayPhase * 0.7f) * m_swayAmount * 0.5f;

    Vector3 swayVelocity = m_velocity;
    swayVelocity.x += swayX;
    swayVelocity.z += swayZ;

    m_position += swayVelocity * deltaTime;
    m_velocity += m_acceleration * deltaTime * m_fallSpeedMultiplier;
    m_rotation += m_rotationSpeed * deltaTime;

    const float twoPi = 6.28318530718f;
    if (m_rotation > twoPi)
        m_rotation -= twoPi;
    else if (m_rotation < 0.0f)
        m_rotation += twoPi;

    float sizeVariation = 0.9f + 0.1f * std::sin(m_age * 10.0f);
    m_size = m_size * sizeVariation;

    return true;
}

std::unique_ptr<Particle> dx3d::createSnowParticle()
{
    return std::make_unique<SnowParticle>();
}