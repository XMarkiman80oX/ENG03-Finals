#pragma once
#include <../Core/Forward.h>
#include <../Math/Math.h>
#include <memory>
#include <vector>

namespace dx3d
{
    class BaseGameObject;
    class SceneCamera;

    class SelectionSystem
    {
    public:
        SelectionSystem();
        ~SelectionSystem();

        void setSelectedObject(std::shared_ptr<BaseGameObject> object);
        std::shared_ptr<BaseGameObject> getSelectedObject() const { return m_selectedObject; }

        std::shared_ptr<BaseGameObject> pickObject(
            const std::vector<std::shared_ptr<BaseGameObject>>& objects,
            const SceneCamera& camera,
            float mouseX, float mouseY,
            ui32 viewportWidth, ui32 viewportHeight);

    private:
        bool rayIntersectsAABB(const Vector3& rayOrigin, const Vector3& rayDir,
            const Vector3& aabbMin, const Vector3& aabbMax, float& t);

    private:
        std::shared_ptr<BaseGameObject> m_selectedObject;
    };
}