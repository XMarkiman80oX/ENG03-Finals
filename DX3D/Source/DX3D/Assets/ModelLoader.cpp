#include <DX3D/Assets/ModelLoader.h>
#include <DX3D/Graphics/Texture2D.h>
#include <fstream>
#include <locale.h>
#include <string>

#define TINYOBJLOADER_IMPLEMENTATION
#include <DX3D/Assets/tiny_obj_loader.h>

using namespace dx3d;

std::unordered_map<std::string, std::shared_ptr<Material>> ModelLoader::s_materialCache;

// Helper function to get the directory from a file path
std::string ModelLoader::getDirectory(const std::string& filePath) {
    size_t last_slash_idx = filePath.rfind('/');
    if (std::string::npos != last_slash_idx) {
        return filePath.substr(0, last_slash_idx + 1);
    }
    last_slash_idx = filePath.rfind('\\');
    if (std::string::npos != last_slash_idx) {
        return filePath.substr(0, last_slash_idx + 1);
    }
    return "";
}

// Helper function to get the application's asset path
std::string ModelLoader::getAssetPath(const std::string& relativePath)
{
    // This path is now relative to $(SolutionDir)
    return "DX3D/Assets/Models" + relativePath;
}

std::shared_ptr<Model> ModelLoader::LoadModel(
    const std::string& filePath,
    const GraphicsResourceDesc& resourceDesc)
{
    auto model = std::make_shared<Model>();

    if (!loadOBJ(filePath, model, resourceDesc))
    {
        return nullptr;
    }

    model->setFilePath(filePath);

    // Get file name from path
    size_t last_slash_idx = filePath.rfind('/');
    if (std::string::npos != last_slash_idx) {
        model->setName(filePath.substr(last_slash_idx + 1));
    }
    else {
        model->setName(filePath);
    }

    return model;
}

bool ModelLoader::loadOBJ(
    const std::string& filePath,
    std::shared_ptr<Model>& model,
    const GraphicsResourceDesc& resourceDesc)
{
    // We construct the full path from the project root (the solution directory)
    std::string fullPath = getAssetPath(filePath);

    // --- ROBUST LOCALE HANDLING ---
    char* old_locale = setlocale(LC_NUMERIC, nullptr);
    std::string old_locale_str = old_locale ? old_locale : "";
    setlocale(LC_NUMERIC, "C");
    // --- END OF LOCALE HANDLING ---

    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = getDirectory(fullPath).c_str();
    reader_config.triangulate = true;

    tinyobj::ObjReader reader;

    // Final check. If this fails, the path is definitively wrong or the file is locked.
    std::ifstream file_check(fullPath);
    if (!file_check.good()) {
        // If you hit this, the `fullPath` variable is incorrect.
        // The most likely cause is the Working Directory setting in Visual Studio.
        file_check.close();
        if (!old_locale_str.empty()) setlocale(LC_NUMERIC, old_locale_str.c_str());
        return false;
    }
    file_check.close();

    if (!reader.ParseFromFile(fullPath, reader_config)) {
        if (!old_locale_str.empty()) setlocale(LC_NUMERIC, old_locale_str.c_str());
        return false;
    }

    // Restore the locale after parsing is complete
    if (!old_locale_str.empty()) {
        setlocale(LC_NUMERIC, old_locale_str.c_str());
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();


    std::vector<std::shared_ptr<Material>> loadedMaterials;
    for (const auto& material : materials)
    {
        auto mat = std::make_shared<Material>(material.name);

        mat->setDiffuseColor(Vector4(material.diffuse[0], material.diffuse[1], material.diffuse[2], 1.0f));
        mat->setAmbientColor(Vector4(material.ambient[0], material.ambient[1], material.ambient[2], 1.0f));
        mat->setSpecularColor(Vector4(material.specular[0], material.specular[1], material.specular[2], 1.0f));
        mat->setEmissiveColor(Vector4(material.emission[0], material.emission[1], material.emission[2], 1.0f));
        mat->setSpecularPower(material.shininess);
        mat->setOpacity(material.dissolve);

        if (!material.diffuse_texname.empty())
        {
            std::string texturePath = getDirectory(fullPath) + material.diffuse_texname;
            std::ifstream texture_check(texturePath);
            if (texture_check.good()) {
                try
                {
                    auto texture = std::make_shared<Texture2D>(texturePath, resourceDesc);
                    mat->setDiffuseTexture(texture);
                }
                catch (const std::exception&) { /* Handle error */ }
            }
            texture_check.close();
        }
        loadedMaterials.push_back(mat);
    }

    if (loadedMaterials.empty())
    {
        loadedMaterials.push_back(std::make_shared<Material>("DefaultMaterial"));
    }

    for (size_t s = 0; s < shapes.size(); s++)
    {
        const auto& shape = shapes[s];
        std::vector<Vertex> vertices;
        std::vector<ui32> indices;
        size_t index_offset = 0;

        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
        {
            size_t fv = size_t(shape.mesh.num_face_vertices[f]);
            for (size_t v = 0; v < fv; v++)
            {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                Vertex vertex;

                if (idx.vertex_index >= 0)
                {
                    vertex.position.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                    vertex.position.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                    vertex.position.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
                }

                if (idx.normal_index >= 0)
                {
                    vertex.normal.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    vertex.normal.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    vertex.normal.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
                }
                else
                {
                    vertex.normal = { 0.0f, 1.0f, 0.0f };
                }

                if (idx.texcoord_index >= 0)
                {
                    vertex.texCoord.x = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    vertex.texCoord.y = 1.0f - attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                }
                else
                {
                    vertex.texCoord = { 0.0f, 0.0f };
                }

                vertex.color = { 1.0f, 1.0f, 1.0f, 1.0f };

                vertices.push_back(vertex);
                indices.push_back(static_cast<ui32>(indices.size()));
            }
            index_offset += fv;
        }

        auto mesh = std::make_shared<Mesh>(shape.name.empty() ? ("Mesh_" + std::to_string(s)) : shape.name);
        mesh->createRenderingResources(vertices, indices, resourceDesc);

        int materialId = shape.mesh.material_ids.empty() ? -1 : shape.mesh.material_ids[0];
        if (materialId >= 0 && materialId < static_cast<int>(loadedMaterials.size()))
        {
            mesh->setMaterial(loadedMaterials[materialId]);
        }
        else
        {
            if (!loadedMaterials.empty()) {
                mesh->setMaterial(loadedMaterials[0]);
            }
        }

        model->addMesh(mesh);
    }

    return true;
}