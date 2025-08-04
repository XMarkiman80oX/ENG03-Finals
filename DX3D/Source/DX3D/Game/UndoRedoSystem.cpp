#include <DX3D/Game/UndoRedoSystem.h>
#include <DX3D/Graphics/Primitives/AGameObject.h>
#include <DX3D/Graphics/Primitives/LightObject.h>
#include <algorithm>
#include <iterator>

using namespace dx3d;

// TransformAction Implementation
TransformAction::TransformAction(std::shared_ptr<AGameObject> object,
    const Vector3& oldPos, const Vector3& newPos,
    const Vector3& oldRot, const Vector3& newRot,
    const Vector3& oldScale, const Vector3& newScale)
    : m_object(object)
    , m_oldPosition(oldPos), m_newPosition(newPos)
    , m_oldRotation(oldRot), m_newRotation(newRot)
    , m_oldScale(oldScale), m_newScale(newScale)
{
}

void UndoRedoSystem::recordAction(std::unique_ptr<IAction> action)
{
    if (!action)
        return;

    // Add to undo stack WITHOUT executing (since the action has already been applied)
    m_undoStack.push_back(std::move(action));

    // Clear redo stack since we performed a new action
    m_redoStack.clear();

    // Trim undo stack if it exceeds maximum
    trimUndoStack();
}

void TransformAction::execute()
{
    if (auto obj = m_object.lock())
    {
        obj->setPosition(m_newPosition);
        obj->setRotation(m_newRotation);
        obj->setScale(m_newScale);
    }
}

void TransformAction::undo()
{
    if (auto obj = m_object.lock())
    {
        obj->setPosition(m_oldPosition);
        obj->setRotation(m_oldRotation);
        obj->setScale(m_oldScale);
    }
}

std::string TransformAction::getDescription() const
{
    return "Transform Object";
}

// DeleteAction Implementation
DeleteAction::DeleteAction(std::shared_ptr<AGameObject> object,
    std::vector<std::shared_ptr<AGameObject>>& objectList,
    std::vector<std::shared_ptr<LightObject>>& lightList)
    : m_object(object)
    , m_objectList(&objectList)
    , m_lightList(&lightList)
    , m_originalIndex(0)
{
    // Find the index of the object in the list
    auto it = std::find(objectList.begin(), objectList.end(), object);
    if (it != objectList.end())
    {
        m_originalIndex = static_cast<size_t>(std::distance(objectList.begin(), it));
    }

    if (auto light = std::dynamic_pointer_cast<LightObject>(m_object)) {
        auto light_it = std::find(m_lightList->begin(), m_lightList->end(), light);
        if (light_it != m_lightList->end()) {
            m_lightList->erase(light_it);
        }
    }
}

void DeleteAction::execute()
{
    if (m_object && m_objectList)
    {
        auto it = std::find(m_objectList->begin(), m_objectList->end(), m_object);
        if (it != m_objectList->end())
        {
            m_objectList->erase(it);
        }
    }
}

void DeleteAction::undo()
{
    if (m_object && m_objectList)
    {
        // Insert back at original position or at end if index is out of bounds
        if (m_originalIndex <= m_objectList->size())
        {
            m_objectList->insert(m_objectList->begin() + m_originalIndex, m_object);
        }
        else
        {
            m_objectList->push_back(m_object);
        }

        if (auto light = std::dynamic_pointer_cast<LightObject>(m_object)) {
            m_lightList->push_back(light);
        }
    }
}

std::string DeleteAction::getDescription() const
{
    return "Delete Object";
}

// CreateAction Implementation
CreateAction::CreateAction(std::shared_ptr<AGameObject> object,
    std::vector<std::shared_ptr<AGameObject>>& objectList)
    : m_object(object)
    , m_objectList(&objectList)
{
}

void CreateAction::execute()
{
    if (m_object && m_objectList)
    {
        m_objectList->push_back(m_object);
    }
}

void CreateAction::undo()
{
    if (m_object && m_objectList)
    {
        auto it = std::find(m_objectList->begin(), m_objectList->end(), m_object);
        if (it != m_objectList->end())
        {
            m_objectList->erase(it);
        }
    }
}

std::string CreateAction::getDescription() const
{
    return "Create Object";
}

// UndoRedoSystem Implementation
UndoRedoSystem::UndoRedoSystem(ui32 maxActions)
    : m_maxActions(maxActions)
{
    m_undoStack.reserve(maxActions);
    m_redoStack.reserve(maxActions);
}

void UndoRedoSystem::executeAction(std::unique_ptr<IAction> action)
{
    if (!action)
        return;

    // Execute the action
    action->execute();

    // Add to undo stack
    m_undoStack.push_back(std::move(action));

    // Clear redo stack since we performed a new action
    m_redoStack.clear();

    // Trim undo stack if it exceeds maximum
    trimUndoStack();
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
    if (!canUndo())
        return;

    // Get the last action from undo stack
    auto action = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    // Undo the action
    action->undo();

    // Move to redo stack
    m_redoStack.push_back(std::move(action));
}

void UndoRedoSystem::redo()
{
    if (!canRedo())
        return;

    // Get the last action from redo stack
    auto action = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    // Execute the action again
    action->execute();

    // Move back to undo stack
    m_undoStack.push_back(std::move(action));
}

void UndoRedoSystem::clear()
{
    m_undoStack.clear();
    m_redoStack.clear();
}

std::string UndoRedoSystem::getUndoDescription() const
{
    if (canUndo())
    {
        return "Undo " + m_undoStack.back()->getDescription();
    }
    return "Nothing to Undo";
}

std::string UndoRedoSystem::getRedoDescription() const
{
    if (canRedo())
    {
        return "Redo " + m_redoStack.back()->getDescription();
    }
    return "Nothing to Redo";
}

void UndoRedoSystem::trimUndoStack()
{
    while (m_undoStack.size() > m_maxActions)
    {
        m_undoStack.erase(m_undoStack.begin());
    }
}