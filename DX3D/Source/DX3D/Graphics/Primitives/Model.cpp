#include <DX3D/Graphics/Primitives/Model.h>
#include <DX3D/Assets/ModelLoader.h>
#include <algorithm>

using namespace dx3d;

Model::Model() : AGameObject(), m_name("Model"), m_filePath("")
{
}

Model::Model(const Vector3& position, const Vector3& rotation, const Vector3& scale)
    : AGameObject(position, rotation, scale), m_name("Model"), m_filePath("")
{
}

void Model::addMesh(std::shared_ptr<Mesh> mesh)
{
    if (mesh)
    {
        m_meshes.push_back(mesh);
        DX3DLogInfo(("Added mesh '" + mesh->getName() + "' to model '" + m_name + "'").c_str());
    }
}

void Model::removeMesh(size_t index)
{
    if (index < m_meshes.size())
    {
        auto meshName = m_meshes[index]->getName();
        m_meshes.erase(m_meshes.begin() + index);
        DX3DLogInfo(("Removed mesh '" + meshName + "' from model '" + m_name + "'").c_str());
    }
}

void Model::clearMeshes()
{
    m_meshes.clear();
    DX3DLogInfo(("Cleared all meshes from model '" + m_name + "'").c_str());
}

std::shared_ptr<Mesh> Model::getMesh(size_t index) const
{
    if (index < m_meshes.size())
    {
        return m_meshes[index];
    }
    return nullptr;
}

bool Model::isReadyForRendering() const
{
    if (m_meshes.empty())
        return false;

    // Check if all meshes are ready for rendering
    return std::all_of(m_meshes.begin(), m_meshes.end(),
        [](const std::shared_ptr<Mesh>& mesh) {
            return mesh && mesh->isReadyForRendering();
        });
}

void Model::update(float deltaTime)
{
    // Call base class update
    AGameObject::update(deltaTime);

    // Add any model-specific update logic here if needed
}

std::shared_ptr<Model> Model::LoadFromFile(
    const std::string& filePath,
    const GraphicsResourceDesc& resourceDesc)
{
    return ModelLoader::LoadModel(filePath, resourceDesc);
}