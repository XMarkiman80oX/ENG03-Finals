#pragma once
#include <../Graphics/VertexBuffer.h>
#include <../Graphics/IndexBuffer.h>
#include <../Math/Math.h>
#include <../ECS/Entity.h>
#include <../ECS/Components/TransformComponent.h>
#include <../ECS/Components/PhysicsComponent.h>
#include <../ECS/Components/MaterialComponent.h>
#include <../Graphics/GraphicsEngine.h>
#include <../Graphics/RenderSystem.h>
#include <memory>
#include <typeinfo>
#include <vector>
#include <algorithm>

namespace dx3d
{
    class AGameObject : public std::enable_shared_from_this<AGameObject>
    {
    public:
        struct Transform
        {
            Vector3 position{ 0.0f, 0.0f, 0.0f };
            Vector3 rotation{ 0.0f, 0.0f, 0.0f };
            Vector3 scale{ 1.0f, 1.0f, 1.0f };

            Matrix4x4 getLocalMatrix() const;
        };

    public:
        AGameObject();
        AGameObject(const Vector3& position, const Vector3& rotation = Vector3(0, 0, 0), const Vector3& scale = Vector3(1, 1, 1));
        virtual ~AGameObject();

        void setPosition(const Vector3& position);
        void setRotation(const Vector3& rotation);
        void setScale(const Vector3& scale);

        const Vector3& getPosition() const;
        const Vector3& getRotation() const;
        const Vector3& getScale() const;

        const Vector3& getLocalPosition() const { return m_transform.position; }
        const Vector3& getLocalRotation() const { return m_transform.rotation; }
        const Vector3& getLocalScale() const { return m_transform.scale; }

        Vector3 getWorldPosition() const;
        Vector3 getWorldRotation() const;
        Vector3 getWorldScale() const;

        Matrix4x4 getWorldMatrix() const;
        Matrix4x4 getLocalMatrix() const { return m_transform.getLocalMatrix(); }

        void rotate(const Vector3& deltaRotation);
        void translate(const Vector3& deltaPosition);

        void setEnabled(bool enabled);
        bool isEnabled() const { return m_enabled; }

        void enablePhysics(PhysicsBodyType bodyType = PhysicsBodyType::Dynamic);
        void disablePhysics();
        bool hasPhysics() const;

        void setPhysicsMass(float mass);
        void setPhysicsRestitution(float restitution);
        void setPhysicsFriction(float friction);

        void applyForce(const Vector3& force);
        void applyImpulse(const Vector3& impulse);

        Vector3 getLinearVelocity() const;
        void setLinearVelocity(const Vector3& velocity);

        Entity getEntity() const { return m_entity; }

        virtual void update(float deltaTime) {}
        virtual void render() {}

        std::string getObjectType();

        void attachMaterial(std::shared_ptr<Material> material);
        void detachMaterial();
        bool hasMaterial() const;
        std::shared_ptr<Material> getMaterial() const;
        void setTexture(const std::string& textureFileName);
        std::string getTextureName() const;

        void setParent(std::shared_ptr<AGameObject> parent);
        void removeParent();
        std::shared_ptr<AGameObject> getParent() const { return m_parent.lock(); }
        bool hasParent() const { return !m_parent.expired(); }

        void addChild(std::shared_ptr<AGameObject> child);
        void removeChild(std::shared_ptr<AGameObject> child);
        const std::vector<std::weak_ptr<AGameObject>>& getChildren() const { return m_children; }
        bool hasChildren() const { return !m_children.empty(); }

        void setWorldPosition(const Vector3& worldPos);
        void setWorldRotation(const Vector3& worldRot);
        void setWorldScale(const Vector3& worldScale);

    protected:
        virtual CollisionShapeType getCollisionShapeType() const = 0;
        virtual PhysicsComponent createPhysicsComponent() const;

        void syncTransformFromECS();
        void syncTransformToECS();
        void updateChildrenTransforms();

    protected:
        Transform m_transform;
        Entity m_entity;
        bool m_enabled = true;
        bool m_hadPhysicsBeforeDisable = false;
        PhysicsBodyType m_previousBodyType = PhysicsBodyType::Dynamic;

        std::weak_ptr<AGameObject> m_parent;
        std::vector<std::weak_ptr<AGameObject>> m_children;

    private:
        static EntityID s_nextEntityID;
        Matrix4x4 getParentWorldMatrix() const;
    };
}