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
    sampler2D albedoTextureHandle;
    sampler2D roughnessTextureHandle;
    sampler2D metalnessTextureHandle;
    sampler2D normalTextureHandle;
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

struct Model {
    uint triangleIndex;
    uint triangleCount;
    uint padding[2];
    vec3 offset;
    float scale;
    Material material;
};

struct HitRecord {
    bool hit;
    float t;
    vec3 pos;
    vec3 normal;
    vec2 uv;
    Material material;
};

uniform uint triangleCount;
layout (std430, binding = 0) buffer TriangleBuffer {
    Triangle triangles[];
};

uniform uint modelCount;
layout (std430, binding = 1) buffer ModelBuffer {
    Model models[];
};

vec3 safeNormalize(vec3 vector, vec3 fallback) {
    vec3 normalized = normalize(vector);
    return normalized == vec3(0.0) ? fallback : normalized;
}

uint rngState;
float randomUniform() {
    rngState = rngState * 747796405u + 2891336453u;
    uint result = ((rngState >> ((rngState >> 28u) + 4u)) ^ rngState) * 277803737u;
    result = (result >> 22u) ^ result;
    return result / 4294967294.0;
}
vec3 sampleCosineHemisphere() {
    float phi = 2.0f * 3.1415926 * randomUniform();

    float z = sqrt(randomUniform());
    float sinTheta = sqrt(clamp(1.0f - z * z, 0.0f, 1.0f));
    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);

    return vec3(x, y, z);
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

    if (abs(det) < 0) return record;

    float inv_det = 1.0 / det;
    vec3 s = ray.origin - triangle.aPosition;
    float u = inv_det * dot(s, ray_cross_e2);

    if (u < 0 || u - 1 > 0) return record;

    vec3 s_cross_e1 = cross(s, edge1);
    float v = inv_det * dot(ray.dir, s_cross_e1);

    if (v < 0 || u + v - 1 > 0) return record;

    float t = inv_det * dot(edge2, s_cross_e1);

    float w = 1.0 - u - v;

    vec2 uv = vec2(triangle.aU, triangle.aV) * w + vec2(triangle.bU, triangle.bV) * u + vec2(triangle.cU, triangle.cV) * v;

    float alpha = opacity;
    if (uvec2(textureHandle) != uvec2(0)) {
        alpha = texture(textureHandle, uv).a;
    }
    if (randomUniform() >= alpha) return record;

    normal = normalize(triangle.aNormal * w + triangle.bNormal * u + triangle.cNormal * v);

    if (t > 0) {
        record.hit = true;
        record.t = t;
        record.pos = ray.origin + ray.dir * t;
        record.normal = normal;
        record.uv = uv;
    }
    return record;
}

HitRecord intersectScene(Ray ray) {
    HitRecord closestRecord;
    closestRecord.hit = false;
    float closestT = 1.0 / 0.0;

    for (uint i = 0; i < modelCount; i++) {
        Model model = models[i];

        Ray localRay = ray;
        localRay.origin -= model.offset;
        localRay.origin /= model.scale;
        localRay.dir /= model.scale;

        for (uint i = model.triangleIndex; i < model.triangleIndex+model.triangleCount; i++) {
            HitRecord record = intersectTriangle(localRay, triangles[i], model.material.opacity, model.material.albedoTextureHandle);
            if (record.hit && record.t < closestT) {
                closestRecord = record;
                closestRecord.pos *= model.scale;
                closestRecord.pos += model.offset;
                closestRecord.material = model.material;
                closestT = record.t;
            }
        }
    }

    return closestRecord;
}

uniform uint skyboxFormat;
uniform samplerCube skyboxCubemapTexture;
uniform sampler2D skyboxEquirectangularTexture;
vec3 getSkybox(Ray ray) {
//    return vec3(1.0);
    if (skyboxFormat == 0) {
        float a = 0.5*(ray.dir.y + 1.0);
        return mix(vec3(1.0, 1.0, 1.0), vec3(0.5, 0.7, 1.0), a);
    } else if (skyboxFormat == 1) {
        return pow(texture(skyboxCubemapTexture, ray.dir).rgb, vec3(2.2));
    } else if (skyboxFormat == 2) {
        float u = atan(ray.dir.z, ray.dir.x) * 0.5 / 3.1415926 + 0.5;
        float v = asin(ray.dir.y) / 3.1415926 + 0.5;
        return pow(texture(skyboxEquirectangularTexture, vec2(u, v)).rgb, vec3(2.2));
    } else {
        return vec3(0.0);
    }
}

float fresnelReflection(vec3 wi, vec3 normal, float etaOutside, float etaInside) {
    float cosThetaI = dot(wi, normal);
    float etaI = etaOutside;
    float etaT = etaInside;
    if (cosThetaI <= 0.0) {
        etaI = etaInside;
        etaT = etaOutside;
        cosThetaI = -cosThetaI;
    }

    float sinThetaI = sqrt(max(0.0, 1.0 - cosThetaI * cosThetaI));
    float sinThetaT = etaI / etaT * sinThetaI;
    if (sinThetaT >= 1.0) return 1.0;
    float cosThetaT = sqrt(max(0.0, 1.0 - sinThetaT * sinThetaT));

    float Rparl =   ((etaT * cosThetaI) - (etaI * cosThetaT)) /
                    ((etaT * cosThetaI) + (etaI * cosThetaT));
    float Rperp =   ((etaI * cosThetaI) - (etaT * cosThetaT)) /
                    ((etaI * cosThetaI) + (etaT * cosThetaT));
    return (Rparl * Rparl + Rperp * Rperp) / 2;
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
    if (alpha.x < 0.001 && alpha.y < 0.001) return vec3(0.0, 0.0, 1.0);
    if (wi.z < 0.0) wi = -wi;
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

void sampleOutgoingReflection(inout Ray ray, HitRecord record, out vec3 rayTint) {
    Material material = record.material;
    vec3 albedo = uvec2(material.albedoTextureHandle) == uvec2(0) ?
        material.albedo :
        pow(texture(material.albedoTextureHandle, record.uv).rgb, vec3(2.2));
    float roughness = uvec2(material.roughnessTextureHandle) == uvec2(0) ?
        material.roughness :
        texture(material.roughnessTextureHandle, record.uv).r;
    float metalness = uvec2(material.metalnessTextureHandle) == uvec2(0) ?
        material.metalness :
        texture(material.metalnessTextureHandle, record.uv).r;
    vec3 textureN = uvec2(material.normalTextureHandle) == uvec2(0) ?
        vec3(0.0, 0.0, 1.0) :
        normalize(texture(material.normalTextureHandle, record.uv).rgb * 2.0 - 1.0);
    float transmission = //uvec2(material.transmissionTextureHandle) == uvec2(0) ?
        material.transmission //:
        //texture(material.transmissionTextureHandle, record.uv).r
    ;

    vec3 N = record.normal;
    vec3 T, B;
    frisvad(N, T, B);
    vec3 wiWorld = -ray.dir;
    vec3 wiTangent = normalize(vec3(dot(wiWorld, T), dot(wiWorld, B), dot(wiWorld, N)));

    bool normalMapping = textureN != vec3(0.0, 0.0, 1.0);
    vec3 textureT, textureB;
    if (normalMapping) {
        textureN = normalize(textureN.x * T + textureN.y * B + textureN.z * N);
        frisvad(textureN, textureT, textureB);
        wiTangent = normalize(vec3(dot(wiWorld, textureT), dot(wiWorld, textureB), dot(wiWorld, textureN)));
    }

    /*
    float iorOutside = wiTangent.z > 0.0 ? 1.0 : material.ior;
    float iorInside = wiTangent.z > 0.0 ? material.ior : 1.0;

    vec3 microsurfaceNormal = sampleGgxVndfNormal(wiTangent, pow(vec2(roughness), vec2(2.0)));
    if (wiTangent.z < 0.0) microsurfaceNormal = -microsurfaceNormal;
    float reflectionFraction = fresnelReflection(wiTangent, microsurfaceNormal, iorOutside, iorInside);

    vec3 specularDirection = reflect(-wiTangent, microsurfaceNormal);
    vec3 diffuseDirection = sampleCosineHemisphere();
    if (wiTangent.z < 0.0) diffuseDirection = -diffuseDirection;
    vec3 transmissionDirection = refract(-wiTangent, microsurfaceNormal, iorOutside/iorInside);

    vec3 woTangent;
    if (randomUniform() < metalness) {
        woTangent = specularDirection;
        rayTint = albedo;
    } else {
        if (randomUniform() < reflectionFraction) {
            woTangent = specularDirection;
            rayTint = vec3(1.0);
        } else {
            if (randomUniform() < transmission) {
                woTangent = transmissionDirection;
                rayTint = albedo;
            } else {
                woTangent = diffuseDirection;
                rayTint = albedo;
            }
        }
    }

//    if (textureNormal) {
//        woTangent = normalize(woTangent.x * textureT + woTangent.y * textureB + woTangent.z * textureN);
//    }
    */

    float reflectionFraction = fresnelReflection(wiTangent, vec3(0.0, 0.0, 1.0), 1.0, material.ior);

    vec3 woTangent;
    if (randomUniform() < reflectionFraction) {
        woTangent = reflectBetter(wiTangent, vec3(0.0, 0.0, 1.0));
        rayTint = vec3(1.0);
    } else {
        if (randomUniform() < transmission) {
            woTangent = refractBetter(wiTangent, vec3(0.0, 0.0, 1.0), 1.0, material.ior);
            rayTint = albedo;
        } else {
            woTangent = sampleCosineHemisphere();
            rayTint = albedo;
        }
    }

    vec3 woWorld = normalize(woTangent.x * T + woTangent.y * B + woTangent.z * N);

    if (normalMapping) {
        woWorld = normalize(woTangent.x * textureT + woTangent.y * textureB + woTangent.z * textureN);
    }

    if (dot(woWorld, N) >= 0.0)
        ray.origin = record.pos + N * 0.001;
    else
        ray.origin = record.pos - N * 0.001;

    ray.dir = woWorld;

//    ray.dir = vec3(0.0, 1.0, 0.0);
//    rayTint = textureN;
}

uniform uint maxBounces;
vec3 trace(Ray cameraRay) {
    Ray ray = cameraRay;
    vec3 incomingLight = vec3(0.0);
    vec3 rayColour = vec3(1.0);

    for (uint bounce = 0; bounce <= maxBounces; bounce++) {
        HitRecord record = intersectScene(ray);

        if (record.hit) {
            vec3 rayTint;
            sampleOutgoingReflection(ray, record, rayTint);

            incomingLight += (record.material.emissionColour * record.material.emissionStrength) * rayColour;
            rayColour *= rayTint;
            if (rayColour == vec3(0.0)) break;
        } else {
            incomingLight += getSkybox(ray) * rayColour;
            break;
        }
    }

    return incomingLight;
}

uniform uvec2 halfScreenSize;
uniform vec3 cameraPos;
uniform uint samples;
uniform uint currentFrame;
in vec2 uv;
uniform sampler2D lastFrame;

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
}
