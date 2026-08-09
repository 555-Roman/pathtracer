#include "model.h"

void instantiate(Model model, vec3 translation, vec3 rotation, vec3 scale, Material material) {
    Model instance{};
    instance.bvhNodeIndex = model.bvhNodeIndex;

    instance.translation = translation;
    instance.rotation = rotation;
    instance.scale = scale;
    instance.updateMatrices();

    instance.material = material;
    models.push_back(instance);
}

void instantiate(Model model, Material material) {
    instantiate(model, vec3(0.0), vec3(0.0), vec3(1.0), material);
}


void sendModels() {
    gpuModels.resize(models.size());
    for (int i = 0; i < models.size(); i++) {
        Model m = models[i];
        gpuModels[i] = m.gpuModel();
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, model_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gpuModels.size() * sizeof(GPU_Model), gpuModels.data(), GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void sendModel(uint modelIndex) {
    if (modelIndex >= models.size()) return;
    if (modelIndex >= gpuModels.size()) return;
    gpuModels[modelIndex] = models[modelIndex].gpuModel();
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, model_ssbo);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, modelIndex * sizeof(GPU_Model), sizeof(GPU_Model), &gpuModels[modelIndex]);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
