#version 460 core
#extension GL_ARB_bindless_texture: require

in vec3 originalRayDir;

out vec4 FragColor;

struct Ray {
    vec3 origin;
    vec3 dir;
};

struct Material {
    vec3 albedo;
    float opacity;
    vec3 emissionColour;
    float emissionStrength;
    float roughness;
    float metalness;
    float ior;
    float transmission;
    vec3 complexN;
    float padding0;
    vec3 complexK;
    float padding1;
    sampler2D albedoTextureHandle;
    sampler2D roughnessTextureHandle;
    sampler2D metalnessTextureHandle;
    sampler2D normalTextureHandle;
};
struct UsefulMaterial {
    vec3 albedo;
    float opacity;
    vec3 emissionColour;
    float emissionStrength;
    vec2 roughness;
    float metalness;
    float ior;
    float transmission;
    vec3 shadingNormal;
    vec3 complexN;
    vec3 complexK;
};

struct Triangle {
    vec3 aPosition;
    float aU;
    vec3 bPosition;
    float bU;
    vec3 cPosition;
    float cU;
    vec3 aNormal;
    float aV;
    vec3 bNormal;
    float bV;
    vec3 cNormal;
    float cV;
};

struct BVHNode {
    vec3 minBound;
    uint index;
    vec3 maxBound;
    uint triangleCount;
};

struct Model {
    uint bvhNodeIndex;
    uint triangleIndex;
    uint triangleCount;
    uint padding;

    Material material;

    mat4 modelWorldMatrix;
    mat4 worldModelMatrix;
};

struct HitRecord {
    bool hit;
    float t;
    vec3 pos;
    vec3 geometryNormal;
    vec3 interpolatedNormal;
    vec2 uv;
    uint triangleIdx;
    Material material;
};

layout (std430, binding = 0) buffer TriangleBuffer {
    Triangle triangles[];
};

layout (std430, binding = 1) buffer BVHNodeBuffer {
    BVHNode bvhNodes[];
};

uniform uint modelCount;
uniform uint lightCount;
layout (std430, binding = 2) buffer ModelBuffer {
    Model models[];
};

struct BsdfSample {
    vec3 wi;
    vec3 f;
    float pdf;
    bool smoothBounce;
    bool transmitted;
};

vec3 debugColour = vec3(0.0);

uint rngState;
float randomUniform() {
    rngState = rngState * 747796405u + 2891336453u;
    uint result = ((rngState >> ((rngState >> 28u) + 4u)) ^ rngState) * 277803737u;
    result = (result >> 22u) ^ result;
    return result / 4294967294.0;
}
vec3 randomSphere() {
    float z = 1.0 - 2.0 * randomUniform();
    float r = max(0.0, 1.0 - z*z);
    float phi = 2 * 3.1415926 * randomUniform();
    return vec3(r * cos(phi), r * sin(phi), z);
}
vec3 sampleCosineHemisphere(vec3 wo) {
    float phi = 2.0f * 3.1415926 * randomUniform();

    float z = sqrt(randomUniform());

    if (wo.z < 0.0) z = -z;
    float sinTheta = sqrt(clamp(1.0f - z * z, 0.0f, 1.0f));
    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);

    return vec3(x, y, z);
}

float AABBDst(Ray ray, vec3 minBound, vec3 maxBound) {
    vec3 invDir = 1.0 / ray.dir;

    vec3 tMin = (minBound - ray.origin) / ray.dir;
    vec3 tMax = (maxBound - ray.origin) / ray.dir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);

    bool hit = tFar >= tNear && tFar > 0;
    float dst = hit ? tNear > 0 ? tNear : 0 : 1.0 / 0.0;
    return dst;
}

HitRecord intersectTriangle(Ray ray, Triangle triangle, float opacity, sampler2D textureHandle) {
    HitRecord record;
    record.hit = false;

    vec3 edge1 = triangle.bPosition - triangle.aPosition;
    vec3 edge2 = triangle.cPosition - triangle.aPosition;

    vec3 normal = normalize(cross(edge1, edge2));
//    if (dot(normal, ray.dir) > 0) return record;

    vec3 ray_cross_e2 = cross(ray.dir, edge2);
    float det = dot(edge1, ray_cross_e2);

    if (det == 0.0) return record;

    float inv_det = 1.0 / det;
    vec3 s = ray.origin - triangle.aPosition;
    float u = inv_det * dot(s, ray_cross_e2);

    if (u < 0 || u - 1 > 0) return record;

    vec3 s_cross_e1 = cross(s, edge1);
    float v = inv_det * dot(ray.dir, s_cross_e1);

    if (v < 0 || u + v - 1 > 0) return record;

    float t = inv_det * dot(edge2, s_cross_e1);

    if (t <= 0.001) return record;

    float w = 1.0 - u - v;

    vec2 uv = vec2(triangle.aU, triangle.aV) * w + vec2(triangle.bU, triangle.bV) * u + vec2(triangle.cU, triangle.cV) * v;

    float alpha = opacity;
    if (uvec2(textureHandle) != uvec2(0)) {
        alpha = texture(textureHandle, uv).a;
    }
    if (randomUniform() >= alpha) return record;

    vec3 interpolatedNormal = normalize(triangle.aNormal * w + triangle.bNormal * u + triangle.cNormal * v);

    record.hit = true;
    record.t = t;
    record.pos = ray.origin + ray.dir * t;
    record.geometryNormal = normal;
    record.interpolatedNormal = interpolatedNormal;
    record.uv = uv;
    return record;
}

HitRecord intersectBVH(Ray ray, uint rootIdx, float tMax, Material material) {
    HitRecord closestRecord;
    closestRecord.hit = false;
    float closestT = tMax;

    debugColour.b += 1;
    if (AABBDst(ray, bvhNodes[rootIdx].minBound, bvhNodes[rootIdx].maxBound) >= closestT) return closestRecord;

    uint stack[63 + 1];
    uint stackPtr = 0;
    stack[stackPtr++] = rootIdx;

    while (stackPtr > 0 && stackPtr <= 63 + 1) {
        BVHNode node = bvhNodes[stack[--stackPtr]];

        if (node.triangleCount > 0) {
            debugColour.r += node.triangleCount;
            for (uint i = node.index; i < node.index + node.triangleCount; i++) {
                HitRecord record = intersectTriangle(ray, triangles[i], material.opacity, material.albedoTextureHandle);
                if (record.hit && record.t < closestT) {
                    debugColour.g = i;
                    closestRecord = record;
                    closestRecord.triangleIdx = i;
                    closestT = record.t;
                }
            }
        } else {
            uint childIndexA = node.index + 0;
            uint childIndexB = node.index + 1;
            BVHNode childA = bvhNodes[childIndexA];
            BVHNode childB = bvhNodes[childIndexB];

            float dstA = AABBDst(ray, childA.minBound, childA.maxBound);
            float dstB = AABBDst(ray, childB.minBound, childB.maxBound);
            debugColour.b += 2;

            bool isNearestA = dstA <= dstB;
            float dstNear = isNearestA ? dstA : dstB;
            float dstFar = isNearestA ? dstB : dstA;
            uint childIndexNear = isNearestA ? childIndexA : childIndexB;
            uint childIndexFar = isNearestA ? childIndexB : childIndexA;

            if (dstFar < closestT) stack[stackPtr++] = childIndexFar;
            if (dstNear < closestT) stack[stackPtr++] = childIndexNear;
        }
    }

    return closestRecord;
}

HitRecord intersectScene(Ray ray) {
    HitRecord closestRecord;
    closestRecord.hit = false;
    float closestT = 1.0 / 0.0;

    for (uint i = 0; i < modelCount; i++) {
        Model model = models[i];

        Ray localRay = ray;
        localRay.origin = vec3(model.modelWorldMatrix * vec4(localRay.origin, 1.0));
        localRay.dir = vec3(model.modelWorldMatrix * vec4(localRay.dir, 0.0));

        HitRecord record = intersectBVH(localRay, model.bvhNodeIndex, closestT, model.material);
        if (record.hit && record.t < closestT) {
            closestRecord = record;
            closestRecord.pos = vec3(model.worldModelMatrix * vec4(closestRecord.pos, 1.0));
            closestRecord.geometryNormal = normalize(vec3(model.worldModelMatrix * vec4(closestRecord.geometryNormal, 0.0)));
            closestRecord.interpolatedNormal = normalize(vec3(model.worldModelMatrix * vec4(closestRecord.interpolatedNormal, 0.0)));
            closestRecord.material = model.material;
            closestT = record.t;
        }
    }

    return closestRecord;
}

uniform uint skyboxFormat;
uniform samplerCube skyboxCubemapTexture;
uniform sampler2D skyboxEquirectangularTexture;
vec3 getSkybox(Ray ray) {
    if (skyboxFormat == 0) {
        float a = 0.5*(ray.dir.y + 1.0);
        return mix(vec3(1.0, 1.0, 1.0), vec3(0.5, 0.7, 1.0), a);
    } else if (skyboxFormat == 1) {
        return pow(texture(skyboxCubemapTexture, ray.dir).rgb, vec3(2.2));
    } else if (skyboxFormat == 2) {
        float u = atan(ray.dir.z, ray.dir.x) * 0.5 / 3.1415926 + 0.5;
        float v = asin(ray.dir.y) / 3.1415926 + 0.5;
        return pow(texture(skyboxEquirectangularTexture, vec2(u, v)).rgb, vec3(2.2));
    } else if (skyboxFormat == 3) {
        if (ray.dir.y >= 0.0)
            return vec3(1.0);
        return vec3(0.0);
    } else {
        return vec3(0.0);
    }
}

float fresnelReflection(vec3 wi, vec3 normal, float etaOutside, float etaInside) {
    float cosThetaI = dot(wi, normal);
    float etaI = etaOutside;
    float etaT = etaInside;
    if (cosThetaI < 0.0) {
        etaI = etaInside;
        etaT = etaOutside;
        cosThetaI = -cosThetaI;
    }
    float eta = etaT / etaI;

    float temp = eta * eta + cosThetaI * cosThetaI - 1;
    if (temp < 0) return 1;

    float g = sqrt(temp);
    return 0.5 * pow((g - cosThetaI) / (g + cosThetaI), 2) * (1 + pow(((g + cosThetaI)  * cosThetaI - 1) / ((g - cosThetaI) * cosThetaI+ 1), 2));
}

vec3 fresnelConductor(vec3 wi, vec3 normal, float etaDielectric, vec3 etaConductor, vec3 kConductor) {
    float cosThetaI = dot(wi, normal);
    if (cosThetaI < 0.0) return vec3(0.0);
    vec3 eta = etaConductor / etaDielectric;
    vec3 eta2 = eta*eta;
    vec3 k =   kConductor   / etaDielectric;
    vec3 k2 = k*k;

    float cos2ThetaI = cosThetaI * cosThetaI;
    float sin2ThetaI = 1 - cos2ThetaI;

    vec3 t0 = eta2 - k2 - sin2ThetaI;
    vec3 a2plusb2 = sqrt(t0 * t0 + 4 * eta2 * k2);
    vec3 t1 = a2plusb2 + cos2ThetaI;
    vec3 a = sqrt(0.5f * (a2plusb2 + t0));
    vec3 t2 = 2 * a * cosThetaI;
    vec3 Rs = (t1 - t2) / (t1 + t2);

    vec3 t3 = cos2ThetaI * a2plusb2 + sin2ThetaI * sin2ThetaI;
    vec3 t4 = t2 * sin2ThetaI;
    vec3 Rp = Rs * (t3 - t4) / (t3 + t4);

    return 0.5 * (Rp + Rs);
}

vec3 schlickFresnel(vec3 wi, vec3 normal, float etaOutside, float etaInside, vec3 albedo) {
    float cosThetaI = dot(wi, normal);
    if (cosThetaI < 0.0) return vec3(0.0);
    cosThetaI = max(cosThetaI, 0.0);

    float x = 1.0 - cosThetaI;
    x = x*x*x*x*x;
    return albedo + (1.0 - albedo) * x;
}

vec3 sampleGgxVndfHemisphere(vec3 wi) {
    float phi = 2.0f * 3.1415926 * randomUniform();

    float z = fma((1.0f - randomUniform()), (1.0f + wi.z), -wi.z);
    float sinTheta = sqrt(clamp(1.0f - z * z, 0.0f, 1.0f));
    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    vec3 c = vec3(x, y, z);

    vec3 h = c + wi;
    return h;
}

vec3 sampleGgxVndfNormal(vec3 wi, vec2 alpha) {
    if (wi.z < 0.0) wi.z = -wi.z;
    vec3 wiStd = normalize(vec3(wi.xy * alpha, wi.z));
    vec3 wmStd = sampleGgxVndfHemisphere(wiStd);
    vec3 wm = normalize(vec3(wmStd.xy * alpha, wmStd.z));
    return wm;
}

void frisvad(vec3 n, out vec3 b1, out vec3 b2) {
    if(n.z < -0.9999999) {
        b1 = vec3(0.0, -1.0, 0.0);
        b2 = vec3(-1.0, 0.0, 0.0);
        return;
    }

    float a = 1.0 / (1.0 + n.z);
    float b = -n.x * n.y * a;
    b1 = vec3(1.0 - n.x * n.x * a, b, -n.x);
    b2 = vec3(b, 1.0 - n.y * n.y * a, -n.y);
}

vec3 reflectBetter(vec3 w, vec3 n) {
    if (dot(w, n) < 0.0) n = -n;
    return reflect(-w, n);
}

vec3 refractBetter(vec3 w, vec3 n, float iorOutside, float iorInside) {
    float cosThetaI = dot(w, n);
    float iorI = iorOutside;
    float iorT = iorInside;
    if (cosThetaI < 0.0) {
        cosThetaI = -cosThetaI;
        iorI = iorInside;
        iorT = iorOutside;
        n = -n;
    }

    return refract(-w, n, iorI/iorT);
}

bool sameHemisphere(vec3 w0, vec3 w1) {
    return w0.z * w1.z > 0.0;
}

float ggxD(vec3 wm, vec2 alpha) {
    float cos2Theta = wm.z * wm.z;
    float sin2Theta = max(0.0, 1.0 - cos2Theta);
    float tan2Theta = sin2Theta / cos2Theta;
    if (isinf(tan2Theta)) return 0.0;

    float cos4Theta = cos2Theta * cos2Theta;

    float sinTheta = sqrt(sin2Theta);
    float cosPhi = (sinTheta == 0.0) ? 1.0 : clamp(wm.x / sinTheta, -1.0, 1.0);
    float sinPhi = (sinTheta == 0.0) ? 0.0 : clamp(wm.y / sinTheta, -1.0, 1.0);

    float e = tan2Theta * ((cosPhi / alpha.x) * (cosPhi / alpha.x) + (sinPhi / alpha.y) * (sinPhi / alpha.y));
    return 1.0 / (3.1415926 * alpha.x * alpha.y * cos4Theta * (1.0 + e) * (1.0 + e));
}

float ggxLambda(vec3 w, vec2 alpha) {
    float cos2Theta = w.z * w.z;
    float sin2Theta = max(0.0, 1.0 - cos2Theta);
    float tan2Theta = sin2Theta / cos2Theta;
    if (isinf(tan2Theta)) return 0.0;

    float sinTheta = sqrt(sin2Theta);
    float cosPhi = (sinTheta == 0.0) ? 1.0 : clamp(w.x / sinTheta, -1.0, 1.0);
    float sinPhi = (sinTheta == 0.0) ? 0.0 : clamp(w.y / sinTheta, -1.0, 1.0);

    float alpha2 = ((cosPhi * alpha.x) * (cosPhi * alpha.x) + (sinPhi * alpha.y) * (sinPhi * alpha.y));
    return (sqrt(1.0 + alpha2 * tan2Theta) - 1.0) / 2.0;
}

float ggxG(vec3 wo, vec3 wi, vec2 alpha) {
    return 1.0 / (1.0 + ggxLambda(wo, alpha) + ggxLambda(wi, alpha));
}

float ggxG1(vec3 w, vec2 alpha) {
    return 1.0 / (1.0 + ggxLambda(w, alpha));
}

float ggxD(vec3 w, vec3 wm, vec2 alpha) {
    return ggxG1(w, alpha) / abs(w.z) * ggxD(wm, alpha) * abs(dot(w, wm));
}

float ggxPDF(vec3 w, vec3 wm, vec2 alpha) {
    return ggxD(w, wm, alpha);
}

vec3 diffuse_f(vec3 wo, vec3 wi, UsefulMaterial material) {
    if (!sameHemisphere(wo, wi)) return vec3(0.0);
    return vec3(material.albedo) / 3.1415926;
}

vec3 transmissive_f(vec3 wo, vec3 wi, UsefulMaterial material) {
    if (sameHemisphere(wo, wi)) return vec3(0.0);
    if (max(material.roughness.x, material.roughness.y) < 0.001) return vec3(0.0);

    float cosTheta_o = wo.z;
    float cosTheta_i = wi.z;
    float etap = cosTheta_o > 0 ? material.ior : (1 / material.ior);
    vec3 wm = wi * etap + wo;
    if (cosTheta_i == 0 || cosTheta_o == 0 || dot(wm, wm) == 0) return vec3(0.0);
    wm = normalize(wm);
    wm = wm.z < 0.0 ? -wm : wm;

    if (dot(wm, wi) * cosTheta_i < 0 || dot(wm, wo) * cosTheta_o < 0)
        return vec3(0.0);

    float reflectedFraction = fresnelReflection(wo, wm, 1.0, material.ior);

    float denom = pow(dot(wi, wm) + dot(wo, wm) / etap, 2) * cosTheta_i * cosTheta_o;
    float ft = ggxD(wm, material.roughness) * (1 - reflectedFraction) * ggxG(wo, wi, material.roughness) * abs(dot(wi, wm) * dot(wo, wm) / denom);

    return ft * material.albedo;
}

vec3 entered_f(vec3 wo, vec3 wi, UsefulMaterial material) {
    return mix(
        diffuse_f(wo, wi, material),
        transmissive_f(wo, wi, material),
        material.transmission
    );
}

vec3 reflective_f(vec3 wo, vec3 wi, UsefulMaterial material) {
    if (!sameHemisphere(wo, wi)) return vec3(0.0);
    if (max(material.roughness.x, material.roughness.y) < 0.001) return vec3(0.0);

    float cosTheta_o = wo.z;
    float cosTheta_i = wi.z;
    vec3 wm = wi + wo;
    if (cosTheta_i == 0 || cosTheta_o == 0 || dot(wm, wm) == 0) return vec3(0.0);
    wm = normalize(wm);
    wm = wm.z < 0.0 ? -wm : wm;

    if (dot(wm, wi) * cosTheta_i < 0 || dot(wm, wo) * cosTheta_o < 0)
    return vec3(0.0);

    float reflectedFraction = fresnelReflection(wo, wm, 1.0, material.ior);

    float fr = ggxD(wm, material.roughness) * ggxG(wo, wi, material.roughness) * reflectedFraction / abs(4 * cosTheta_i * cosTheta_o);
    return vec3(fr);
}

vec3 dielectric_f(vec3 wo, vec3 wi, UsefulMaterial material) {
    return reflective_f(wo, wi, material) + entered_f(wo, wi, material);
}

vec3 conductor_f(vec3 wo, vec3 wi, UsefulMaterial material) {
    if (!sameHemisphere(wo, wi)) return vec3(0.0);
    if (max(material.roughness.x, material.roughness.y) < 0.001) return vec3(0.0);

    float cosTheta_o = abs(wo.z);
    float cosTheta_i = abs(wi.z);
    if (cosTheta_i == 0 || cosTheta_o == 0) return vec3(0.0);
    vec3 wm = wi + wo;
    if (dot(wm, wm) == 0) return vec3(0.0);
    wm = normalize(wm);

    vec3 reflectionTint;
    if (material.complexN == vec3(0.0))
        reflectionTint = schlickFresnel(wo, wm, 1.0, material.ior, material.albedo);
    else
        reflectionTint = fresnelConductor(wo, wm, 1.0, material.complexN, material.complexK);

    return ggxD(wm, material.roughness) * reflectionTint * ggxG(wo, wi, material.roughness) / (4 * cosTheta_i * cosTheta_o);
}

vec3 bsdf_f(vec3 wo, vec3 wp, UsefulMaterial material) {
    return mix(
        dielectric_f(wo, wp, material),
        conductor_f(wo, wp, material),
        material.metalness
    );
}

BsdfSample diffuse_sample_f(vec3 wo, UsefulMaterial material) {
    vec3 wi = sampleCosineHemisphere(wo);
    float pdf = abs(wi.z) / 3.1415926;
    vec3 f = material.albedo / 3.1415926;

    return BsdfSample(wi, f, pdf, false, false);
}

BsdfSample transmissive_sample_f(vec3 wo, vec3 wm, UsefulMaterial material) {
    if (max(material.roughness.x, material.roughness.y) < 0.001) {
        vec3 wi = refractBetter(wo, vec3(0.0, 0.0, 1.0), 1.0, material.ior);
        if (wi == vec3(0.0)) return BsdfSample(vec3(0.0), vec3(0.0), 0.0, false, true);

        vec3 ft = material.albedo / abs(wi.z);
        return BsdfSample(wi, ft, 1.0, true, true);
    } else {
        vec3 wi = refractBetter(wo, wm, 1.0, material.ior);
        if (sameHemisphere(wo, wi) || wi.z == 0 || wi == vec3(0.0)) return BsdfSample(vec3(0.0), vec3(0.0), 0.0, false, true);

        float eta = material.ior;
        if (dot(wo, wm) < 0.0) eta = 1.0 / eta;
        float denom = pow(dot(wi, wm) + dot(wo, wm) / eta, 2);
        float dwm_dwi = abs(dot(wi, wm)) / denom;
        float pdf = ggxPDF(wo, wm, material.roughness) * dwm_dwi;

        vec3 ft = material.albedo * ggxD(wm, material.roughness) * ggxG(wo, wi, material.roughness) * abs(dot(wi, wm) * dot(wo, wm) / (wi.z * wo.z * denom));

        return BsdfSample(wi, ft, pdf, false, true);
    }
}

BsdfSample entered_sample_f(vec3 wo, vec3 wm, UsefulMaterial material) {
    if (randomUniform() < material.transmission) {
        BsdfSample transmissiveSample = transmissive_sample_f(wo, wm, material);
        transmissiveSample.f *= material.transmission;
        transmissiveSample.pdf *= material.transmission;
        return transmissiveSample;
    }

    BsdfSample diffuseSample = diffuse_sample_f(wo, material);
    diffuseSample.f *= (1.0 - material.transmission);
    diffuseSample.pdf *= (1.0 - material.transmission);
    return diffuseSample;
}

BsdfSample reflective_sample_f(vec3 wo, vec3 wm, UsefulMaterial material) {
    if (max(material.roughness.x, material.roughness.y) < 0.001) {
        vec3 wi = vec3(-wo.x, -wo.y, wo.z);
        float fr = 1.0 / abs(wi.z);
        return BsdfSample(wi, vec3(fr), 1.0, true, false);
    } else {
        vec3 wi = reflectBetter(wo, wm);
        if (!sameHemisphere(wo, wi)) return BsdfSample(vec3(0.0), vec3(0.0), 0.0, false, false);

        float pdf = ggxPDF(wo, wm, material.roughness) / (4 * abs(dot(wo, wm)));
        float f = ggxD(wm, material.roughness) * ggxG(wo, wi, material.roughness) / (4 * wi.z * wo.z);
        return BsdfSample(wi, vec3(f), pdf, false, false);
    }
}

BsdfSample dielectric_sample_f(vec3 wo, UsefulMaterial material) {
    vec3 wm;
    float reflectedFraction;
    if (max(material.roughness.x, material.roughness.y) < 0.001) {
        wm = vec3(0.0, 0.0, 1.0);
        reflectedFraction = fresnelReflection(wo, wm, 1.0, material.ior);
    } else {
        wm = sampleGgxVndfNormal(wo, material.roughness);
        reflectedFraction = fresnelReflection(wo, wm, 1.0, material.ior);
    }

    if (randomUniform() < reflectedFraction) {
        BsdfSample reflectiveSample = reflective_sample_f(wo, wm, material);
        reflectiveSample.f *= reflectedFraction;
        reflectiveSample.pdf *= reflectedFraction;
        return reflectiveSample;
    }

    BsdfSample enteredSample = entered_sample_f(wo, wm, material);
    enteredSample.f *= (1.0 - reflectedFraction);
    enteredSample.pdf *= (1.0 - reflectedFraction);
    return enteredSample;
}

BsdfSample conductor_sample_f(vec3 wo, UsefulMaterial material) {
    if (max(material.roughness.x, material.roughness.y) < 0.001) {
        vec3 wi = vec3(-wo.x, -wo.y, wo.z);

        vec3 reflectionTint;
        if (material.complexN == vec3(0.0))
            reflectionTint = schlickFresnel(wo, vec3(0.0, 0.0, 1.0), 1.0, material.ior, material.albedo);
        else
            reflectionTint = fresnelConductor(wo, vec3(0.0, 0.0, 1.0), 1.0, material.complexN, material.complexK);

        return BsdfSample(wi, reflectionTint / abs(wi.z), 1, true, false);
    }

    vec3 wm = sampleGgxVndfNormal(wo, material.roughness);
    vec3 wi = reflectBetter(wo, wm);
    if (!sameHemisphere(wo, wi)) return BsdfSample(vec3(0.0), vec3(0.0), 0.0, false, false);

    float pdf = ggxPDF(wo, wm, material.roughness) / (4 * abs(dot(wo, wm)));

    float cosTheta_o = abs(wo.z), cosTheta_i = abs(wi.z);

    vec3 reflectionTint;
    if (material.complexN == vec3(0.0))
        reflectionTint = schlickFresnel(wo, wm, 1.0, material.ior, material.albedo);
    else
        reflectionTint = fresnelConductor(wo, wm, 1.0, material.complexN, material.complexK);

    vec3 f = ggxD(wm, material.roughness) * reflectionTint * ggxG(wo, wi, material.roughness) / (4 * cosTheta_i * cosTheta_o);

    return BsdfSample(wi, f, pdf, false, false);
}

BsdfSample bsdf_sample_f(vec3 wo, UsefulMaterial material) {
    if (randomUniform() < material.metalness) {
        BsdfSample conductorSample = conductor_sample_f(wo, material);
        conductorSample.f *= material.metalness;
        conductorSample.pdf *= material.metalness;
        return conductorSample;
    }
    BsdfSample dielectricSample = dielectric_sample_f(wo, material);
    dielectricSample.f *= (1.0 - material.metalness);
    dielectricSample.pdf *= (1.0 - material.metalness);
    return dielectricSample;
}

uniform uint displayDebug;

UsefulMaterial getMaterial(HitRecord record) {
    Material material = record.material;

    UsefulMaterial useful;
    useful.albedo = material.albedo;
    useful.opacity = material.opacity;
    useful.emissionColour = material.emissionColour;
    useful.emissionStrength = material.emissionStrength;
    useful.roughness = vec2(material.roughness * material.roughness);
    useful.metalness = material.metalness;
    useful.ior = material.ior;
    useful.transmission = material.transmission;
    useful.complexN = material.complexN;
    useful.complexK = material.complexK;
    useful.shadingNormal = vec3(0.0, 0.0, 1.0);

    if (uvec2(material.albedoTextureHandle) != uvec2(0)) {
        vec4 color = texture(material.albedoTextureHandle, record.uv);
        useful.albedo = pow(color.rgb, vec3(2.2));
        useful.opacity = color.a;
    }
    if (uvec2(material.roughnessTextureHandle) != uvec2(0))
        useful.roughness = vec2(pow(texture(material.roughnessTextureHandle, record.uv).r, 2.0));
    if (uvec2(material.metalnessTextureHandle) != uvec2(0))
        useful.metalness = texture(material.metalnessTextureHandle, record.uv).r;
    if (uvec2(material.normalTextureHandle) != uvec2(0))
        useful.shadingNormal = normalize(texture(material.normalTextureHandle, record.uv).rgb * 2.0 - 1.0);

    vec3 N = record.interpolatedNormal;
    vec3 T, B;
    frisvad(N, T, B);
    N = normalize(useful.shadingNormal.x * T + useful.shadingNormal.y * B + useful.shadingNormal.z * N);
    useful.shadingNormal = N;

    useful.albedo = vec3(1.0);
    useful.roughness = vec2(0.0);
    useful.metalness = 1.0;

    return useful;
}

uniform int maxBounces;
vec3 trace(Ray cameraRay) {
    Ray ray = cameraRay;
    vec3 incomingLight = vec3(0.0);
    vec3 rayColour = vec3(1.0);

    for (uint bounce = 0; bounce <= maxBounces; bounce++) {
        HitRecord record = intersectScene(ray);
        if (!record.hit) {
            incomingLight += getSkybox(ray) * rayColour;
            break;
        }
//        if (displayDebug > 0)
//            return record.interpolatedNormal * .5 + .5;
//            return record.geometryNormal * .5 + .5;
//            return getMaterial(record).shadingNormal * .5 + .5;
        UsefulMaterial material = getMaterial(record);


        vec3 N = material.shadingNormal;
        vec3 T, B;
        frisvad(N, T, B);

        vec3 wo = -ray.dir;
        vec3 woLocal = normalize(vec3(dot(wo, T), dot(wo, B), dot(wo, N)));
        vec3 Le = material.emissionColour * material.emissionStrength;
        incomingLight += Le * rayColour;

        if (material.emissionStrength > 0.0) break;

        #define BSDF_SAMPLING
#ifdef BSDF_SAMPLING
        BsdfSample bsdfSample = bsdf_sample_f(woLocal, material);
        if (bsdfSample.wi == vec3(0.0) || bsdfSample.f == vec3(0.0) || bsdfSample.pdf == 0.0) break;

        vec3 fcos = bsdfSample.f * abs(bsdfSample.wi.z);

//        rayColour *= fcos / bsdfSample.pdf;
        if (rayColour == vec3(0.0)) break;

        vec3 offset = record.geometryNormal * 0.001 * sign(dot(record.geometryNormal, wo));
        if (bsdfSample.transmitted)
            offset = -offset;

        ray.origin = record.pos + offset;
        vec3 wiWorld = normalize(bsdfSample.wi.x * T + bsdfSample.wi.y * B + bsdfSample.wi.z * N);
        ray.dir = wiWorld;
#else
        vec3 wp = randomSphere();
        vec3 wpLocal = normalize(vec3(dot(wp, T), dot(wp, B), dot(wp, N)));
        float wpPdf = 1.0 / (4.0 * 3.1415926);

        vec3 fcos = bsdf_f(woLocal, wpLocal, material) * abs(dot(wp, material.shadingNormal));

        rayColour *= fcos / wpPdf;
        if (rayColour == vec3(0.0)) break;

        ray.origin = record.pos + record.geometryNormal * 0.001 * sign(dot(wp, record.geometryNormal));
        ray.dir = wp;
#endif
    }

    return incomingLight;
}

vec4 turbo_color_map(float x) {
    // Source:
    // https://research.google/blog/turbo-an-improved-rainbow-colormap-for-visualization/

    vec4 kRedVec4 = vec4(0.13572138, 4.61539260, -42.66032258, 132.13108234);
    vec4 kGreenVec4 = vec4(0.09140261, 2.19418839, 4.84296658, -14.18503333);
    vec4 kBlueVec4 = vec4(0.10667330, 12.64194608, -60.58204836, 110.36276771);
    vec2 kRedVec2   = vec2(-152.94239396, 59.28637943);
    vec2 kGreenVec2 = vec2(4.27729857, 2.82956604);
    vec2 kBlueVec2  = vec2(-89.90310912, 27.34824973);

    x             = clamp(x, 0, 1);
    vec4 v4 = vec4(1.0, x, x * x, x * x * x);
    vec2 v2 = vec2(v4.z, v4.w) * v4.z;
    return vec4(dot(v4, kRedVec4) + dot(v2, kRedVec2),
        dot(v4, kGreenVec4) + dot(v2, kGreenVec2),
        dot(v4, kBlueVec4) + dot(v2, kBlueVec2), 1);
}

uniform uvec2 halfScreenSize;
uniform vec3 cameraPos;
uniform int samples;
uniform uint currentFrame;
in vec2 uv;
uniform sampler2D lastFrame;
uniform float debugMaxTriangleIntersections;
uniform float debugMaxAABBIntersections;

void main() {
    uvec2 FragCoord = uvec2(gl_FragCoord.xy * halfScreenSize) * 2;
    rngState = FragCoord.y * halfScreenSize.x * 2 + FragCoord.x + currentFrame * 719393u;

    Ray ray = {cameraPos, normalize(originalRayDir)};

    vec3 rayColour = vec3(0.0);
    for (uint i = 0; i < samples; i++) {
        rayColour += trace(ray);
    }
    rayColour /= samples;

    vec3 accumulatedColour = texture(lastFrame, uv).rgb;
    float weight = 1.0 / float(currentFrame + 1u);
    FragColor = vec4(mix(accumulatedColour, rayColour, weight), 1.0);

    if (displayDebug > 0) {
        FragColor = turbo_color_map(debugColour.r / debugMaxTriangleIntersections + debugColour.b / debugMaxAABBIntersections);
//        FragColor = vec4(debugColour.r / debugMaxTriangleIntersections, 0.0, debugColour.b / debugMaxAABBIntersections, 1.0);
//        if (max(FragColor.r, FragColor.b) > 1.0) FragColor = vec4(1.0);
//        FragColor = vec4(debugColour, 1.0);
    }
}
