#include "import.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void import(const char* filePath) {
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(filePath,
        aiProcess_Triangulate           |
        aiProcess_JoinIdenticalVertices     |
        aiProcess_PreTransformVertices      |
        aiProcess_RemoveRedundantMaterials  |
        aiProcess_OptimizeMeshes            |
        aiProcess_GlobalScale
    );

    if (nullptr == scene) {
        std::cout << importer.GetErrorString() << std::endl;
    }

    processScene(scene);
}

void processScene(const aiScene* scene) {
    for (int meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++) {
        aiMesh* mesh = scene->mMeshes[meshIdx];
        std::vector<vec3> vertices;
        vertices.resize(mesh->mNumVertices);
        for (int vIdx = 0; vIdx < mesh->mNumVertices; vIdx++) {
            aiVector3D vertex = mesh->mVertices[vIdx];
            vertices[vIdx] = vec3(vertex.x, vertex.y, vertex.z);
        }
        uint triangleIndex = triangles.size();
        for (int fIdx = 0; fIdx < mesh->mNumFaces; fIdx++) {
            aiFace face = mesh->mFaces[fIdx];
            triangles.push_back(Triangle{vertices[face.mIndices[0]], 0.0, vertices[face.mIndices[1]], 0.0, vertices[face.mIndices[2]], 0.0, vec2(0.0), vec2(0.0), vec2(0.0), vec2(0.0)});
        }
        uint triangleCount = triangles.size() - triangleIndex;
        models.push_back({triangleIndex, triangleCount, {0, 0}, vec3(0.0), 1.0, Material{vec3(1.0), 0.0, vec3(0.0), 0.0, 0, 0}});
    }
}
