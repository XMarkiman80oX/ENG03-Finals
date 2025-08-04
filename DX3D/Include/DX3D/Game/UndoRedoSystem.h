#pragma once
#include <DX3D/Core/Core.h>
#include <DX3D/Math/Math.h>
#include <memory>
#include <vector>
#include <string>

namespace dx3d
{
    class AGameObject;
    class LightObject;

    // Base class for all undoable actions
    class IAction
    {
    public:
        virtual ~IAction() = default;
        virtual void execute() = 0;
        virtual void undo() = 0;
        virtual std::string getDescription() const = 0;
    };

    // Transform action for position, rotation, scale changes
    class TransformAction : public IAction
    {
    public:
        TransformAction(std::shared_ptr<AGameObject> object,
            const Vector3& oldPos, const Vector3& newPos,
            const Vector3& oldRot, const Vector3& newRot,
            const Vector3& oldScale, const Vector3& newScale);

        void execute() override;
        void undo() override;
        std::string getDescription() const override;

    private:
        std::weak_ptr<AGameObject> m_object;
        Vector3 m_oldPosition, m_newPosition;
        Vector3 m_oldRotation, m_newRotation;
        Vector3 m_oldScale, m_newScale;
    };

    // Delete action for object deletion
    class DeleteAction : public IAction
    {
    public:
        DeleteAction(std::shared_ptr<AGameObject> object,
            std::vector<std::shared_ptr<AGameObject>>& objectList,
            std::vector<std::shared_ptr<LightObject>>& lightList);

        void execute() override;
        void undo() override;
        std::string getDescription() const override;

    private:
        std::shared_ptr<AGameObject> m_object;
        std::vector<std::shared_ptr<AGameObject>>* m_objectList;
        std::vector<std::shared_ptr<LightObject>>* m_lightList;
        size_t m_originalIndex;
    };

    // Create action for object creation
    class CreateAction : public IAction
    {
    public:
        CreateAction(std::shared_ptr<AGameObject> object,
            std::vector<std::shared_ptr<AGameObject>>& objectList);

        void execute() override;
        void undo() override;
        std::string getDescription() const override;

    private:
        std::shared_ptr<AGameObject> m_object;
        std::vector<std::shared_ptr<AGameObject>>* m_objectList;
    };

    // Main undo/redo system
    class UndoRedoSystem
    {
    public:
        explicit UndoRedoSystem(ui32 maxActions = 10);
        ~UndoRedoSystem() = default;

        // Execute and record an action
        void recordAction(std::unique_ptr<IAction> action);
        void executeAction(std::unique_ptr<IAction> action);

        // Undo/Redo operations
        bool canUndo() const;
        bool canRedo() const;
        void undo();
        void redo();

        // Clear history
        void clear();

        // Get descriptions for UI
        std::string getUndoDescription() const;
        std::string getRedoDescription() const;

        // Configuration
        void setMaxActions(ui32 maxActions) { m_maxActions = maxActions; }
        ui32 getMaxActions() const { return m_maxActions; }

        // Statistics
        ui32 getUndoCount() const { return static_cast<ui32>(m_undoStack.size()); }
        ui32 getRedoCount() const { return static_cast<ui32>(m_redoStack.size()); }

    private:
        void trimUndoStack();

    private:
        std::vector<std::unique_ptr<IAction>> m_undoStack;
        std::vector<std::unique_ptr<IAction>> m_redoStack;
        ui32 m_maxActions;
    };
}