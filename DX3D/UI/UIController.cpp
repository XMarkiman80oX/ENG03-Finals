#include <../Core/EventLog.h>
#include <../Game/SelectionSystem.h>
#include <../Game/UndoRedoSystem.h>
#include <../Graphics/Primitives/AGameObject.h>
#include <../Graphics/Primitives/LightObject.h>
#include <../Scene/SceneStateManager.h>
#include <../UI/UIController.h>

using namespace dx3d;

UIController::UIController(
    UndoRedoSystem& undoRedoSystem,
    SelectionSystem& selectionSystem,
    SceneStateManager& sceneStateManager,
    std::vector<std::shared_ptr<BaseGameObject>>& gameObjects,
    std::vector<std::shared_ptr<LightObject>>& lights)
    : m_undoRedoSystem(undoRedoSystem)
    , m_selectionSystem(selectionSystem)
    , m_sceneStateManager(sceneStateManager)
    , m_gameObjects(gameObjects)
    , m_lights(lights)
{
}

void UIController::onUndoClicked()
{
    if (m_undoRedoSystem.canUndo())
    {
        m_undoRedoSystem.undo();
    }
}

void UIController::onRedoClicked()
{
    if (m_undoRedoSystem.canRedo())
    {
        m_undoRedoSystem.redo();
    }
}

void UIController::onDeleteClicked()
{
    auto selectedObject = m_selectionSystem.getSelectedObject();
    if (selectedObject && m_sceneStateManager.isEditMode())
    {
        auto deleteAction = std::make_unique<DeleteAction>(selectedObject, m_gameObjects, m_lights);
        m_undoRedoSystem.executeAction(std::move(deleteAction));
        m_selectionSystem.setSelectedObject(nullptr);
    }
}

void UIController::onPlayClicked()
{
    if (m_sceneStateManager.isEditMode())
    {
        m_sceneStateManager.transitionToPlay();
    }
    else if (m_sceneStateManager.isPauseMode())
    {
        m_sceneStateManager.transitionToPlay();
    }
    else if (m_sceneStateManager.isPlayMode())
    {
        m_sceneStateManager.transitionToPause();
    }
}

void UIController::onStopClicked()
{
    if (!m_sceneStateManager.isEditMode())
    {
        m_sceneStateManager.transitionToEdit();
    }
}

void UIController::onPauseClicked()
{
    if (m_sceneStateManager.isPlayMode())
    {
        m_sceneStateManager.transitionToPause();
    }
}

void UIController::onFrameStepClicked()
{
    if (m_sceneStateManager.isPauseMode())
    {
        m_sceneStateManager.frameStep();
    }
}

void UIController::onObjectSelected(std::shared_ptr<BaseGameObject> object)
{
    m_selectionSystem.setSelectedObject(object);
}

void UIController::onTransformChanged(std::shared_ptr<BaseGameObject> object,
    const Vector3& oldPos, const Vector3& newPos,
    const Vector3& oldRot, const Vector3& newRot,
    const Vector3& oldScale, const Vector3& newScale)
{
    if (!m_sceneStateManager.isEditMode())
        return;

    const float epsilon = 0.001f;
    auto isChanged = [epsilon](float a, float b) { return std::abs(a - b) > epsilon; };

    if (isChanged(oldPos.x, newPos.x) || isChanged(oldPos.y, newPos.y) || isChanged(oldPos.z, newPos.z) ||
        isChanged(oldRot.x, newRot.x) || isChanged(oldRot.y, newRot.y) || isChanged(oldRot.z, newRot.z) ||
        isChanged(oldScale.x, newScale.x) || isChanged(oldScale.y, newScale.y) || isChanged(oldScale.z, newScale.z))
    {
        auto transformAction = std::make_unique<TransformAction>(
            object,
            oldPos, newPos,
            oldRot, newRot,
            oldScale, newScale
        );

        m_undoRedoSystem.recordAction(std::move(transformAction));
    }
}

void UIController::onParentChanged(std::shared_ptr<BaseGameObject> child,
    std::shared_ptr<BaseGameObject> oldParent,
    std::shared_ptr<BaseGameObject> newParent)
{
    if (!m_sceneStateManager.isEditMode())
        return;

    auto parentAction = std::make_unique<ParentAction>(child, oldParent, newParent, m_gameObjects);
    m_undoRedoSystem.executeAction(std::move(parentAction));
}