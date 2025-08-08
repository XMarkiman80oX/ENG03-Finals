#pragma once
#include <memory>
#include <vector>
#include <string>
namespace dx3d
{
    class BaseGameObject;
    class SelectionSystem;
    class UIController;
    class SceneStateManager;
    class UndoRedoSystem;

    class SceneOutlinerUI
    {
    public:
        SceneOutlinerUI(
            UIController& controller,
            SelectionSystem& selectionSystem,
            SceneStateManager& sceneStateManager,
            UndoRedoSystem& undoRedoSystem,
            const std::vector<std::shared_ptr<BaseGameObject>>& gameObjects
        );

        void render(float deltaTime);
        void renderHierarchyWindow();

    private:
        void renderHierarchy();
        void renderObjectNode(std::shared_ptr<BaseGameObject> object, int& nodeIndex);
        std::string getObjectDisplayName(std::shared_ptr<BaseGameObject> object, int index);
        std::string getObjectIcon(std::shared_ptr<BaseGameObject> object);

    private:
        UIController& m_controller;
        SelectionSystem& m_selectionSystem;
        SceneStateManager& m_sceneStateManager;
        UndoRedoSystem& m_undoRedoSystem;
        const std::vector<std::shared_ptr<BaseGameObject>>& m_gameObjects;
        std::shared_ptr<BaseGameObject> m_draggedObject = nullptr;
    };
}