#pragma once
#include <DX3D/Core/Core.h>

namespace dx3d
{
    using EntityID = ui32;
    constexpr EntityID INVALID_ENTITY = 0;

    class Entity
    {
    public:
        Entity() : m_id(INVALID_ENTITY) {}
        explicit Entity(EntityID id) : m_id(id) {}

        EntityID getID() const { return m_id; }
        bool isValid() const { return m_id != INVALID_ENTITY; }

        bool operator==(const Entity& other) const { return m_id == other.m_id; }
        bool operator!=(const Entity& other) const { return m_id != other.m_id; }

    private:
        EntityID m_id;
    };
}