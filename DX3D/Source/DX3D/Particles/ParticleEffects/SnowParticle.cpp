#include <DX3D/Particles/ParticleEffects/SnowParticle.h>
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
    // Call base initialization
    Particle::initialize(position, velocity);

    // Randomize snow-specific properties
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    // Randomize sway parameters
    m_swayAmount = 0.3f + dist(gen) * 0.4f;  // 0.3 to 0.7
    m_swaySpeed = 1.5f + dist(gen) * 2.0f;   // 1.5 to 3.5
    m_swayPhase = dist(gen) * 6.28318530718f; // Random starting phase

    // Randomize fall speed (slower particles appear further away)
    m_fallSpeedMultiplier = 0.5f + dist(gen) * 0.5f; // 0.5 to 1.0

    // Set some rotation for visual variety
    m_rotation = dist(gen) * 6.28318530718f;
    m_rotationSpeed = (dist(gen) - 0.5f) * 2.0f; // -1 to 1 radians/second

    // Modify the initial velocity based on fall speed
    m_velocity.y *= m_fallSpeedMultiplier;
}

bool SnowParticle::update(float deltaTime)
{
    if (!m_alive)
        return false;

    // Update age first
    m_age += deltaTime;

    // Check if particle has expired
    if (m_age >= m_lifetime)
    {
        m_alive = false;
        return false;
    }

    // Update sway phase
    m_swayPhase += m_swaySpeed * deltaTime;

    // Calculate horizontal sway movement
    float swayX = std::sin(m_swayPhase) * m_swayAmount;
    float swayZ = std::cos(m_swayPhase * 0.7f) * m_swayAmount * 0.5f; // Different frequency for Z

    // Apply sway to velocity (temporary adjustment)
    Vector3 swayVelocity = m_velocity;
    swayVelocity.x += swayX;
    swayVelocity.z += swayZ;

    // Update position with swayed velocity
    m_position += swayVelocity * deltaTime;

    // Apply downward acceleration (gravity)
    m_velocity += m_acceleration * deltaTime * m_fallSpeedMultiplier;

    // Update rotation
    m_rotation += m_rotationSpeed * deltaTime;

    // Keep rotation in [0, 2π] range
    const float twoPi = 6.28318530718f;
    if (m_rotation > twoPi)
        m_rotation -= twoPi;
    else if (m_rotation < 0.0f)
        m_rotation += twoPi;

    // Vary size slightly based on age for a twinkling effect
    float sizeVariation = 0.9f + 0.1f * std::sin(m_age * 10.0f);
    m_size = m_size * sizeVariation;

    return true;
}

// Factory function implementation
std::unique_ptr<Particle> dx3d::createSnowParticle()
{
    return std::make_unique<SnowParticle>();
}