#include "import.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

void import(const char* filePath) {
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(filePath,
        aiProcess_Triangulate               |
        aiProcess_JoinIdenticalVertices     |
        aiProcess_RemoveRedundantMaterials  |
        aiProcess_GlobalScale               |
        aiProcess_GenNormals                |
        aiProcess_GenUVCoords
    );

    if (nullptr == scene) {
        std::cout << importer.GetErrorString() << std::endl;
    }

    processScene(scene, filePath);
}

void processScene(const aiScene* scene, const char* filePath) {
    aiNode* rootNode = scene->mRootNode;
    for (int childIdx = 0; childIdx < rootNode->mNumChildren; childIdx++) {
        aiNode* child = rootNode->mChildren[childIdx];
        for (int meshIdx = 0; meshIdx < child->mNumMeshes; meshIdx++) {
            aiMesh* mesh = scene->mMeshes[child->mMeshes[meshIdx]];
            processMesh(mesh, scene, filePath);
        }
    }
}

void processMesh(aiMesh* mesh, const aiScene* scene, const char* filePath) {
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
        triangles.push_back(Triangle{positions[idx0], 0.0, positions[idx1], 0.0, positions[idx2], 0.0, uvs[idx0], uvs[idx1], uvs[idx2], vec2(0.0)});
    }
    uint triangleCount = triangles.size() - triangleIndex;

    Material material = getMeshMaterial(mesh, scene, filePath);
    models.push_back(Model{triangleIndex, triangleCount, {0, 0}, vec3(0.0), 1.0, material});
}

Material getMeshMaterial(aiMesh* mesh, const aiScene* scene, const char* filePath) {
    aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

    aiColor3D colour (0.f,0.f,0.f);
    mat->Get(AI_MATKEY_COLOR_DIFFUSE,colour);

    GLuint64 textureHandle = 0;
    if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
        std::string directoryPath = filePath;
        directoryPath = directoryPath.substr(0, directoryPath.find_last_of('/')+1);
        aiString str;
        mat->GetTexture(aiTextureType_DIFFUSE, 0, &str);
        textureHandle = getTexture((directoryPath + std::string(str.C_Str())).c_str());
    }

    return Material{
        vec3(colour.r, colour.g, colour.b),
        0.0,
        vec3(0.0),
        0.0,
        textureHandle,
        0
    };
}
