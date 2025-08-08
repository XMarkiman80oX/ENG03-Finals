#include "../Assets/ModelParser.h"
#include "../Graphics/Texture2D.h"
#include <fstream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "../Assets/tiny_obj_loader.h"

using namespace dx3d;

std::unordered_map<std::string, std::shared_ptr<Material>> ModelParser::m_materialRegistry;

std::shared_ptr<Model> ModelParser::LoadModel(
    const std::string& path,
    const GraphicsResourceDesc& resDesc)
{
    try {
        auto newModel = std::make_shared<Model>();

        if (!parseOBJ(path, newModel, resDesc)) {
            return generateDefaultModel(resDesc);
        }

        newModel->setFilePath(path);
        newModel->setName(path);
        return newModel;
    }
    catch (const std::exception& ex) {
        return generateDefaultModel(resDesc);
    }
    catch (...) {
        return generateDefaultModel(resDesc);
    }
}

std::string ModelParser::extractDirectory(const std::string& path) {
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        return path.substr(0, lastSlash + 1);
    }
    return "";
}

std::string ModelParser::findAssetPath(const std::string& path)
{
    std::vector<std::string> searchPaths = {
        "../Assets/Models/" + path,
        "GDENG03-Engine/../Assets/Models/" + path,
        "../Assets/Models/" + path,
        "../../Assets/Models/" + path
    };

    for (const auto& searchPath : searchPaths) {
        std::ifstream fileStream(searchPath);
        if (fileStream.good()) {
            fileStream.close();
            return searchPath;
        }
    }

    return "../Assets/Models/" + path;
}

std::shared_ptr<Material> ModelParser::retrieveMaterial(
    const std::string& name,
    const std::string& file,
    const std::string& directory,
    const GraphicsResourceDesc& resDesc)
{
    std::string key = directory + name;
    auto iter = m_materialRegistry.find(key);
    if (iter != m_materialRegistry.end()) {
        return iter->second;
    }

    auto newMaterial = std::make_shared<Material>(name);
    std::string materialPath = directory + file;
    std::ifstream materialStream(materialPath);

    if (materialStream.is_open()) {
        std::string currentLine;
        bool materialFound = false;

        while (std::getline(materialStream, currentLine)) {
            if (currentLine.empty() || currentLine[0] == '#') continue;

            std::istringstream stream(currentLine);
            std::string token;
            stream >> token;

            if (token == "newmtl") {
                std::string materialName;
                stream >> materialName;
                materialFound = (materialName == name);
            }
            else if (materialFound) {
                if (token == "Ka" || token == "Kd" || token == "Ks" || token == "Ke") {
                    float val1, val2, val3;
                    stream >> val1 >> val2 >> val3;
                    if (token == "Ka") newMaterial->setAmbientColor({ val1, val2, val3, 1.0f });
                    if (token == "Kd") newMaterial->setDiffuseColor({ val1, val2, val3, 1.0f });
                    if (token == "Ks") newMaterial->setSpecularColor({ val1, val2, val3, 1.0f });
                    if (token == "Ke") newMaterial->setEmissiveColor({ val1, val2, val3, 1.0f });
                }
                else if (token == "Ns") {
                    float power;
                    stream >> power;
                    newMaterial->setSpecularPower(power);
                }
                else if (token == "d" || token == "Tr") {
                    float value;
                    stream >> value;
                    if (token == "Tr") value = 1.0f - value;
                    newMaterial->setOpacity(value);
                }
                else if (token == "map_Kd") {
                    std::string path;
                    stream >> path;

                    std::vector<std::string> searchPaths = {
                        directory + path,
                        directory + "../Textures/" + path,
                        "../Assets/Textures/" + path,
                        "../Assets/Models/Textures/" + path
                    };

                    for (const auto& texPath : searchPaths) {
                        try {
                            auto tex = std::make_shared<Texture2D>(texPath, resDesc);
                            newMaterial->setDiffuseTexture(tex);
                            break;
                        }
                        catch (const std::exception&) {
                            continue;
                        }
                    }
                }
            }
        }
        materialStream.close();
    }

    m_materialRegistry[key] = newMaterial;
    return newMaterial;
}

std::shared_ptr<Model> ModelParser::generateDefaultModel(const GraphicsResourceDesc& resDesc)
{
    auto newModel = std::make_shared<Model>();
    newModel->setName("DefaultCubeModel");

    std::vector<Vertex> vertData = {
        { {-0.5f, -0.5f, -0.5f}, {1.f, 0.f, 0.f, 1.f}, {0.f, 0.f, -1.f}, {0.f, 1.f} },
        { {0.5f, -0.5f, -0.5f}, {0.f, 1.f, 0.f, 1.f}, {0.f, 0.f, -1.f}, {1.f, 1.f} },
        { {0.5f, 0.5f, -0.5f}, {0.f, 0.f, 1.f, 1.f}, {0.f, 0.f, -1.f}, {1.f, 0.f} },
        { {-0.5f, 0.5f, -0.5f}, {1.f, 1.f, 0.f, 1.f}, {0.f, 0.f, -1.f}, {0.f, 0.f} },
        { {-0.5f, -0.5f, 0.5f}, {1.f, 0.f, 1.f, 1.f}, {0.f, 0.f, 1.f}, {0.f, 1.f} },
        { {0.5f, -0.5f, 0.5f}, {0.f, 1.f, 1.f, 1.f}, {0.f, 0.f, 1.f}, {1.f, 1.f} },
        { {0.5f, 0.5f, 0.5f}, {1.f, 1.f, 1.f, 1.f}, {0.f, 0.f, 1.f}, {1.f, 0.f} },
        { {-0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f, 1.f}, {0.f, 0.f, 1.f}, {0.f, 0.f} }
    };

    std::vector<ui32> idxData = {
        0, 1, 2, 0, 2, 3, 4, 6, 5, 4, 7, 6, 0, 3, 7, 0, 7, 4,
        1, 5, 6, 1, 6, 2, 3, 2, 6, 3, 6, 7, 0, 4, 5, 0, 5, 1
    };

    try {
        auto subMesh = std::make_shared<Mesh>("DefaultCubeMesh");
        subMesh->createRenderingResources(vertData, idxData, resDesc);
        auto defaultMaterial = std::make_shared<Material>("DefaultMaterial");
        defaultMaterial->setDiffuseColor({ 0.7f, 0.7f, 0.7f, 1.0f });
        subMesh->setMaterial(defaultMaterial);
        newModel->addMesh(subMesh);
    }
    catch (...) {
        // return empty model
    }

    return newModel;
}

bool ModelParser::parseOBJ(
    const std::string& path,
    std::shared_ptr<Model>& asset,
    const GraphicsResourceDesc& resDesc)
{
    try {
        std::string absolutePath = findAssetPath(path);
        std::string parentDirectory = extractDirectory(absolutePath);

        std::ifstream fileCheck(absolutePath);
        if (!fileCheck.good()) {
            return false;
        }
        fileCheck.close();

        tinyobj::ObjReaderConfig readerConfig;
        readerConfig.triangulate = true;

        tinyobj::ObjReader objReader;

        if (!objReader.ParseFromFile(absolutePath, readerConfig)) {
            return false;
        }

        auto& attributes = objReader.GetAttrib();
        auto& geometry = objReader.GetShapes();
        auto& materialData = objReader.GetMaterials();

        if (geometry.empty()) {
            return false;
        }

        std::vector<std::shared_ptr<Material>> materials;
        for (const auto& mat : materialData) {
            try {
                auto newMaterial = std::make_shared<Material>(mat.name);
                newMaterial->setDiffuseColor({ mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1.0f });
                newMaterial->setAmbientColor({ mat.ambient[0], mat.ambient[1], mat.ambient[2], 1.0f });
                newMaterial->setSpecularColor({ mat.specular[0], mat.specular[1], mat.specular[2], 1.0f });
                newMaterial->setEmissiveColor({ mat.emission[0], mat.emission[1], mat.emission[2], 1.0f });
                newMaterial->setSpecularPower(mat.shininess);
                newMaterial->setOpacity(mat.dissolve);

                if (!mat.diffuse_texname.empty()) {
                    std::vector<std::string> searchPaths = {
                       parentDirectory + mat.diffuse_texname,
                       parentDirectory + "../Textures/" + mat.diffuse_texname,
                       "../Assets/Textures/" + mat.diffuse_texname,
                       "../Assets/Models/Textures/" + mat.diffuse_texname
                    };

                    for (const auto& texPath : searchPaths) {
                        try {
                            auto tex = std::make_shared<Texture2D>(texPath, resDesc);
                            newMaterial->setDiffuseTexture(tex);
                            break;
                        }
                        catch (const std::exception&) {
                            continue;
                        }
                    }
                }
                materials.push_back(newMaterial);
            }
            catch (const std::exception&) {
                auto defaultMat = std::make_shared<Material>(mat.name);
                defaultMat->setDiffuseColor({ 0.7f, 0.7f, 0.7f, 1.0f });
                materials.push_back(defaultMat);
            }
        }

        if (materials.empty()) {
            auto defaultMat = std::make_shared<Material>("Default");
            defaultMat->setDiffuseColor({ 0.7f, 0.7f, 0.7f, 1.0f });
            materials.push_back(defaultMat);
        }

        for (size_t i = 0; i < geometry.size(); i++) {
            try {
                const auto& shapeData = geometry[i];
                std::vector<Vertex> vertices;
                std::vector<ui32> indices;

                size_t offset = 0;
                for (size_t j = 0; j < shapeData.mesh.num_face_vertices.size(); j++) {
                    int numVerts = shapeData.mesh.num_face_vertices[j];

                    if (numVerts < 3) continue;

                    for (int k = 0; k < numVerts; k++) {
                        if (offset + k >= shapeData.mesh.indices.size()) continue;

                        tinyobj::index_t data = shapeData.mesh.indices[offset + k];
                        Vertex v;
                        v.color = { 1.0f, 1.0f, 1.0f, 1.0f };
                        v.normal = { 0.0f, 1.0f, 0.0f };
                        v.texCoord = { 0.0f, 0.0f };

                        if (data.vertex_index >= 0) {
                            v.position.x = attributes.vertices[3 * data.vertex_index + 0];
                            v.position.y = attributes.vertices[3 * data.vertex_index + 1];
                            v.position.z = attributes.vertices[3 * data.vertex_index + 2];
                        }
                        if (data.normal_index >= 0) {
                            v.normal.x = attributes.normals[3 * data.normal_index + 0];
                            v.normal.y = attributes.normals[3 * data.normal_index + 1];
                            v.normal.z = attributes.normals[3 * data.normal_index + 2];
                        }
                        if (data.texcoord_index >= 0) {
                            v.texCoord.x = attributes.texcoords[2 * data.texcoord_index + 0];
                            v.texCoord.y = 1.0f - attributes.texcoords[2 * data.texcoord_index + 1];
                        }
                        vertices.push_back(v);
                        indices.push_back(static_cast<ui32>(indices.size()));
                    }
                    offset += numVerts;
                }

                if (!vertices.empty() && !indices.empty()) {
                    auto mesh = std::make_shared<Mesh>("SubMesh_" + std::to_string(i));
                    mesh->createRenderingResources(vertices, indices, resDesc);

                    if (!shapeData.mesh.material_ids.empty() && shapeData.mesh.material_ids[0] >= 0) {
                        mesh->setMaterial(materials[shapeData.mesh.material_ids[0]]);
                    }
                    else {
                        mesh->setMaterial(materials[0]);
                    }
                    asset->addMesh(mesh);
                }
            }
            catch (const std::exception&) {
                continue;
            }
        }

        return true;
    }
    catch (const std::exception&) {
        return false;
    }
    catch (...) {
        return false;
    }
}