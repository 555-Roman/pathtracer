#ifndef PATHTRACER_MODEL_H
#define PATHTRACER_MODEL_H

#include "include.h"

void instantiate(Model model);
void instantiate(Model model, Material material);
void instantiate(Model model, vec3 offset, float scale);
void instantiate(Model model, vec3 offset, float scale, Material material);

void sendModels();
void sendModel(uint modelIndex);

#endif //PATHTRACER_MODEL_H
