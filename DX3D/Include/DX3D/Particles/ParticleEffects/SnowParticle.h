#pragma once
#include <DX3D/Particles/Particle.h>

namespace dx3d
{
    class SnowParticle : public Particle
    {
    public:
        SnowParticle();
        virtual ~SnowParticle() = default;

        // Override initialization for snow-specific behavior
        virtual void initialize(const Vector3& position, const Vector3& velocity) override;

        // Override update for snow-specific physics
        virtual bool update(float deltaTime) override;

    private:
        // Snow-specific properties
        float m_swayAmount;      // How much the snowflake sways side to side
        float m_swaySpeed;       // Speed of the swaying motion
        float m_swayPhase;       // Current phase of the sway
        float m_fallSpeedMultiplier; // Individual fall speed variation
    };

    // Factory function for creating snow particles
    std::unique_ptr<Particle> createSnowParticle();
}