#include <../Scene/SceneStateManager.h>
#include <../Graphics/Primitives/AGameObject.h>
#include <../Physics/PhysicsSystem.h>
#include <../Core/Logger.h>

using namespace dx3d;

SceneStateManager::SceneStateManager()
{
    m_stateInfo.currentState = SceneState::Edit;
    m_stateInfo.previousState = SceneState::Edit;
    m_stateInfo.canFrameStep = false;
    m_stateInfo.frameStepRequested = false;
}

SceneStateManager::~SceneStateManager()
{
}

void SceneStateManager::transitionToEdit()
{
    SceneState oldState = m_stateInfo.currentState;
    setState(SceneState::Edit);

    if (oldState == SceneState::Play || oldState == SceneState::Pause)
    {
        notifyStateChange(oldState, SceneState::Edit);
    }
}

void SceneStateManager::transitionToPlay()
{
    SceneState oldState = m_stateInfo.currentState;
    setState(SceneState::Play);
    notifyStateChange(oldState, SceneState::Play);
}

void SceneStateManager::transitionToPause()
{
    if (m_stateInfo.currentState != SceneState::Play)
        return;

    SceneState oldState = m_stateInfo.currentState;
    setState(SceneState::Pause);
    notifyStateChange(oldState, SceneState::Pause);
}

void SceneStateManager::frameStep()
{
    if (m_stateInfo.currentState != SceneState::Pause)
        return;

    m_stateInfo.frameStepRequested = true;
}

void SceneStateManager::saveObjectStates(const std::vector<std::shared_ptr<AGameObject>>& objects)
{
    m_objectSnapshots.clear();

    for (const auto& object : objects)
    {
        if (!object) continue;

        ObjectSnapshot snapshot;
        snapshot.position = object->getPosition();
        snapshot.rotation = object->getRotation();
        snapshot.scale = object->getScale();
        snapshot.hadPhysics = object->hasPhysics();

        if (snapshot.hadPhysics)
        {
            snapshot.linearVelocity = object->getLinearVelocity();
        }
        else
        {
            snapshot.linearVelocity = Vector3(0, 0, 0);
        }

        uint32_t objectId = object->getEntity().getID();
        m_objectSnapshots[objectId] = snapshot;
    }
}

void SceneStateManager::restoreObjectStates(const std::vector<std::shared_ptr<AGameObject>>& objects)
{
    for (const auto& object : objects)
    {
        if (!object) continue;

        uint32_t objectId = object->getEntity().getID();
        auto it = m_objectSnapshots.find(objectId);

        if (it != m_objectSnapshots.end())
        {
            const ObjectSnapshot& snapshot = it->second;

            object->setPosition(snapshot.position);
            object->setRotation(snapshot.rotation);
            object->setScale(snapshot.scale);

            if (object->hasPhysics())
            {
                object->setLinearVelocity(Vector3(0, 0, 0));
            }
        }
    }
}

void SceneStateManager::addStateChangeCallback(const StateChangeCallback& callback)
{
    m_callbacks.push_back(callback);
}

void SceneStateManager::update(float deltaTime)
{
    /*if (m_stateInfo.frameStepRequested)
    {
        m_stateInfo.frameStepRequested = false;
    }*/
}

void SceneStateManager::setState(SceneState newState)
{
    m_stateInfo.previousState = m_stateInfo.currentState;
    m_stateInfo.currentState = newState;

    m_stateInfo.canFrameStep = (newState == SceneState::Pause);
}

void SceneStateManager::notifyStateChange(SceneState oldState, SceneState newState)
{
    for (const auto& callback : m_callbacks)
    {
        callback(oldState, newState);
    }
}