#include "import.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

void importAndSend(const char* filePath) {
    import(filePath);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangle_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, triangles.size() * sizeof(Triangle), triangles.data(), GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvhNode_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, bvhNodes.size() * sizeof(BVHNode), bvhNodes.data(), GL_DYNAMIC_COPY);
    sendModels();
}

void import(const char* filePath) {
    auto start = std::chrono::high_resolution_clock::now();

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(filePath,
        aiProcess_Triangulate               |
        aiProcess_JoinIdenticalVertices     |
        aiProcess_RemoveRedundantMaterials  |
        aiProcess_GenNormals                |
        aiProcess_GenUVCoords
    );

    if (nullptr == scene) {
        std::cout << importer.GetErrorString() << std::endl;
        return;
    }

    uint startBvhNodeCount = bvhNodes.size();

    processNode(scene->mRootNode, scene, filePath);

    auto end = std::chrono::high_resolution_clock::now();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "import():   " << ms << "ms" << std::endl;
    std::cout << "BVHNodes:   " << bvhNodes.size() - startBvhNodeCount << std::endl;
    std::cout << "max depth:  " << bvhMaxDepth << std::endl;
}

void processNode(aiNode* node, const aiScene* scene, const char* filePath) {
    for (int meshIdx = 0; meshIdx < node->mNumMeshes; meshIdx++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[meshIdx]];
        processMesh(mesh, scene, filePath);
    }
    for (int childIdx = 0; childIdx < node->mNumChildren; childIdx++) {
        aiNode* child = node->mChildren[childIdx];
        processNode(child, scene, filePath);
    }
}

void processMesh(aiMesh* mesh, const aiScene* scene, const char* filePath) {
    std::cout << "mesh" << std::endl;

    std::vector<vec3> positions;
    std::vector<vec3> normals;
    std::vector<vec2> uvs;
    for (int vertexIdx = 0; vertexIdx < mesh->mNumVertices; vertexIdx++) {
        aiVector3D position = mesh->mVertices[vertexIdx];
        positions.push_back(vec3(position.x, position.y, position.z));
        if (mesh->HasNormals()) {
            aiVector3D normal = mesh->mNormals[vertexIdx];
            normals.push_back(vec3(normal.x, normal.y, normal.z));
        } else {
            normals.push_back(vec3(0.0, 1.0, 0.0));
        }
        if (mesh->HasTextureCoords(0)) {
            aiVector3D uv = mesh->mTextureCoords[0][vertexIdx];
            uvs.push_back(vec2(uv.x, uv.y));
        } else {
            uvs.push_back(vec2(0.0));
        }
    }

    uint triangleIndex = triangles.size();
    for (int faceIdx = 0; faceIdx < mesh->mNumFaces; faceIdx++) {
        aiFace face = mesh->mFaces[faceIdx];
        uint idx0 = face.mIndices[0];
        uint idx1 = face.mIndices[1];
        uint idx2 = face.mIndices[2];
        triangles.push_back(Triangle{
            positions[idx0], uvs[idx0].x,
            positions[idx1], uvs[idx1].x,
            positions[idx2], uvs[idx2].x,
            normals[idx0], uvs[idx0].y,
            normals[idx1], uvs[idx1].y,
            normals[idx2], uvs[idx2].y,
        });
    }
    uint triangleCount = triangles.size() - triangleIndex;
    std::cout << "triangle count: " << triangleCount << std::endl;

    uint bvhNodeIndex = bvhNodes.size();

    auto start = std::chrono::high_resolution_clock::now();

    buildBVH(triangleIndex, triangleCount);

    auto end = std::chrono::high_resolution_clock::now();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "buildBVH(): " << ms << "ms" << std::endl;

    Material material = getMeshMaterial(mesh, scene, filePath);
    Model model = {
        bvhNodeIndex,
        triangleIndex,
        triangleCount,
        vec3(0.0),
        vec3(0.0),
        vec3(1.0),
        mat4(1.0),
        mat4(1.0),
        material
    };
    if (material.emissionStrength > 0.0) {
        // std::cout << "aaa " <<models.size() << std::endl;
        models.insert(models.begin() + nextLightIndex, model);
        nextLightIndex++;
        // std::cout << models.size() << std::endl;
        // std::cout << "Light!" << std::endl;
        // std::cout << std::endl;
    } else {
        models.push_back(model);
        // std::cout << "Not Light!" << std::endl;
        // std::cout << std::endl;
    }
}

Material getMeshMaterial(aiMesh* mesh, const aiScene* scene, const char* filePath) {
    aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

    aiColor3D colour (0.f,0.f,0.f);
    mat->Get(AI_MATKEY_COLOR_DIFFUSE, colour);

    float opacity = 1.0;
    mat->Get(AI_MATKEY_OPACITY, opacity);

    aiColor3D emissionColour (0.f,0.f,0.f);
    mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissionColour);

    float emissionStrength = 0.0;
    mat->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissionStrength);
    if (emissionColour.r > 0.0 && emissionColour.g > 0.0 && emissionColour.b > 0.0 && emissionStrength == 0.0) {
        emissionStrength = max(max(emissionColour.r, emissionColour.g), emissionColour.b);
        emissionColour = aiColor3D(emissionColour.r / emissionStrength, emissionColour.g / emissionStrength, emissionColour.b / emissionStrength);
    }

    float roughness = 1.0;
    mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);

    float metalness = 0.0;
    mat->Get(AI_MATKEY_METALLIC_FACTOR, metalness);

    float ior = 1.5;
    mat->Get(AI_MATKEY_REFRACTI, ior);

    float transmission = 0.0;
    mat->Get(AI_MATKEY_TRANSMISSION_FACTOR, transmission);

    GLuint64 albedoTextureHandle = getTextureHandle(aiTextureType_DIFFUSE, mat, filePath);
    GLuint64 roughnessTextureHandle = getTextureHandle(aiTextureType_SHININESS, mat, filePath);
    GLuint64 metalnessTextureHandle = getTextureHandle(aiTextureType_METALNESS, mat, filePath);
    GLuint64 normalTextureHandle = getTextureHandle(aiTextureType_NORMALS, mat, filePath);

    return Material{
        vec3(colour.r, colour.g, colour.b),
        opacity,
        vec3(emissionColour.r, emissionColour.g, emissionColour.b),
        emissionStrength,
        roughness,
        metalness,
        ior,
        transmission,
        vec3(0.0, 0.0, 0.0),
        0.0,
        vec3(0.0, 0.0, 0.0),
        0.0,
        albedoTextureHandle,
        roughnessTextureHandle,
        metalnessTextureHandle,
        normalTextureHandle
    };
}

GLuint64 getTextureHandle(aiTextureType textureType, aiMaterial* mat, const char* filePath) {
    GLuint64 textureHandle = 0;
    if (mat->GetTextureCount(textureType) > 0) {
        aiString str;
        mat->GetTexture(textureType, 0, &str);
        std::filesystem::path path(std::string(str.C_Str()));
        if (path.is_relative()) {
            std::string directoryPath = filePath;
            directoryPath = directoryPath.substr(0, directoryPath.find_last_of('/')+1);
            if (directoryPath.empty()) {
                directoryPath = filePath;
                directoryPath = directoryPath.substr(0, directoryPath.find_last_of('\\')+1);
            }
            textureHandle = getTexture((directoryPath + std::string(str.C_Str())).c_str());
        } else if (path.is_absolute()) {
            textureHandle = getTexture(std::string(str.C_Str()).c_str());
        } else {
            std::cout << "Invalid texture path: " << path << std::endl;
        }
    }
    return textureHandle;
}
