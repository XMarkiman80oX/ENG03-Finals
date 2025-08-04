#include <DX3D/UI/Panels/InspectorUI.h>
#include <DX3D/UI/UIController.h>
#include <DX3D/Game/SelectionSystem.h>
#include <DX3D/Scene/SceneStateManager.h>
#include <DX3D/Graphics/Primitives/AGameObject.h>
#include <DX3D/Graphics/Primitives/LightObject.h>
#include <DX3D/Graphics/Primitives/CameraObject.h>
#include <DX3D/Graphics/Primitives/Cube.h>
#include <DX3D/Graphics/Primitives/Sphere.h>
#include <DX3D/Graphics/Primitives/Plane.h>
#include <DX3D/Graphics/Primitives/Cylinder.h>
#include <DX3D/Graphics/Primitives/Capsule.h>
#include <DX3D/ECS/ComponentManager.h>
#include <DX3D/ECS/Components/PhysicsComponent.h>
#include <DX3D/Graphics/ResourceManager.h>
#include <imgui.h>
#include <filesystem>

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

    
    float inspectorHeight = halfHeight * 0.45f; 
    float inspectorY = halfHeight + (halfHeight * 0.15f);

    ImGui::SetNextWindowPos(ImVec2(halfWidth, inspectorY));
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

    // Render material section for all objects
    ImGui::Separator();
    renderMaterialSection(selectedObject);

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

void InspectorUI::renderMaterialSection(std::shared_ptr<AGameObject> object)
{
    ImGui::Text("Material");

    // Skip material section for lights and cameras
    if (std::dynamic_pointer_cast<LightObject>(object) ||
        std::dynamic_pointer_cast<CameraObject>(object))
    {
        ImGui::Text("Materials not applicable to this object type");
        return;
    }

    bool hasMaterial = object->hasMaterial();
    ImGui::Text("Has Material: %s", hasMaterial ? "Yes" : "No");

    if (hasMaterial)
    {
        auto material = object->getMaterial();
        if (material)
        {
            ImGui::Text("Material Name: %s", material->getName().c_str());

            // Material properties
            Vector4 diffuseColor = material->getDiffuseColor();
            if (ImGui::ColorEdit4("Diffuse Color", &diffuseColor.x))
            {
                material->setDiffuseColor(diffuseColor);
            }

            Vector4 ambientColor = material->getAmbientColor();
            if (ImGui::ColorEdit4("Ambient Color", &ambientColor.x))
            {
                material->setAmbientColor(ambientColor);
            }

            Vector4 specularColor = material->getSpecularColor();
            if (ImGui::ColorEdit4("Specular Color", &specularColor.x))
            {
                material->setSpecularColor(specularColor);
            }

            float specularPower = material->getSpecularPower();
            if (ImGui::SliderFloat("Specular Power", &specularPower, 1.0f, 128.0f))
            {
                material->setSpecularPower(specularPower);
            }

            float opacity = material->getOpacity();
            if (ImGui::SliderFloat("Opacity", &opacity, 0.0f, 1.0f))
            {
                material->setOpacity(opacity);
            }

            // Texture section
            ImGui::Separator();
            ImGui::Text("Texture");

            bool hasTexture = material->hasDiffuseTexture();
            ImGui::Text("Has Texture: %s", hasTexture ? "Yes" : "No");

            if (hasTexture)
            {
                std::string textureName = object->getTextureName();
                ImGui::Text("Current Texture: %s", textureName.c_str());

                if (ImGui::Button("Remove Texture"))
                {
                    material->setDiffuseTexture(nullptr);
                }
            }

            // Texture selection dropdown
            ImGui::Text("Available Textures:");
            renderTextureSelector(object);
        }
    }
    else
    {
        if (ImGui::Button("Add Material"))
        {
            auto newMaterial = ResourceManager::getInstance().createMaterial("Material_" + object->getObjectType());
            object->attachMaterial(newMaterial);
        }
    }

    // Primitive Selection Area for Materials
    ImGui::Separator();
    renderPrimitiveSelector(object);
}

void InspectorUI::renderTextureSelector(std::shared_ptr<AGameObject> object)
{
    // Get list of available texture files
    std::vector<std::string> textureFiles;

    // Common texture paths to search
    std::vector<std::string> texturePaths = {
        "DX3D/Assets/Textures/",
        "Assets/Textures/",
        "Textures/"
    };

    // Scan for texture files
    for (const auto& path : texturePaths)
    {
        if (std::filesystem::exists(path))
        {
            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                if (entry.is_regular_file())
                {
                    std::string extension = entry.path().extension().string();
                    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

                    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
                        extension == ".bmp" || extension == ".tga" || extension == ".dds")
                    {
                        textureFiles.push_back(entry.path().filename().string());
                    }
                }
            }
        }
    }

    // Also add already loaded textures
    auto loadedTextures = ResourceManager::getInstance().getLoadedTextureNames();
    for (const auto& texture : loadedTextures)
    {
        if (std::find(textureFiles.begin(), textureFiles.end(), texture) == textureFiles.end())
        {
            textureFiles.push_back(texture);
        }
    }

    if (!textureFiles.empty())
    {
        static int selectedTextureIndex = 0;
        std::vector<const char*> textureNames;
        for (const auto& file : textureFiles)
        {
            textureNames.push_back(file.c_str());
        }

        ImGui::Combo("Select Texture", &selectedTextureIndex, textureNames.data(), static_cast<int>(textureNames.size()));

        ImGui::SameLine();
        if (ImGui::Button("Apply Texture"))
        {
            if (selectedTextureIndex >= 0 && selectedTextureIndex < textureFiles.size())
            {
                object->setTexture(textureFiles[selectedTextureIndex]);
            }
        }
    }
    else
    {
        ImGui::Text("No texture files found in common directories");
        ImGui::Text("Place textures in: DX3D/Assets/Textures/");
    }
}

void InspectorUI::renderPrimitiveSelector(std::shared_ptr<AGameObject> object)
{
    ImGui::Text("Primitive Material Operations");

    if (!object->hasMaterial())
    {
        ImGui::Text("Add a material first to use these operations");
        return;
    }

    auto material = object->getMaterial();
    if (!material)
        return;

    ImGui::Text("Current object: %s", object->getObjectType().c_str());

    // Quick material presets
    ImGui::Text("Material Presets:");

    if (ImGui::Button("Metal"))
    {
        material->setDiffuseColor(Vector4(0.7f, 0.7f, 0.8f, 1.0f));
        material->setAmbientColor(Vector4(0.1f, 0.1f, 0.1f, 1.0f));
        material->setSpecularColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        material->setSpecularPower(64.0f);
    }

    ImGui::SameLine();
    if (ImGui::Button("Plastic"))
    {
        material->setDiffuseColor(Vector4(0.8f, 0.2f, 0.2f, 1.0f));
        material->setAmbientColor(Vector4(0.2f, 0.05f, 0.05f, 1.0f));
        material->setSpecularColor(Vector4(0.5f, 0.5f, 0.5f, 1.0f));
        material->setSpecularPower(32.0f);
    }

    ImGui::SameLine();
    if (ImGui::Button("Rubber"))
    {
        material->setDiffuseColor(Vector4(0.3f, 0.3f, 0.3f, 1.0f));
        material->setAmbientColor(Vector4(0.1f, 0.1f, 0.1f, 1.0f));
        material->setSpecularColor(Vector4(0.1f, 0.1f, 0.1f, 1.0f));
        material->setSpecularPower(4.0f);
    }

    if (ImGui::Button("Gold"))
    {
        material->setDiffuseColor(Vector4(1.0f, 0.843f, 0.0f, 1.0f));
        material->setAmbientColor(Vector4(0.2f, 0.169f, 0.0f, 1.0f));
        material->setSpecularColor(Vector4(1.0f, 1.0f, 0.8f, 1.0f));
        material->setSpecularPower(128.0f);
    }

    ImGui::SameLine();
    if (ImGui::Button("Glass"))
    {
        material->setDiffuseColor(Vector4(0.9f, 0.9f, 1.0f, 0.3f));
        material->setAmbientColor(Vector4(0.1f, 0.1f, 0.1f, 0.3f));
        material->setSpecularColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        material->setSpecularPower(128.0f);
        material->setOpacity(0.3f);
    }

    // Material operations
    ImGui::Separator();
    ImGui::Text("Material Operations:");

    static char materialName[128] = "";
    ImGui::InputText("Material Name", materialName, sizeof(materialName));

    if (ImGui::Button("Save Material Settings"))
    {
        if (strlen(materialName) > 0)
        {
            material->setName(std::string(materialName));
        }
        // Here you could save the material settings to a file
        // For now, just update the name
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset to Default"))
    {
        material->setDiffuseColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        material->setAmbientColor(Vector4(0.2f, 0.2f, 0.2f, 1.0f));
        material->setSpecularColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        material->setSpecularPower(32.0f);
        material->setOpacity(1.0f);
    }

    // Display material statistics
    ImGui::Separator();
    ImGui::Text("Material Info:");
    ImGui::Text("Name: %s", material->getName().c_str());
    ImGui::Text("Has Diffuse Texture: %s", material->hasDiffuseTexture() ? "Yes" : "No");

    if (material->hasDiffuseTexture())
    {
        auto texture = material->getDiffuseTexture();
        ImGui::Text("Texture Size: %d x %d", texture->getWidth(), texture->getHeight());
        ImGui::Text("Texture Path: %s", texture->getFilePath().c_str());
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