#pragma once
#include <../Core/Base.h>
#include <../Math/Math.h>

namespace dx3d
{
    class Particle
    {
    public:
        Particle();
        virtual ~Particle() = default;
        virtual void initialize(const Vector3& position, const Vector3& velocity);
        virtual bool update(float deltaTime);
        virtual void reset();

    protected:
        Vector3 m_position;
        Vector3 m_velocity;
        Vector3 m_acceleration;
        Vector4 m_color;        
        float m_size;
        float m_rotation;       
        float m_rotationSpeed; 
        float m_age;
        float m_lifetime;
        bool m_alive;

    public:
        // Getters
        const Vector3& getPosition() const { return m_position; }
        const Vector3& getVelocity() const { return m_velocity; }
        const Vector3& getAcceleration() const { return m_acceleration; }
        const Vector4& getColor() const { return m_color; }
        float getSize() const { return m_size; }
        float getRotation() const { return m_rotation; }
        float getAge() const { return m_age; }
        float getLifetime() const { return m_lifetime; }
        bool isAlive() const { return m_alive; }

        // Setters
        void setPosition(const Vector3& position) { m_position = position; }
        void setVelocity(const Vector3& velocity) { m_velocity = velocity; }
        void setAcceleration(const Vector3& acceleration) { m_acceleration = acceleration; }
        void setColor(const Vector4& color) { m_color = color; }
        void setSize(float size) { m_size = size; }
        void setRotation(float rotation) { m_rotation = rotation; }
        void setLifetime(float lifetime) { m_lifetime = lifetime; }
    };
}