#include <../Graphics/Primitives/AGameObject.h>
#include <../ECS/ComponentManager.h>
#include <../Physics/PhysicsSystem.h>
#include <../Graphics/ResourceManager.h>
#include <../ECS/Components/MaterialComponent.h>
#include <DirectXMath.h>

using namespace dx3d;
using namespace DirectX;

EntityID AGameObject::s_nextEntityID = 1;

AGameObject::AGameObject()
{
    m_entity = Entity(s_nextEntityID++);

    auto& componentManager = ComponentManager::getInstance();
    TransformComponent transform;
    transform.position = m_transform.position;
    transform.rotation = m_transform.rotation;
    transform.scale = m_transform.scale;
    componentManager.addComponent(m_entity.getID(), transform);
}

AGameObject::AGameObject(const Vector3& position, const Vector3& rotation, const Vector3& scale)
{
    m_entity = Entity(s_nextEntityID++);

    m_transform.position = position;
    m_transform.rotation = rotation;
    m_transform.scale = scale;

    auto& componentManager = ComponentManager::getInstance();
    TransformComponent transform;
    transform.position = position;
    transform.rotation = rotation;
    transform.scale = scale;
    componentManager.addComponent(m_entity.getID(), transform);
}

AGameObject::~AGameObject()
{
    if (hasParent())
    {
        if (auto parent = m_parent.lock())
        {
            parent->removeChild(shared_from_this());
        }
    }

    for (auto& weakChild : m_children)
    {
        if (auto child = weakChild.lock())
        {
            child->m_parent.reset();
        }
    }

    if (hasPhysics())
    {
        disablePhysics();
    }

    auto& componentManager = ComponentManager::getInstance();
    componentManager.removeEntity(m_entity.getID());
}

void AGameObject::setPosition(const Vector3& position)
{
    m_transform.position = position;
    syncTransformToECS();
    updateChildrenTransforms();
}

void AGameObject::setRotation(const Vector3& rotation)
{
    m_transform.rotation = rotation;
    syncTransformToECS();
    updateChildrenTransforms();
}

void AGameObject::setScale(const Vector3& scale)
{
    m_transform.scale = scale;
    syncTransformToECS();
    updateChildrenTransforms();
}

const Vector3& AGameObject::getPosition() const
{
    const_cast<AGameObject*>(this)->syncTransformFromECS();
    return m_transform.position;
}

const Vector3& AGameObject::getRotation() const
{
    const_cast<AGameObject*>(this)->syncTransformFromECS();
    return m_transform.rotation;
}

const Vector3& AGameObject::getScale() const
{
    const_cast<AGameObject*>(this)->syncTransformFromECS();
    return m_transform.scale;
}

Vector3 AGameObject::getWorldPosition() const
{
    if (!hasParent())
    {
        return getPosition();
    }

    Matrix4x4 worldMatrix = getWorldMatrix();
    return Vector3(worldMatrix.m[3][0], worldMatrix.m[3][1], worldMatrix.m[3][2]);
}

Vector3 AGameObject::getWorldRotation() const
{
    if (!hasParent())
    {
        return getRotation();
    }

    Vector3 worldRot = getRotation();
    if (auto parent = m_parent.lock())
    {
        Vector3 parentWorldRot = parent->getWorldRotation();
        worldRot = worldRot + parentWorldRot;
    }
    return worldRot;
}

Vector3 AGameObject::getWorldScale() const
{
    if (!hasParent())
    {
        return getScale();
    }

    Vector3 worldScale = getScale();
    if (auto parent = m_parent.lock())
    {
        Vector3 parentWorldScale = parent->getWorldScale();
        worldScale.x *= parentWorldScale.x;
        worldScale.y *= parentWorldScale.y;
        worldScale.z *= parentWorldScale.z;
    }
    return worldScale;
}

Matrix4x4 AGameObject::getWorldMatrix() const
{
    const_cast<AGameObject*>(this)->syncTransformFromECS();

    Matrix4x4 localMatrix = m_transform.getLocalMatrix();

    if (hasParent())
    {
        Matrix4x4 parentWorldMatrix = getParentWorldMatrix();
        return localMatrix * parentWorldMatrix;
    }

    return localMatrix;
}

Matrix4x4 AGameObject::getParentWorldMatrix() const
{
    if (auto parent = m_parent.lock())
    {
        return parent->getWorldMatrix();
    }
    return Matrix4x4();
}

void AGameObject::rotate(const Vector3& deltaRotation)
{
    syncTransformFromECS();
    m_transform.rotation += deltaRotation;
    syncTransformToECS();
    updateChildrenTransforms();
}

void AGameObject::translate(const Vector3& deltaPosition)
{
    syncTransformFromECS();
    m_transform.position += deltaPosition;
    syncTransformToECS();
    updateChildrenTransforms();
}

void AGameObject::setParent(std::shared_ptr<AGameObject> parent)
{
    if (parent.get() == this)
        return;

    if (auto oldParent = m_parent.lock())
    {
        oldParent->removeChild(shared_from_this());
    }

    if (parent)
    {
        Vector3 worldPos = getWorldPosition();
        Vector3 worldRot = getWorldRotation();
        Vector3 worldScale = getWorldScale();

        m_parent = parent;
        parent->addChild(shared_from_this());

        setWorldPosition(worldPos);
        setWorldRotation(worldRot);
        setWorldScale(worldScale);
    }
    else
    {
        m_parent.reset();
    }
}

void AGameObject::removeParent()
{
    if (auto parent = m_parent.lock())
    {
        Vector3 worldPos = getWorldPosition();
        Vector3 worldRot = getWorldRotation();
        Vector3 worldScale = getWorldScale();

        parent->removeChild(shared_from_this());
        m_parent.reset();

        setPosition(worldPos);
        setRotation(worldRot);
        setScale(worldScale);
    }
}

void AGameObject::addChild(std::shared_ptr<AGameObject> child)
{
    if (!child || child.get() == this)
        return;

    auto it = std::find_if(m_children.begin(), m_children.end(),
        [&child](const std::weak_ptr<AGameObject>& wptr) {
            return !wptr.expired() && wptr.lock() == child;
        });

    if (it == m_children.end())
    {
        m_children.push_back(child);
    }
}

void AGameObject::removeChild(std::shared_ptr<AGameObject> child)
{
    m_children.erase(
        std::remove_if(m_children.begin(), m_children.end(),
            [&child](const std::weak_ptr<AGameObject>& wptr) {
                return wptr.expired() || wptr.lock() == child;
            }),
        m_children.end()
    );
}

void AGameObject::setWorldPosition(const Vector3& worldPos)
{
    if (!hasParent())
    {
        setPosition(worldPos);
        return;
    }

    if (auto parent = m_parent.lock())
    {
        Matrix4x4 parentWorld = parent->getWorldMatrix();
        XMMATRIX xmParentWorld = parentWorld.toXMMatrix();
        XMMATRIX xmParentWorldInv = XMMatrixInverse(nullptr, xmParentWorld);

        XMVECTOR worldPosVec = XMVectorSet(worldPos.x, worldPos.y, worldPos.z, 1.0f);
        XMVECTOR localPosVec = XMVector3Transform(worldPosVec, xmParentWorldInv);

        XMFLOAT3 localPos;
        XMStoreFloat3(&localPos, localPosVec);

        setPosition(Vector3(localPos.x, localPos.y, localPos.z));
    }
}

void AGameObject::setWorldRotation(const Vector3& worldRot)
{
    if (!hasParent())
    {
        setRotation(worldRot);
        return;
    }

    if (auto parent = m_parent.lock())
    {
        Vector3 parentWorldRot = parent->getWorldRotation();
        setRotation(worldRot - parentWorldRot);
    }
}

void AGameObject::setWorldScale(const Vector3& worldScale)
{
    if (!hasParent())
    {
        setScale(worldScale);
        return;
    }

    if (auto parent = m_parent.lock())
    {
        Vector3 parentWorldScale = parent->getWorldScale();
        Vector3 localScale;
        localScale.x = parentWorldScale.x != 0 ? worldScale.x / parentWorldScale.x : 1.0f;
        localScale.y = parentWorldScale.y != 0 ? worldScale.y / parentWorldScale.y : 1.0f;
        localScale.z = parentWorldScale.z != 0 ? worldScale.z / parentWorldScale.z : 1.0f;
        setScale(localScale);
    }
}

void AGameObject::updateChildrenTransforms()
{
    for (auto& weakChild : m_children)
    {
        if (auto child = weakChild.lock())
        {
            child->updateChildrenTransforms();
        }
    }
}

void AGameObject::enablePhysics(PhysicsBodyType bodyType)
{
    if (hasPhysics())
    {
        disablePhysics();
    }

    PhysicsComponent physicsComp = createPhysicsComponent();
    physicsComp.bodyType = bodyType;

    PhysicsSystem::getInstance().addPhysicsComponent(m_entity.getID(), physicsComp);
}

void AGameObject::disablePhysics()
{
    if (hasPhysics())
    {
        PhysicsSystem::getInstance().removePhysicsComponent(m_entity.getID());
    }
}

bool AGameObject::hasPhysics() const
{
    auto& componentManager = ComponentManager::getInstance();
    return componentManager.hasComponent<PhysicsComponent>(m_entity.getID());
}

void AGameObject::setPhysicsMass(float mass)
{
    auto& componentManager = ComponentManager::getInstance();
    auto* physicsComp = componentManager.getComponent<PhysicsComponent>(m_entity.getID());

    if (physicsComp)
    {
        physicsComp->mass = mass;
        if (physicsComp->rigidBody && physicsComp->bodyType == PhysicsBodyType::Dynamic)
        {
            physicsComp->rigidBody->setMass(mass);
        }
    }
}

void AGameObject::setPhysicsRestitution(float restitution)
{
    auto& componentManager = ComponentManager::getInstance();
    auto* physicsComp = componentManager.getComponent<PhysicsComponent>(m_entity.getID());

    if (physicsComp)
    {
        physicsComp->restitution = restitution;
        if (physicsComp->collider)
        {
            physicsComp->collider->getMaterial().setBounciness(restitution);
        }
    }
}

void AGameObject::setPhysicsFriction(float friction)
{
    auto& componentManager = ComponentManager::getInstance();
    auto* physicsComp = componentManager.getComponent<PhysicsComponent>(m_entity.getID());

    if (physicsComp)
    {
        physicsComp->friction = friction;
        if (physicsComp->collider)
        {
            physicsComp->collider->getMaterial().setFrictionCoefficient(friction);
        }
    }
}

void AGameObject::applyForce(const Vector3& force)
{
    auto& componentManager = ComponentManager::getInstance();
    auto* physicsComp = componentManager.getComponent<PhysicsComponent>(m_entity.getID());

    if (physicsComp && physicsComp->rigidBody)
    {
        rp3d::Vector3 reactForce = PhysicsSystem::toReactVector(force);
        physicsComp->rigidBody->applyWorldForceAtCenterOfMass(reactForce);
    }
}

void AGameObject::applyImpulse(const Vector3& impulse)
{
    auto& componentManager = ComponentManager::getInstance();
    auto* physicsComp = componentManager.getComponent<PhysicsComponent>(m_entity.getID());

    if (physicsComp && physicsComp->rigidBody)
    {
        rp3d::Vector3 currentVelocity = physicsComp->rigidBody->getLinearVelocity();

        rp3d::Vector3 reactImpulse = PhysicsSystem::toReactVector(impulse);
        float mass = physicsComp->rigidBody->getMass();

        rp3d::Vector3 velocityChange = reactImpulse / mass;
        rp3d::Vector3 newVelocity = currentVelocity + velocityChange;

        physicsComp->rigidBody->setLinearVelocity(newVelocity);
    }
}

Vector3 AGameObject::getLinearVelocity() const
{
    auto& componentManager = ComponentManager::getInstance();
    auto* physicsComp = componentManager.getComponent<PhysicsComponent>(m_entity.getID());

    if (physicsComp && physicsComp->rigidBody)
    {
        return PhysicsSystem::fromReactVector(physicsComp->rigidBody->getLinearVelocity());
    }

    return Vector3(0, 0, 0);
}

void AGameObject::setLinearVelocity(const Vector3& velocity)
{
    auto& componentManager = ComponentManager::getInstance();
    auto* physicsComp = componentManager.getComponent<PhysicsComponent>(m_entity.getID());

    if (physicsComp && physicsComp->rigidBody)
    {
        rp3d::Vector3 reactVelocity = PhysicsSystem::toReactVector(velocity);
        physicsComp->rigidBody->setLinearVelocity(reactVelocity);
    }
}

std::string AGameObject::getObjectType()
{
    const std::type_info& typeInfo = typeid(*this);
    std::string rawName = typeInfo.name();

    std::string removedPrefix = "class dx3d::";
    int removdPrefixLength = removedPrefix.length();

    size_t startPos = rawName.find(removedPrefix);

    if (startPos != std::string::npos) {
        rawName.erase(startPos, removdPrefixLength);
    }
    return rawName;
}

PhysicsComponent AGameObject::createPhysicsComponent() const
{
    PhysicsComponent component;
    component.shapeType = getCollisionShapeType();

    Vector3 scale = getScale();

    switch (component.shapeType)
    {
    case CollisionShapeType::Box:
        component.boxHalfExtents = Vector3(scale.x * 0.5f, scale.y * 0.5f, scale.z * 0.5f);
        break;

    case CollisionShapeType::Sphere:
        component.sphereRadius = scale.x * 0.5f;
        break;

    case CollisionShapeType::Cylinder:
        component.cylinderRadius = scale.x * 0.5f;
        component.cylinderHeight = scale.y;
        break;

    case CollisionShapeType::Capsule:
        component.capsuleRadius = scale.x * 0.5f;
        component.capsuleHeight = scale.y;
        break;

    case CollisionShapeType::Plane:
        component.boxHalfExtents = Vector3(scale.x * 0.5f, 0.01f, scale.z * 0.5f);
        break;
    }

    return component;
}

void AGameObject::syncTransformFromECS()
{
    auto& componentManager = ComponentManager::getInstance();
    auto* transformComp = componentManager.getComponent<TransformComponent>(m_entity.getID());

    if (transformComp)
    {
        m_transform.position = transformComp->position;
        m_transform.rotation = transformComp->rotation;
        m_transform.scale = transformComp->scale;
    }
}

void AGameObject::syncTransformToECS()
{
    auto& componentManager = ComponentManager::getInstance();
    auto* transformComp = componentManager.getComponent<TransformComponent>(m_entity.getID());

    if (transformComp)
    {
        transformComp->position = m_transform.position;
        transformComp->rotation = m_transform.rotation;
        transformComp->scale = m_transform.scale;

        if (hasPhysics())
        {
            auto* physicsComp = componentManager.getComponent<PhysicsComponent>(m_entity.getID());
            if (physicsComp && physicsComp->rigidBody)
            {
                rp3d::Vector3 position = PhysicsSystem::toReactVector(m_transform.position);
                rp3d::Quaternion orientation = PhysicsSystem::toReactQuaternion(m_transform.rotation);
                rp3d::Transform transform(position, orientation);
                physicsComp->rigidBody->setTransform(transform);
            }
        }
    }
}

Matrix4x4 AGameObject::Transform::getLocalMatrix() const
{
    Matrix4x4 scaleMatrix = Matrix4x4::CreateScale(scale);
    Matrix4x4 rotationX = Matrix4x4::CreateRotationX(rotation.x);
    Matrix4x4 rotationY = Matrix4x4::CreateRotationY(rotation.y);
    Matrix4x4 rotationZ = Matrix4x4::CreateRotationZ(rotation.z);
    Matrix4x4 translationMatrix = Matrix4x4::CreateTranslation(position);

    Matrix4x4 result = scaleMatrix * rotationZ * rotationY * rotationX * translationMatrix;
    return result;
}

void AGameObject::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;

    if (!enabled)
    {
        m_hadPhysicsBeforeDisable = hasPhysics();
        if (m_hadPhysicsBeforeDisable)
        {
            auto& componentManager = ComponentManager::getInstance();
            auto* physicsComp = componentManager.getComponent<PhysicsComponent>(m_entity.getID());
            if (physicsComp)
            {
                m_previousBodyType = physicsComp->bodyType;
            }
            disablePhysics();
        }
    }
    else
    {
        if (m_hadPhysicsBeforeDisable)
        {
            enablePhysics(m_previousBodyType);

            auto& componentManager = ComponentManager::getInstance();
            auto* physicsComp = componentManager.getComponent<PhysicsComponent>(m_entity.getID());
            if (physicsComp)
            {
            }
        }
    }
}

void AGameObject::attachMaterial(std::shared_ptr<Material> material)
{
    auto& componentManager = ComponentManager::getInstance();
    MaterialComponent matComp;
    matComp.material = material;
    componentManager.addComponent(m_entity.getID(), matComp);
}

void AGameObject::detachMaterial()
{
    auto& componentManager = ComponentManager::getInstance();
    componentManager.removeComponent<MaterialComponent>(m_entity.getID());
}

bool AGameObject::hasMaterial() const
{
    auto& componentManager = ComponentManager::getInstance();
    return componentManager.hasComponent<MaterialComponent>(m_entity.getID());
}

std::shared_ptr<Material> AGameObject::getMaterial() const
{
    auto& componentManager = ComponentManager::getInstance();
    auto* matComp = componentManager.getComponent<MaterialComponent>(m_entity.getID());
    return matComp ? matComp->material : nullptr;
}

void AGameObject::setTexture(const std::string& textureFileName)
{
    auto& componentManager = ComponentManager::getInstance();
    auto* matComp = componentManager.getComponent<MaterialComponent>(m_entity.getID());

    if (!matComp)
    {
        MaterialComponent newMatComp;
        newMatComp.material = ResourceManager::getInstance().createMaterial();
        newMatComp.textureFileName = "";
        newMatComp.hasTexture = false;

        componentManager.addComponent(m_entity.getID(), newMatComp);
        matComp = componentManager.getComponent<MaterialComponent>(m_entity.getID());
    }

    if (!matComp->material)
    {
        matComp->material = ResourceManager::getInstance().createMaterial();
    }

    auto texture = ResourceManager::getInstance().loadTexture(textureFileName);
    if (texture)
    {
        matComp->material->setDiffuseTexture(texture);
        matComp->textureFileName = textureFileName;
        matComp->hasTexture = true;
    }
    else
    {
        matComp->hasTexture = false;
    }
}

std::string AGameObject::getTextureName() const
{
    auto& componentManager = ComponentManager::getInstance();
    auto* matComp = componentManager.getComponent<MaterialComponent>(m_entity.getID());

    if (matComp)
    {
        return matComp->textureFileName;
    }

    return "";
}