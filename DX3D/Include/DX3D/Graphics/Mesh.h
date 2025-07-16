#pragma once
#include <DX3D/Graphics/VertexBuffer.h>
#include <DX3D/Graphics/IndexBuffer.h>
#include <DX3D/Graphics/Material.h>
#include <DX3D/Graphics/Vertex.h>
#include <memory>
#include <vector>
#include <string>

namespace dx3d
{
    class Mesh
    {
    public:
        Mesh(const std::string& name = "");
        ~Mesh() = default;

        // Create rendering resources from vertex/index data
        void createRenderingResources(
            const std::vector<Vertex>& vertices,
            const std::vector<ui32>& indices,
            const GraphicsResourceDesc& resourceDesc
        );

        // Getters
        std::shared_ptr<VertexBuffer> getVertexBuffer() const { return m_vertexBuffer; }
        std::shared_ptr<IndexBuffer> getIndexBuffer() const { return m_indexBuffer; }
        std::shared_ptr<Material> getMaterial() const { return m_material; }

        ui32 getIndexCount() const { return m_indexCount; }
        const std::string& getName() const { return m_name; }

        // Setters
        void setMaterial(std::shared_ptr<Material> material) { m_material = material; }
        void setName(const std::string& name) { m_name = name; }

        // Check if mesh is ready for rendering
        bool isReadyForRendering() const;

    private:
        std::string m_name;
        std::shared_ptr<VertexBuffer> m_vertexBuffer;
        std::shared_ptr<IndexBuffer> m_indexBuffer;
        std::shared_ptr<Material> m_material;
        ui32 m_indexCount;
    };
}