#include <DX3D/Assets/ModelLoader.h>
#include <DX3D/Graphics/Texture2D.h>
#include <filesystem>

// Include TinyOBJ loader (header-only library)
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

using namespace dx3d;

// Static member initialization
std::unordered_map<std::string, std::shared_ptr<Material>> ModelLoader::s_materialCache;

std::shared_ptr<Model> ModelLoader::LoadModel(
    const std::string& filePath,
    const GraphicsResourceDesc& resourceDesc)
{
    DX3DLogInfo(("Loading model: " + filePath).c_str());

    auto model = std::make_shared<Model>();

    if (!loadOBJ(filePath, model, resourceDesc))
    {
        DX3DLogError(("Failed to load model: " + filePath).c_str());
        return nullptr;
    }

    // Set model properties
    model->setFilePath(filePath);
    std::string fileName = std::filesystem::path(filePath).filename().string();
    model->setName(fileName);

    DX3DLogInfo(("Model loaded successfully: " + filePath +
        " (" + std::to_string(model->getMeshCount()) + " meshes)").c_str());

    return model;
}

bool ModelLoader::loadOBJ(
    const std::string& filePath,
    std::shared_ptr<Model>& model,
    const GraphicsResourceDesc& resourceDesc)
{
    // Check if file exists
    std::string fullPath = getAssetPath(filePath);
    if (!std::filesystem::exists(fullPath))
    {
        DX3DLogError(("OBJ file not found: " + fullPath).c_str());
        return false;
    }

    // TinyOBJ data structures
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    // Get directory for relative paths
    std::string baseDir = getDirectory(fullPath);

    // Load OBJ file
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
        fullPath.c_str(), baseDir.c_str());

    if (!warn.empty())
    {
        DX3DLogWarning(("OBJ loader warning: " + warn).c_str());
    }

    if (!err.empty())
    {
        DX3DLogError(("OBJ loader error: " + err).c_str());
    }

    if (!ret)
    {
        return false;
    }

    // Load materials first
    std::vector<std::shared_ptr<Material>> loadedMaterials;
    for (const auto& material : materials)
    {
        auto mat = std::make_shared<Material>(material.name);

        // Set material colors
        mat->setDiffuseColor(Vector4(material.diffuse[0], material.diffuse[1], material.diffuse[2], 1.0f));
        mat->setAmbientColor(Vector4(material.ambient[0], material.ambient[1], material.ambient[2], 1.0f));
        mat->setSpecularColor(Vector4(material.specular[0], material.specular[1], material.specular[2], 1.0f));
        mat->setEmissiveColor(Vector4(material.emission[0], material.emission[1], material.emission[2], 1.0f));

        // Set material properties
        mat->setSpecularPower(material.shininess);
        mat->setOpacity(material.dissolve);

        // Load diffuse texture if specified
        if (!material.diffuse_texname.empty())
        {
            std::string texturePath = baseDir + "/" + material.diffuse_texname;
            if (std::filesystem::exists(texturePath))
            {
                try
                {
                    auto texture = std::make_shared<Texture2D>(texturePath, resourceDesc);
                    mat->setDiffuseTexture(texture);
                    DX3DLogInfo(("Loaded texture: " + material.diffuse_texname).c_str());
                }
                catch (const std::exception& e)
                {
                    DX3DLogError(("Failed to load texture " + material.diffuse_texname + ": " + e.what()).c_str());
                }
            }
            else
            {
                DX3DLogWarning(("Texture not found: " + texturePath).c_str());
            }
        }

        loadedMaterials.push_back(mat);
    }

    // Create default material if no materials were loaded
    if (loadedMaterials.empty())
    {
        loadedMaterials.push_back(std::make_shared<Material>("DefaultMaterial"));
    }

    // Process each shape (mesh)
    for (size_t s = 0; s < shapes.size(); s++)
    {
        const auto& shape = shapes[s];
        std::vector<Vertex> vertices;
        std::vector<ui32> indices;

        // Process faces
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
        {
            size_t fv = size_t(shape.mesh.num_face_vertices[f]);

            // Process vertices in face (should be 3 for triangles)
            for (size_t v = 0; v < fv; v++)
            {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                Vertex vertex;

                // Position (required)
                if (idx.vertex_index >= 0)
                {
                    vertex.position[0] = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                    vertex.position[1] = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                    vertex.position[2] = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
                }

                // Normal (optional)
                if (idx.normal_index >= 0)
                {
                    vertex.normal[0] = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    vertex.normal[1] = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    vertex.normal[2] = attrib.normals[3 * size_t(idx.normal_index) + 2];
                }
                else
                {
                    // Default normal pointing up
                    vertex.normal[0] = 0.0f;
                    vertex.normal[1] = 1.0f;
                    vertex.normal[2] = 0.0f;
                }

                // Texture coordinate (optional)
                if (idx.texcoord_index >= 0)
                {
                    vertex.texCoord[0] = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    vertex.texCoord[1] = 1.0f - attrib.texcoords[2 * size_t(idx.texcoord_index) + 1]; // Flip Y
                }
                else
                {
                    vertex.texCoord[0] = 0.0f;
                    vertex.texCoord[1] = 0.0f;
                }

                // Default color (white)
                vertex.color[0] = 1.0f;
                vertex.color[1] = 1.0f;
                vertex.color[2] = 1.0f;
                vertex.color[3] = 1.0f;

                vertices.push_back(vertex);
                indices.push_back(static_cast<ui32>(indices.size()));
            }

            index_offset += fv;
        }

        // Create mesh
        auto mesh = std::make_shared<Mesh>(shape.name.empty() ? ("Mesh_" + std::to_string(s)) : shape.name);

        // Create rendering resources
        mesh->createRenderingResources(vertices, indices, resourceDesc);

        // Assign material
        int materialId = shape.mesh.material_ids.empty() ? 0 : shape.mesh.material_ids[0];
        if (materialId >= 0 && materialId < static_cast<int>(loadedMaterials.size()))
        {
            mesh->setMaterial(loadedMaterials[materialId]);
        }
        else
        {
            mesh->setMaterial(loadedMaterials[0]); // Use default material
        }

        model->addMesh(mesh);
    }

    return true;
}

std::string ModelLoader::getDirectory(const std::string& filePath)
{
    return std::filesystem::path(filePath).parent_path().string();
}

std::string ModelLoader::getAssetPath(const std::string& relativePath)
{
    // Convert relative path to full path in Assets folder
    std::filesystem::path assetPath = std::filesystem::current_path() / "DX3D" / "Assets" / relativePath;
    return assetPath.string();
}