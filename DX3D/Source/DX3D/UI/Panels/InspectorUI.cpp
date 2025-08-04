#include <DX3D/UI/Panels/InspectorUI.h>
#include <DX3D/UI/UIController.h>
#include <DX3D/Game/SelectionSystem.h>
#include <DX3D/Scene/SceneStateManager.h>
#include <DX3D/Graphics/Primitives/AGameObject.h>
#include <DX3D/Graphics/Primitives/LightObject.h>
#include <DX3D/Graphics/Primitives/CameraObject.h>
#include <DX3D/ECS/ComponentManager.h>
#include <DX3D/ECS/Components/PhysicsComponent.h>
#include <imgui.h>

using namespace dx3d;

InspectorUI::InspectorUI(
    UIController& controller,
    SelectionSystem& selectionSystem,
    SceneStateManager& sceneStateManager)
    : m_controller(controller)
    , m_selectionSystem(selectionSystem)
    , m_sceneStateManager(sceneStateManager)
{
}

void InspectorUI::render()
{
    ImGuiIO& io = ImGui::GetIO();
    float windowWidth = io.DisplaySize.x;
    float windowHeight = io.DisplaySize.y;
    float halfWidth = windowWidth * 0.5f;
    float halfHeight = windowHeight * 0.5f;
    float inspectorHeight = halfHeight * 0.6f;

    ImGui::SetNextWindowPos(ImVec2(halfWidth, windowHeight - inspectorHeight));
    ImGui::SetNextWindowSize(ImVec2(halfWidth, inspectorHeight));
    ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    auto selectedObject = m_selectionSystem.getSelectedObject();
    if (!selectedObject)
    {
        ImGui::Text("No object selected");
        ImGui::End();
        return;
    }

    renderObjectInfo(selectedObject);
    ImGui::Separator();
    renderTransform(selectedObject);

    if (auto camera = std::dynamic_pointer_cast<CameraObject>(selectedObject))
    {
        ImGui::Separator();
        renderCamera(selectedObject);
    }

    if (auto light = std::dynamic_pointer_cast<LightObject>(selectedObject))
    {
        ImGui::Separator();
        renderLight(light);
    }

    if (selectedObject->hasPhysics())
    {
        ImGui::Separator();
        renderPhysics(selectedObject);
    }

    ImGui::End();
}

void InspectorUI::renderObjectInfo(std::shared_ptr<AGameObject> object)
{
    ImGui::Text("Object Info");
    ImGui::Text("Type: %s", object->getObjectType().c_str());
    ImGui::Text("Entity ID: %u", object->getEntity().getID());

    bool enabled = object->isEnabled();
    if (ImGui::Checkbox("Enabled", &enabled))
    {
        object->setEnabled(enabled);
    }
}

void InspectorUI::renderTransform(std::shared_ptr<AGameObject> object)
{
    if (!m_sceneStateManager.isEditMode())
    {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
    }

    ImGui::Text("Transform");

    Vector3 position = object->getPosition();
    Vector3 rotation = object->getRotation();
    Vector3 scale = object->getScale();

    Vector3 oldPos = position;
    Vector3 oldRot = rotation;
    Vector3 oldScale = scale;

    bool transformChanged = false;

    if (ImGui::DragFloat3("Position", &position.x, 0.1f))
    {
        object->setPosition(position);
        transformChanged = true;
    }

    // Convert radians to degrees for display
    Vector3 rotationDegrees = { rotation.x * 180.0f / 3.14159f,
                               rotation.y * 180.0f / 3.14159f,
                               rotation.z * 180.0f / 3.14159f };

    if (ImGui::DragFloat3("Rotation", &rotationDegrees.x, 1.0f))
    {
        // Convert back to radians
        rotation = { rotationDegrees.x * 3.14159f / 180.0f,
                    rotationDegrees.y * 3.14159f / 180.0f,
                    rotationDegrees.z * 3.14159f / 180.0f };
        object->setRotation(rotation);
        transformChanged = true;
    }

    if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.01f, 100.0f))
    {
        object->setScale(scale);
        transformChanged = true;
    }

    if (transformChanged && m_sceneStateManager.isEditMode())
    {
        m_controller.onTransformChanged(object, oldPos, position, oldRot, rotation, oldScale, scale);
    }

    if (!m_sceneStateManager.isEditMode())
    {
        ImGui::PopStyleVar();
    }
}

void InspectorUI::renderCamera(std::shared_ptr<AGameObject> object)
{
    auto camera = std::dynamic_pointer_cast<CameraObject>(object);
    if (!camera) return;

    ImGui::Text("Camera");

    float fov = camera->getFOV() * 180.0f / 3.14159f; // Convert to degrees
    if (ImGui::SliderFloat("FOV", &fov, 30.0f, 120.0f))
    {
        camera->setFOV(fov * 3.14159f / 180.0f); // Convert back to radians
    }

    float nearPlane = camera->getNearPlane();
    if (ImGui::DragFloat("Near Plane", &nearPlane, 0.01f, 0.01f, 10.0f))
    {
        camera->setNearPlane(nearPlane);
    }

    float farPlane = camera->getFarPlane();
    if (ImGui::DragFloat("Far Plane", &farPlane, 1.0f, 10.0f, 1000.0f))
    {
        camera->setFarPlane(farPlane);
    }
}

void InspectorUI::renderPhysics(std::shared_ptr<AGameObject> object)
{
    ImGui::Text("Physics");

    auto& componentManager = ComponentManager::getInstance();
    auto* physicsComp = componentManager.getComponent<PhysicsComponent>(object->getEntity().getID());

    if (!physicsComp) return;

    // Display physics properties
    ImGui::Text("Body Type: %s",
        physicsComp->bodyType == PhysicsBodyType::Static ? "Static" :
        physicsComp->bodyType == PhysicsBodyType::Kinematic ? "Kinematic" : "Dynamic");

    if (physicsComp->bodyType == PhysicsBodyType::Dynamic)
    {
        float mass = physicsComp->mass;
        if (ImGui::DragFloat("Mass", &mass, 0.1f, 0.1f, 100.0f))
        {
            object->setPhysicsMass(mass);
        }

        Vector3 velocity = object->getLinearVelocity();
        ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", velocity.x, velocity.y, velocity.z);
    }

    float restitution = physicsComp->restitution;
    if (ImGui::SliderFloat("Restitution", &restitution, 0.0f, 1.0f))
    {
        object->setPhysicsRestitution(restitution);
    }

    float friction = physicsComp->friction;
    if (ImGui::SliderFloat("Friction", &friction, 0.0f, 1.0f))
    {
        object->setPhysicsFriction(friction);
    }
}

void InspectorUI::renderLight(std::shared_ptr<LightObject> lightObject)
{
    ImGui::Text("Light");

    Light& lightData = lightObject->getLightData();

    const char* lightTypes[] = { "Directional", "Point", "Spot" };
    int currentType = lightData.type;
    if (ImGui::Combo("Type", &currentType, lightTypes, 3))
    {
        lightData.type = currentType;
    }

    ImGui::ColorEdit3("Color", &lightData.color.x);
    ImGui::DragFloat("Intensity", &lightData.intensity, 0.1f, 0.0f, 10.0f);

    if (lightData.type == LIGHT_TYPE_POINT || lightData.type == LIGHT_TYPE_SPOT)
    {
        ImGui::DragFloat("Radius", &lightData.radius, 1.0f, 1.0f, 100.0f);
    }

    if (lightData.type == LIGHT_TYPE_SPOT)
    {
        float innerAngle = lightData.spot_angle_inner * 180.0f / 3.14159f;
        float outerAngle = lightData.spot_angle_outer * 180.0f / 3.14159f;

        if (ImGui::SliderFloat("Inner Angle", &innerAngle, 0.0f, 90.0f))
        {
            lightData.spot_angle_inner = innerAngle * 3.14159f / 180.0f;
        }

        if (ImGui::SliderFloat("Outer Angle", &outerAngle, 0.0f, 90.0f))
        {
            lightData.spot_angle_outer = outerAngle * 3.14159f / 180.0f;
        }

        ImGui::DragFloat("Falloff", &lightData.spot_falloff, 0.1f, 0.1f, 5.0f);
    }
}