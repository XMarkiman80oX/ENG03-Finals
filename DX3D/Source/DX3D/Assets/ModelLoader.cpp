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

        std::vector<std::shared_ptr<Material>> loadedMaterials;

        for (const auto& mat : materials) {
            try {
                auto material = std::make_shared<Material>(mat.name);
                material->setDiffuseColor(Vector4(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1.0f));
                loadedMaterials.push_back(material);
            }
            catch (...) {
                printf("Error loading material, skipping\n");
            }
        }

        if (loadedMaterials.empty()) {
            loadedMaterials.push_back(std::make_shared<Material>("Default"));
        }

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
                            printf("Index out of bounds, skipping face\n");
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
                    mesh->setMaterial(loadedMaterials[0]);
                    model->addMesh(mesh);
                    printf("Created mesh %zu with %zu vertices\n", s, vertices.size());
                }
            }
            catch (const std::exception& e) {
                printf("Error processing shape %zu: %s\n", s, e.what());
                continue;
            }
            catch (...) {
                printf("Unknown error processing shape %zu\n", s);
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