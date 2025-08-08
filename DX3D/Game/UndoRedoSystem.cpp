#include <../Game/UndoRedoSystem.h>
#include <../Graphics/Primitives/AGameObject.h>
#include <../Graphics/Primitives/LightObject.h>
#include <algorithm>

using namespace dx3d;

CreateAction::CreateAction(std::shared_ptr<BaseGameObject> object,
    std::vector<std::shared_ptr<BaseGameObject>>& gameObjects)
    : m_object(object)
    , m_gameObjects(gameObjects)
    , m_insertIndex(static_cast<int>(gameObjects.size()))
{
}

void CreateAction::undo()
{
    auto it = std::find(m_gameObjects.begin(), m_gameObjects.end(), m_object);
    if (it != m_gameObjects.end())
    {
        m_insertIndex = static_cast<int>(std::distance(m_gameObjects.begin(), it));
        m_gameObjects.erase(it);
    }
}

void CreateAction::redo()
{
    if (m_insertIndex >= 0 && m_insertIndex <= static_cast<int>(m_gameObjects.size()))
    {
        m_gameObjects.insert(m_gameObjects.begin() + m_insertIndex, m_object);
    }
    else
    {
        m_gameObjects.push_back(m_object);
    }
}

std::string CreateAction::getDescription() const
{
    return "Create " + m_object->getObjectType();
}

DeleteAction::DeleteAction(std::shared_ptr<BaseGameObject> object,
    std::vector<std::shared_ptr<BaseGameObject>>& gameObjects,
    std::vector<std::shared_ptr<LightObject>>& lights)
    : m_object(object)
    , m_gameObjects(gameObjects)
    , m_lights(lights)
    , m_objectIndex(-1)
    , m_wasLight(false)
{
    if (std::dynamic_pointer_cast<LightObject>(object))
    {
        m_wasLight = true;
    }
}

void DeleteAction::undo()
{
    if (m_objectIndex >= 0 && m_objectIndex <= static_cast<int>(m_gameObjects.size()))
    {
        m_gameObjects.insert(m_gameObjects.begin() + m_objectIndex, m_object);

        if (m_wasLight)
        {
            auto lightObj = std::static_pointer_cast<LightObject>(m_object);
            m_lights.push_back(lightObj);
        }
    }
}

void DeleteAction::redo()
{
    auto it = std::find(m_gameObjects.begin(), m_gameObjects.end(), m_object);
    if (it != m_gameObjects.end())
    {
        m_objectIndex = static_cast<int>(std::distance(m_gameObjects.begin(), it));
        m_gameObjects.erase(it);

        if (m_wasLight)
        {
            auto lightObj = std::static_pointer_cast<LightObject>(m_object);
            auto lightIt = std::find(m_lights.begin(), m_lights.end(), lightObj);
            if (lightIt != m_lights.end())
            {
                m_lights.erase(lightIt);
            }
        }
    }
}

std::string DeleteAction::getDescription() const
{
    return "Delete " + m_object->getObjectType();
}

TransformAction::TransformAction(std::shared_ptr<BaseGameObject> object,
    const Vector3& oldPos, const Vector3& newPos,
    const Vector3& oldRot, const Vector3& newRot,
    const Vector3& oldScale, const Vector3& newScale)
    : m_object(object)
    , m_oldPosition(oldPos)
    , m_newPosition(newPos)
    , m_oldRotation(oldRot)
    , m_newRotation(newRot)
    , m_oldScale(oldScale)
    , m_newScale(newScale)
{
}

void TransformAction::undo()
{
    if (m_object)
    {
        m_object->setPosition(m_oldPosition);
        m_object->setRotation(m_oldRotation);
        m_object->setScale(m_oldScale);
    }
}

void TransformAction::redo()
{
    if (m_object)
    {
        m_object->setPosition(m_newPosition);
        m_object->setRotation(m_newRotation);
        m_object->setScale(m_newScale);
    }
}

std::string TransformAction::getDescription() const
{
    return "Transform " + m_object->getObjectType();
}

ParentAction::ParentAction(std::shared_ptr<BaseGameObject> child,
    std::shared_ptr<BaseGameObject> oldParent,
    std::shared_ptr<BaseGameObject> newParent,
    std::vector<std::shared_ptr<BaseGameObject>>& gameObjects)
    : m_child(child)
    , m_oldParent(oldParent)
    , m_newParent(newParent)
    , m_gameObjects(gameObjects)
{
    if (child)
    {
        m_oldWorldPos = child->getWorldPosition();
        m_oldWorldRot = child->getWorldRotation();
        m_oldWorldScale = child->getWorldScale();
    }
}

void ParentAction::undo()
{
    if (!m_child)
        return;

    if (auto oldParent = m_oldParent.lock())
    {
        m_child->setParent(oldParent);
    }
    else
    {
        m_child->removeParent();
    }

    m_child->setWorldPosition(m_oldWorldPos);
    m_child->setWorldRotation(m_oldWorldRot);
    m_child->setWorldScale(m_oldWorldScale);

    bool found = false;
    for (const auto& obj : m_gameObjects) { if (obj == m_child) { found = true; break; } }
    if (!found) { m_gameObjects.push_back(m_child); }
}

void ParentAction::redo()
{
    if (!m_child)
        return;

    if (auto newParent = m_newParent.lock())
    {
        m_child->setParent(newParent);
    }
    else
    {
        m_child->removeParent();
    }

    bool found = false;
    for (const auto& obj : m_gameObjects) { if (obj == m_child) { found = true; break; } }
    if (!found) { m_gameObjects.push_back(m_child); }
}

std::string ParentAction::getDescription() const
{
    std::string desc = "Parent " + m_child->getObjectType();
    if (auto newParent = m_newParent.lock())
    {
        desc += " to " + newParent->getObjectType();
    }
    else
    {
        desc += " (unparent)";
    }
    return desc;
}

UndoRedoSystem::UndoRedoSystem(size_t maxHistorySize)
    : m_maxHistorySize(maxHistorySize)
{
}

UndoRedoSystem::~UndoRedoSystem()
{
}

void UndoRedoSystem::executeAction(std::unique_ptr<IUndoableAction> action)
{
    action->redo();
    m_undoStack.push(std::move(action));

    while (!m_redoStack.empty())
    {
        m_redoStack.pop();
    }

    while (m_undoStack.size() > m_maxHistorySize)
    {
        std::stack<std::unique_ptr<IUndoableAction>> tempStack;
        while (m_undoStack.size() > 1)
        {
            tempStack.push(std::move(m_undoStack.top()));
            m_undoStack.pop();
        }
        m_undoStack.pop();

        while (!tempStack.empty())
        {
            m_undoStack.push(std::move(tempStack.top()));
            tempStack.pop();
        }
    }
}

void UndoRedoSystem::recordAction(std::unique_ptr<IUndoableAction> action)
{
    m_undoStack.push(std::move(action));

    while (!m_redoStack.empty())
    {
        m_redoStack.pop();
    }

    while (m_undoStack.size() > m_maxHistorySize)
    {
        std::stack<std::unique_ptr<IUndoableAction>> tempStack;
        while (m_undoStack.size() > 1)
        {
            tempStack.push(std::move(m_undoStack.top()));
            m_undoStack.pop();
        }
        m_undoStack.pop();

        while (!tempStack.empty())
        {
            m_undoStack.push(std::move(tempStack.top()));
            tempStack.pop();
        }
    }
}

bool UndoRedoSystem::canUndo() const
{
    return !m_undoStack.empty();
}

bool UndoRedoSystem::canRedo() const
{
    return !m_redoStack.empty();
}

void UndoRedoSystem::undo()
{
    if (canUndo())
    {
        auto action = std::move(m_undoStack.top());
        m_undoStack.pop();

        action->undo();

        m_redoStack.push(std::move(action));
    }
}

void UndoRedoSystem::redo()
{
    if (canRedo())
    {
        auto action = std::move(m_redoStack.top());
        m_redoStack.pop();

        action->redo();

        m_undoStack.push(std::move(action));
    }
}

void UndoRedoSystem::clear()
{
    while (!m_undoStack.empty())
    {
        m_undoStack.pop();
    }

    while (!m_redoStack.empty())
    {
        m_redoStack.pop();
    }
}

std::string UndoRedoSystem::getUndoDescription() const
{
    if (canUndo())
    {
        return m_undoStack.top()->getDescription();
    }
    return "";
}

std::string UndoRedoSystem::getRedoDescription() const
{
    if (canRedo())
    {
        return m_redoStack.top()->getDescription();
    }
    return "";
}