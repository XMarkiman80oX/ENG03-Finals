#pragma once
#include <DX3D/Math/Math.h>
#include <vector>
#include <memory>
#include <stack>
#include <string>

namespace dx3d
{
    class AGameObject;
    class LightObject;

    class IUndoableAction
    {
    public:
        virtual ~IUndoableAction() = default;
        virtual void undo() = 0;
        virtual void redo() = 0;
        virtual std::string getDescription() const = 0;
    };

    class CreateAction : public IUndoableAction
    {
    public:
        CreateAction(std::shared_ptr<AGameObject> object,
            std::vector<std::shared_ptr<AGameObject>>& gameObjects);

        void undo() override;
        void redo() override;
        std::string getDescription() const override;

    private:
        std::shared_ptr<AGameObject> m_object;
        std::vector<std::shared_ptr<AGameObject>>& m_gameObjects;
        int m_insertIndex;
    };

    class DeleteAction : public IUndoableAction
    {
    public:
        DeleteAction(std::shared_ptr<AGameObject> object,
            std::vector<std::shared_ptr<AGameObject>>& gameObjects,
            std::vector<std::shared_ptr<LightObject>>& lights);

        void undo() override;
        void redo() override;
        std::string getDescription() const override;

    private:
        std::shared_ptr<AGameObject> m_object;
        std::vector<std::shared_ptr<AGameObject>>& m_gameObjects;
        std::vector<std::shared_ptr<LightObject>>& m_lights;
        int m_objectIndex;
        bool m_wasLight;
    };

    class TransformAction : public IUndoableAction
    {
    public:
        TransformAction(std::shared_ptr<AGameObject> object,
            const Vector3& oldPos, const Vector3& newPos,
            const Vector3& oldRot, const Vector3& newRot,
            const Vector3& oldScale, const Vector3& newScale);

        void undo() override;
        void redo() override;
        std::string getDescription() const override;

    private:
        std::shared_ptr<AGameObject> m_object;
        Vector3 m_oldPosition;
        Vector3 m_newPosition;
        Vector3 m_oldRotation;
        Vector3 m_newRotation;
        Vector3 m_oldScale;
        Vector3 m_newScale;
    };

    class ParentAction : public IUndoableAction
    {
    public:
        ParentAction(std::shared_ptr<AGameObject> child,
            std::shared_ptr<AGameObject> oldParent,
            std::shared_ptr<AGameObject> newParent,
            std::vector<std::shared_ptr<AGameObject>>& gameObjects);

        void undo() override;
        void redo() override;
        std::string getDescription() const override;

    private:
        std::shared_ptr<AGameObject> m_child;
        std::weak_ptr<AGameObject> m_oldParent;
        std::weak_ptr<AGameObject> m_newParent;
        Vector3 m_oldWorldPos;
        Vector3 m_oldWorldRot;
        Vector3 m_oldWorldScale;
        std::vector<std::shared_ptr<AGameObject>>& m_gameObjects;
    };

    class UndoRedoSystem
    {
    public:
        explicit UndoRedoSystem(size_t maxHistorySize = 50);
        ~UndoRedoSystem();

        void executeAction(std::unique_ptr<IUndoableAction> action);
        void recordAction(std::unique_ptr<IUndoableAction> action);

        bool canUndo() const;
        bool canRedo() const;

        void undo();
        void redo();

        void clear();

        size_t getUndoCount() const { return m_undoStack.size(); }
        size_t getRedoCount() const { return m_redoStack.size(); }

        std::string getUndoDescription() const;
        std::string getRedoDescription() const;

    private:
        std::stack<std::unique_ptr<IUndoableAction>> m_undoStack;
        std::stack<std::unique_ptr<IUndoableAction>> m_redoStack;
        size_t m_maxHistorySize;
    };
}