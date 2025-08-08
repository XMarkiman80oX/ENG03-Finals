#include <../UI/Panels/SceneOutlinerUI.h>
#include <../UI/UIController.h>
#include <../Game/SelectionSystem.h>
#include <../Scene/SceneStateManager.h>
#include <../Game/UndoRedoSystem.h>
#include <../Graphics/Primitives/AGameObject.h>
#include <../Graphics/Primitives/Cube.h>
#include <../Graphics/Primitives/Plane.h>
#include <../Graphics/Primitives/Sphere.h>
#include <../Graphics/Primitives/Cylinder.h>
#include <../Graphics/Primitives/Capsule.h>
#include <../Graphics/Primitives/CameraObject.h>
#include <../Graphics/Primitives/LightObject.h>
#include <imgui.h>
#include <imgui_internal.h>

using namespace dx3d;

SceneOutlinerUI::SceneOutlinerUI(
    UIController& controller,
    SelectionSystem& selectionSystem,
    SceneStateManager& sceneStateManager,
    UndoRedoSystem& undoRedoSystem,
    const std::vector<std::shared_ptr<BaseGameObject>>& gameObjects)
    : m_controller(controller)
    , m_selectionSystem(selectionSystem)
    , m_sceneStateManager(sceneStateManager)
    , m_undoRedoSystem(undoRedoSystem)
    , m_gameObjects(gameObjects)
{
}

void SceneOutlinerUI::render(float deltaTime)
{
    ImGuiIO& io = ImGui::GetIO();
    float windowWidth = io.DisplaySize.x;
    float windowHeight = io.DisplaySize.y;
    float leftPanelWidth = windowWidth / 4.0f;
    float topBarHeight = 70;
    float panelHeight = 100; // Give it a fixed height

    ImGui::SetNextWindowPos(ImVec2(0, topBarHeight));
    ImGui::SetNextWindowSize(ImVec2(leftPanelWidth, panelHeight));
    ImGui::Begin("Scene Stats", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Objects: %zu", m_gameObjects.size());
    ImGui::Text("Delta Time: %.3f ms", deltaTime * 1000.0f);
    ImGui::Text("FPS: %.1f", 1.0f / deltaTime);

    if (m_sceneStateManager.isEditMode())
    {
        ImGui::Text("Undo Stack: %d", m_undoRedoSystem.getUndoCount());
        ImGui::Text("Redo Stack: %d", m_undoRedoSystem.getRedoCount());
    }

    ImGui::End();
}

void SceneOutlinerUI::renderHierarchy()
{
    std::vector<std::shared_ptr<BaseGameObject>> rootObjects;
    for (const auto& obj : m_gameObjects)
    {
        if (!obj->hasParent())
        {
            rootObjects.push_back(obj);
        }
    }

    int nodeIndex = 0;
    for (const auto& rootObj : rootObjects)
    {
        renderObjectNode(rootObj, nodeIndex);
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAME_OBJECT"))
        {
            std::shared_ptr<BaseGameObject>* droppedObj = (std::shared_ptr<BaseGameObject>*)payload->Data;
            if (*droppedObj && m_draggedObject)
            {
                m_draggedObject->removeParent();
                m_draggedObject = nullptr;
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void SceneOutlinerUI::renderHierarchyWindow()
{
    ImGui::Begin("Scene Hierarchy"); // Creates a floating window

    ImGui::BeginChild("Outliner", ImVec2(0, 0), true);

    renderHierarchy();

    ImGui::EndChild();
    ImGui::End();
}

void SceneOutlinerUI::renderObjectNode(std::shared_ptr<BaseGameObject> object, int& nodeIndex)
{
    if (!object)
        return;

    ImGui::PushID(nodeIndex++);

    bool isEnabled = object->isEnabled();
    bool isSelected = (object == m_selectionSystem.getSelectedObject());

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (isSelected)
        nodeFlags |= ImGuiTreeNodeFlags_Selected;

    if (!object->hasChildren())
        nodeFlags |= ImGuiTreeNodeFlags_Leaf;

    bool isPrimitive = (std::dynamic_pointer_cast<Cube>(object) ||
        std::dynamic_pointer_cast<Sphere>(object) ||
        std::dynamic_pointer_cast<Plane>(object) ||
        std::dynamic_pointer_cast<Cylinder>(object) ||
        std::dynamic_pointer_cast<Capsule>(object));

    if (!isEnabled)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.1f, 0.1f, 1.0f));
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0.1f, 1.0f));
    }

    const char* buttonLabel = isEnabled ? "E" : "D";
    if (ImGui::SmallButton(buttonLabel))
    {
        object->setEnabled(!isEnabled);
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    std::string nodeName = getObjectDisplayName(object, object->getEntity().getID());

    if (!isEnabled)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    }

    if (object->getChildren().empty())
    {
        nodeFlags |= ImGuiTreeNodeFlags_Leaf;
    }

    bool nodeOpen = ImGui::TreeNodeEx(nodeName.c_str(), nodeFlags);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        m_controller.onObjectSelected(object);
    }

    if (isPrimitive && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        ImGui::SetDragDropPayload("GAME_OBJECT", &object, sizeof(std::shared_ptr<BaseGameObject>));
        ImGui::Text("Dragging %s", nodeName.c_str());
        m_draggedObject = object;
        ImGui::EndDragDropSource();
    }

    if (isPrimitive && ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAME_OBJECT"))
        {
            std::shared_ptr<BaseGameObject>* droppedObj = (std::shared_ptr<BaseGameObject>*)payload->Data;

            if (*droppedObj && m_draggedObject && m_draggedObject != object)
            {
                bool canParent = true;
                std::shared_ptr<BaseGameObject> checkParent = object;
                while (checkParent)
                {
                    if (checkParent == m_draggedObject)
                    {
                        canParent = false;
                        break;
                    }
                    checkParent = checkParent->getParent();
                }

                if (canParent)
                {
                    m_controller.onParentChanged(m_draggedObject, m_draggedObject->getParent(), object);
                    m_draggedObject = nullptr;
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (!isEnabled)
    {
        ImGui::PopStyleColor();
    }

    if (nodeOpen)
    {
        int validChildCount = 0;
        for (auto& weakChild : object->getChildren())
        {
            if (auto child = weakChild.lock())
            {
                validChildCount++;
                renderObjectNode(child, nodeIndex);
            }
        }

        if (!object->getChildren().empty() && validChildCount == 0)
        {
            ImGui::TextDisabled("  (Children are invalid or destroyed)");
        }

        ImGui::TreePop();
    }

    ImGui::PopID();


}

std::string SceneOutlinerUI::getObjectDisplayName(std::shared_ptr<BaseGameObject> object, int index)
{
    std::string objectName = "Object";
    if (std::dynamic_pointer_cast<Cube>(object)) objectName = "Cube";
    else if (std::dynamic_pointer_cast<Plane>(object)) objectName = "Plane";
    else if (std::dynamic_pointer_cast<Sphere>(object)) objectName = "Sphere";
    else if (std::dynamic_pointer_cast<Cylinder>(object)) objectName = "Cylinder";
    else if (std::dynamic_pointer_cast<Capsule>(object)) objectName = "Capsule";
    else if (std::dynamic_pointer_cast<CameraObject>(object)) objectName = "Game Camera";
    else if (auto light = std::dynamic_pointer_cast<LightObject>(object))
    {
        switch (light->getLightData().type)
        {
        case LIGHT_TYPE_DIRECTIONAL: objectName = "Directional Light"; break;
        case LIGHT_TYPE_POINT:       objectName = "Point Light"; break;
        case LIGHT_TYPE_SPOT:        objectName = "Spot Light"; break;
        }
    }

    return objectName + " " + std::to_string(index);
}

std::string SceneOutlinerUI::getObjectIcon(std::shared_ptr<BaseGameObject> object)
{
    if (std::dynamic_pointer_cast<Cube>(object)) return "[C]";
    else if (std::dynamic_pointer_cast<Plane>(object)) return "[P]";
    else if (std::dynamic_pointer_cast<Sphere>(object)) return "[S]";
    else if (std::dynamic_pointer_cast<Cylinder>(object)) return "[Y]";
    else if (std::dynamic_pointer_cast<Capsule>(object)) return "[A]";
    else if (std::dynamic_pointer_cast<CameraObject>(object)) return "[CAM]";
    return "[?]";
}