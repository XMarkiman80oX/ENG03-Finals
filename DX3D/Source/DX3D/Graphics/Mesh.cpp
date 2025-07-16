#include <DX3D/Graphics/Mesh.h>

using namespace dx3d;

Mesh::Mesh(const std::string& name)
    : m_name(name)
    , m_vertexBuffer(nullptr)
    , m_indexBuffer(nullptr)
    , m_material(std::make_shared<Material>())
    , m_indexCount(0)
{
}

void Mesh::createRenderingResources(
    const std::vector<Vertex>& vertices,
    const std::vector<ui32>& indices,
    const GraphicsResourceDesc& resourceDesc)
{
    if (vertices.empty() || indices.empty())
    {
        return;
    }

    // Create vertex buffer
    m_vertexBuffer = std::make_shared<VertexBuffer>(
        vertices.data(),
        sizeof(Vertex),
        static_cast<ui32>(vertices.size()),
        resourceDesc
    );

    // Create index buffer
    m_indexBuffer = std::make_shared<IndexBuffer>(
        indices.data(),
        static_cast<ui32>(indices.size()),
        resourceDesc
    );

    m_indexCount = static_cast<ui32>(indices.size());
}

bool Mesh::isReadyForRendering() const
{
    return m_vertexBuffer != nullptr &&
        m_indexBuffer != nullptr &&
        m_material != nullptr &&
        m_indexCount > 0;
}