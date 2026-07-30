#include "model.h"

void instantiate(Model model, vec3 offset, float scale, Material material) {
    Model instance{};
    instance.bvhNodeIndex = model.bvhNodeIndex;
    instance.offset = offset;
    instance.scale = scale;
    instance.material = material;
    models.push_back(instance);
}

void instantiate(Model model, vec3 offset, float scale) {
    instantiate(model, offset, scale, {vec3(1.0), 0.0, vec3(0.0), 0.0, 0, 0});
}

void instantiate(Model model, Material material) {
    instantiate(model, vec3(0.0), 1.0, material);
}

void instantiate(Model model) {
    instantiate(model, vec3(0.0), 1.0, {vec3(1.0), 0.0, vec3(0.0), 0.0, 0, 0});
}
