// Enhanced ModelLoader.cpp implementation

#include <DX3D/Assets/ModelLoader.h>
#include <DX3D/Graphics/Texture2D.h>
#include <fstream>

#define TINYOBJLOADER_IMPLEMENTATION
#include <DX3D/Assets/tiny_obj_loader.h>

using namespace dx3d;

std::unordered_map<std::string, std::shared_ptr<Material>> ModelLoader::s_materialCache;

std::string ModelLoader::getDirectory(const std::string& filePath) {
    size_t pos = filePath.find_last_of("/\\");
    if (pos != std::string::npos) {
        return filePath.substr(0, pos + 1);
    }
    return "";
}

std::string ModelLoader::getAssetPath(const std::string& relativePath)
{
    std::vector<std::string> possiblePaths = {
        "DX3D/Assets/Models/" + relativePath,
        "GDENG03-Engine/DX3D/Assets/Models/" + relativePath,
        "../DX3D/Assets/Models/" + relativePath,
        "../../DX3D/Assets/Models/" + relativePath
    };

    for (const auto& path : possiblePaths) {
        std::ifstream test(path);
        if (test.good()) {
            test.close();
            return path;
        }
    }

    return "DX3D/Assets/Models/" + relativePath;
}

std::shared_ptr<Material> ModelLoader::loadMaterial(
    const std::string& materialName,
    const std::string& materialFile,
    const std::string& baseDirectory,
    const GraphicsResourceDesc& resourceDesc)
{
    // Check cache first
    std::string cacheKey = baseDirectory + materialName;
    auto it = s_materialCache.find(cacheKey);
    if (it != s_materialCache.end()) {
        return it->second;
    }

    auto material = std::make_shared<Material>(materialName);

    // Try to load material file if it exists
    std::string mtlPath = baseDirectory + materialFile;
    std::ifstream mtlFile(mtlPath);

    if (mtlFile.is_open()) {
        printf("Loading MTL file: %s\n", mtlPath.c_str());

        std::string line;
        bool foundMaterial = false;

        while (std::getline(mtlFile, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::istringstream iss(line);
            std::string command;
            iss >> command;

            if (command == "newmtl") {
                std::string name;
                iss >> name;
                foundMaterial = (name == materialName);
            }
            else if (foundMaterial) {
                if (command == "Ka") {
                    float r, g, b;
                    iss >> r >> g >> b;
                    material->setAmbientColor(Vector4(r, g, b, 1.0f));
                }
                else if (command == "Kd") {
                    float r, g, b;
                    iss >> r >> g >> b;
                    material->setDiffuseColor(Vector4(r, g, b, 1.0f));
                }
                else if (command == "Ks") {
                    float r, g, b;
                    iss >> r >> g >> b;
                    material->setSpecularColor(Vector4(r, g, b, 1.0f));
                }
                else if (command == "Ke") {
                    float r, g, b;
                    iss >> r >> g >> b;
                    material->setEmissiveColor(Vector4(r, g, b, 1.0f));
                }
                else if (command == "Ns") {
                    float shininess;
                    iss >> shininess;
                    material->setSpecularPower(shininess);
                }
                else if (command == "d" || command == "Tr") {
                    float opacity;
                    iss >> opacity;
                    if (command == "Tr") opacity = 1.0f - opacity; // Tr is transparency, d is opacity
                    material->setOpacity(opacity);
                }
                else if (command == "map_Kd") {
                    std::string texturePath;
                    iss >> texturePath;

                    // Try different texture paths
                    std::vector<std::string> texturePaths = {
                        baseDirectory + texturePath,
                        baseDirectory + "../Textures/" + texturePath,
                        "DX3D/Assets/Textures/" + texturePath,
                        "DX3D/Assets/Models/Textures/" + texturePath
                    };

                    for (const auto& path : texturePaths) {
                        try {
                            auto texture = std::make_shared<Texture2D>(path, resourceDesc);
                            material->setDiffuseTexture(texture);
                            printf("Loaded texture for %s: %s\n", materialName.c_str(), path.c_str());
                            break;
                        }
                        catch (const std::exception& e) {
                            // Try next path
                            continue;
                        }
                    }
                }
            }
        }
        mtlFile.close();
    }
    else {
        printf("Could not open MTL file: %s\n", mtlPath.c_str());
    }

    // Cache the material
    s_materialCache[cacheKey] = material;
    return material;
}

std::shared_ptr<Model> ModelLoader::createDefaultModel(const GraphicsResourceDesc& resourceDesc)
{
    auto model = std::make_shared<Model>();
    model->setName("DefaultCube");

    std::vector<Vertex> vertices = {
        { {-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f} },
        { {0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f} },
        { {0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f} },
        { {-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f} },

        { {-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} },
        { {0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} },
        { {0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} },
        { {-0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} }
    };

    std::vector<ui32> indices = {
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        0, 3, 7, 0, 7, 4,
        1, 5, 6, 1, 6, 2,
        3, 2, 6, 3, 6, 7,
        0, 4, 5, 0, 5, 1
    };

    try {
        auto mesh = std::make_shared<Mesh>("DefaultCube");
        mesh->createRenderingResources(vertices, indices, resourceDesc);

        auto material = std::make_shared<Material>("DefaultMaterial");
        material->setDiffuseColor(Vector4(0.7f, 0.7f, 0.7f, 1.0f));
        mesh->setMaterial(material);

        model->addMesh(mesh);
        printf("Created default cube model as fallback\n");
    }
    catch (...) {
        printf("Failed to create default model - returning empty model\n");
    }

    return model;
}

std::shared_ptr<Model> ModelLoader::LoadModel(
    const std::string& filePath,
    const GraphicsResourceDesc& resourceDesc)
{
    try {
        auto model = std::make_shared<Model>();

        if (!loadOBJ(filePath, model, resourceDesc)) {
            printf("Failed to load OBJ file: %s - creating default model\n", filePath.c_str());
            return createDefaultModel(resourceDesc);
        }

        model->setFilePath(filePath);
        model->setName(filePath);
        return model;
    }
    catch (const std::exception& e) {
        printf("Exception loading model %s: %s - creating default model\n", filePath.c_str(), e.what());
        return createDefaultModel(resourceDesc);
    }
    catch (...) {
        printf("Unknown error loading model %s - creating default model\n", filePath.c_str());
        return createDefaultModel(resourceDesc);
    }
}

bool ModelLoader::loadOBJ(
    const std::string& filePath,
    std::shared_ptr<Model>& model,
    const GraphicsResourceDesc& resourceDesc)
{
    try {
        std::string fullPath = getAssetPath(filePath);
        std::string baseDirectory = getDirectory(fullPath);

        std::ifstream testFile(fullPath);
        if (!testFile.good()) {
            printf("Failed to open file: %s\n", fullPath.c_str());
            return false;
        }
        testFile.close();

        printf("Loading model from: %s\n", fullPath.c_str());

        tinyobj::ObjReaderConfig config;
        config.triangulate = true;

        tinyobj::ObjReader reader;

        if (!reader.ParseFromFile(fullPath, config)) {
            printf("TinyOBJ failed to parse file: %s\n", reader.Error().c_str());
            return false;
        }

        auto& attrib = reader.GetAttrib();
        auto& shapes = reader.GetShapes();
        auto& materials = reader.GetMaterials();

        printf("Loaded %zu shapes, %zu materials\n", shapes.size(), materials.size());

        if (shapes.empty()) {
            printf("No shapes found in model file\n");
            return false;
        }

        // Load materials properly
        std::vector<std::shared_ptr<Material>> loadedMaterials;

        for (const auto& mat : materials) {
            try {
                auto material = std::make_shared<Material>(mat.name);

                // Set basic material properties from tinyobj data
                material->setDiffuseColor(Vector4(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1.0f));
                material->setAmbientColor(Vector4(mat.ambient[0], mat.ambient[1], mat.ambient[2], 1.0f));
                material->setSpecularColor(Vector4(mat.specular[0], mat.specular[1], mat.specular[2], 1.0f));
                material->setEmissiveColor(Vector4(mat.emission[0], mat.emission[1], mat.emission[2], 1.0f));
                material->setSpecularPower(mat.shininess);
                material->setOpacity(mat.dissolve);

                // Try to load diffuse texture if specified
                if (!mat.diffuse_texname.empty()) {
                    std::vector<std::string> texturePaths = {
                        baseDirectory + mat.diffuse_texname,
                        baseDirectory + "../Textures/" + mat.diffuse_texname,
                        "DX3D/Assets/Textures/" + mat.diffuse_texname,
                        "DX3D/Assets/Models/Textures/" + mat.diffuse_texname
                    };

                    for (const auto& path : texturePaths) {
                        try {
                            auto texture = std::make_shared<Texture2D>(path, resourceDesc);
                            material->setDiffuseTexture(texture);
                            printf("Loaded texture for %s: %s\n", mat.name.c_str(), path.c_str());
                            break;
                        }
                        catch (const std::exception& e) {
                            continue; // Try next path
                        }
                    }
                }

                loadedMaterials.push_back(material);
                printf("Loaded material: %s\n", mat.name.c_str());
            }
            catch (const std::exception& e) {
                printf("Error loading material %s: %s\n", mat.name.c_str(), e.what());
                // Create a default material
                auto defaultMat = std::make_shared<Material>(mat.name);
                defaultMat->setDiffuseColor(Vector4(0.7f, 0.7f, 0.7f, 1.0f));
                loadedMaterials.push_back(defaultMat);
            }
        }

        if (loadedMaterials.empty()) {
            auto defaultMat = std::make_shared<Material>("Default");
            defaultMat->setDiffuseColor(Vector4(0.7f, 0.7f, 0.7f, 1.0f));
            loadedMaterials.push_back(defaultMat);
        }

        // Process shapes
        for (size_t s = 0; s < shapes.size(); s++) {
            try {
                const auto& shape = shapes[s];
                std::vector<Vertex> vertices;
                std::vector<ui32> indices;

                size_t index_offset = 0;
                for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
                    int fv = shape.mesh.num_face_vertices[f];

                    if (fv < 3) continue;

                    for (int v = 0; v < fv; v++) {
                        if (index_offset + v >= shape.mesh.indices.size()) {
                            continue;
                        }

                        tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                        Vertex vertex;
                        vertex.color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
                        vertex.normal = Vector3(0.0f, 1.0f, 0.0f);
                        vertex.texCoord = Vector2(0.0f, 0.0f);

                        if (idx.vertex_index >= 0 && idx.vertex_index * 3 + 2 < attrib.vertices.size()) {
                            vertex.position.x = attrib.vertices[3 * idx.vertex_index + 0];
                            vertex.position.y = attrib.vertices[3 * idx.vertex_index + 1];
                            vertex.position.z = attrib.vertices[3 * idx.vertex_index + 2];
                        }

                        if (idx.normal_index >= 0 && idx.normal_index * 3 + 2 < attrib.normals.size()) {
                            vertex.normal.x = attrib.normals[3 * idx.normal_index + 0];
                            vertex.normal.y = attrib.normals[3 * idx.normal_index + 1];
                            vertex.normal.z = attrib.normals[3 * idx.normal_index + 2];
                        }

                        if (idx.texcoord_index >= 0 && idx.texcoord_index * 2 + 1 < attrib.texcoords.size()) {
                            vertex.texCoord.x = attrib.texcoords[2 * idx.texcoord_index + 0];
                            vertex.texCoord.y = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1];
                        }

                        vertices.push_back(vertex);
                        indices.push_back(static_cast<ui32>(indices.size()));
                    }
                    index_offset += fv;
                }

                if (!vertices.empty() && !indices.empty()) {
                    auto mesh = std::make_shared<Mesh>("Mesh_" + std::to_string(s));
                    mesh->createRenderingResources(vertices, indices, resourceDesc);

                    // Assign material if available
                    if (!shape.mesh.material_ids.empty() && shape.mesh.material_ids[0] >= 0 &&
                        shape.mesh.material_ids[0] < loadedMaterials.size()) {
                        mesh->setMaterial(loadedMaterials[shape.mesh.material_ids[0]]);
                    }
                    else {
                        mesh->setMaterial(loadedMaterials[0]);
                    }

                    model->addMesh(mesh);
                    printf("Created mesh %zu with %zu vertices\n", s, vertices.size());
                }
            }
            catch (const std::exception& e) {
                printf("Error processing shape %zu: %s\n", s, e.what());
                continue;
            }
        }

        printf("Model loaded successfully!\n");
        return true;
    }
    catch (const std::exception& e) {
        printf("Exception in loadOBJ: %s\n", e.what());
        return false;
    }
    catch (...) {
        printf("Unknown exception in loadOBJ\n");
        return false;
    }
}