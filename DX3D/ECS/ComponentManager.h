#pragma once
#include "../ECS/Entity.h"
#include <unordered_map>
#include <memory>
#include <typeindex>

namespace dx3d
{
    class IComponentArray
    {
    public:
        virtual ~IComponentArray() = default;
        virtual void removeEntity(EntityID entity) = 0;
    };

    template<typename T>
    class ComponentArray : public IComponentArray
    {
    public:
        void addComponent(EntityID entity, T component)
        {
            m_components[entity] = std::move(component);
        }

        void removeComponent(EntityID entity)
        {
            m_components.erase(entity);
        }

        T* getComponent(EntityID entity)
        {
            auto it = m_components.find(entity);
            return (it != m_components.end()) ? &it->second : nullptr;
        }

        // FIX: Add const version
        const T* getComponent(EntityID entity) const
        {
            auto it = m_components.find(entity);
            return (it != m_components.end()) ? &it->second : nullptr;
        }

        bool hasComponent(EntityID entity) const
        {
            return m_components.find(entity) != m_components.end();
        }

        void removeEntity(EntityID entity) override
        {
            removeComponent(entity);
        }

        auto begin() { return m_components.begin(); }
        auto end() { return m_components.end(); }

        // FIX: Add const versions
        auto begin() const { return m_components.begin(); }
        auto end() const { return m_components.end(); }

    private:
        std::unordered_map<EntityID, T> m_components;
    };

    class ComponentManager
    {
    public:
        static ComponentManager& getInstance()
        {
            static ComponentManager instance;
            return instance;
        }

        template<typename T>
        void registerComponent()
        {
            std::type_index typeIndex = std::type_index(typeid(T));
            m_componentArrays[typeIndex] = std::make_unique<ComponentArray<T>>();
        }

        template<typename T>
        void addComponent(EntityID entity, T component)
        {
            getComponentArray<T>()->addComponent(entity, std::move(component));
        }

        template<typename T>
        void removeComponent(EntityID entity)
        {
            getComponentArray<T>()->removeComponent(entity);
        }

        template<typename T>
        T* getComponent(EntityID entity)
        {
            return getComponentArray<T>()->getComponent(entity);
        }

        // FIX: Add const version
        template<typename T>
        const T* getComponent(EntityID entity) const
        {
            return getComponentArray<T>()->getComponent(entity);
        }

        template<typename T>
        bool hasComponent(EntityID entity) const
        {
            auto array = getComponentArray<T>();
            return array ? array->hasComponent(entity) : false;
        }

        template<typename T>
        ComponentArray<T>* getComponentArray()
        {
            std::type_index typeIndex = std::type_index(typeid(T));
            auto it = m_componentArrays.find(typeIndex);
            if (it != m_componentArrays.end())
            {
                return static_cast<ComponentArray<T>*>(it->second.get());
            }
            return nullptr;
        }

        template<typename T>
        const ComponentArray<T>* getComponentArray() const
        {
            std::type_index typeIndex = std::type_index(typeid(T));
            auto it = m_componentArrays.find(typeIndex);
            if (it != m_componentArrays.end())
            {
                return static_cast<const ComponentArray<T>*>(it->second.get());
            }
            return nullptr;
        }

        void removeEntity(EntityID entity)
        {
            for (auto& pair : m_componentArrays)
            {
                pair.second->removeEntity(entity);
            }
        }

    private:
        std::unordered_map<std::type_index, std::unique_ptr<IComponentArray>> m_componentArrays;
    };
}