#pragma once
#include <DX3D/Particles/Particle.h>

namespace dx3d
{
    class SnowParticle : public Particle
    {
    public:
        SnowParticle();
        virtual ~SnowParticle() = default;

        virtual void initialize(const Vector3& position, const Vector3& velocity) override;

        virtual bool update(float deltaTime) override;

    private:
        float m_swayAmount;      
        float m_swaySpeed;       
        float m_swayPhase;       
        float m_fallSpeedMultiplier; 
    };

    std::unique_ptr<Particle> createSnowParticle();
}