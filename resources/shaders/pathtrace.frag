#version 460 core

in vec3 originalRayDir;

out vec4 FragColor;

struct Ray {
    vec3 origin;
    vec3 dir;
};

struct Material {
    vec3 colour;
    float padding;
    vec3 emissionColour;
    float emissionStrength;
};

struct Triangle {
    vec4 a;
    vec4 b;
    vec4 c;
    Material material;
};

struct HitRecord {
    bool hit;
    float t;
    vec3 pos;
    vec3 normal;
    Material material;
};

uniform uint triangleCount;
layout (std430, binding = 0) buffer TriangleBuffer {
    Triangle triangles[];
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
vec3 randomSphere() {
    float z = 1 - 2 * randomUniform();
    float r = sqrt(1 - z*z);
    float phi = 2 * 3.1415926 * randomUniform();
    return vec3(r * cos(phi), r * sin(phi), z);
}
vec3 randomCosineHemisphere(vec3 normal) {
    vec3 dir = randomSphere();
    return dir * sign(dot(normal, dir));
}

HitRecord intersectTriangle(Ray ray, Triangle triangle) {
    HitRecord record;
    record.hit = false;

    vec3 edge1 = triangle.b.xyz - triangle.a.xyz;
    vec3 edge2 = triangle.c.xyz - triangle.a.xyz;

    vec3 normal = normalize(cross(edge1, edge2));
    if (dot(normal, ray.dir) > 0) return record;

    vec3 ray_cross_e2 = cross(ray.dir, edge2);
    float det = dot(edge1, ray_cross_e2);

    if (abs(det) < 0) return record;

    float inv_det = 1.0 / det;
    vec3 s = ray.origin - triangle.a.xyz;
    float u = inv_det * dot(s, ray_cross_e2);

    if (u < -0 || u - 1 > 0) return record;

    vec3 s_cross_e1 = cross(s, edge1);
    float v = inv_det * dot(ray.dir, s_cross_e1);

    if (v < 0 || u + v - 1 > 0) return record;

    float t = inv_det * dot(edge2, s_cross_e1);

    if (t > 0) {
        record.hit = true;
        record.t = t;
        record.pos = ray.origin + ray.dir * t;
        record.normal = normal;
        record.material = triangle.material;
    }
    return record;
}

HitRecord intersectScene(Ray ray) {
    HitRecord closestRecord;
    closestRecord.hit = false;
    float closestT = 1.0 / 0.0;

    for (uint i = 0; i < triangleCount; i++) {
        HitRecord record = intersectTriangle(ray, triangles[i]);
        if (record.hit && record.t < closestT) {
            closestRecord = record;
            closestT = record.t;
        }
    }

    return closestRecord;
}

vec3 getSkybox(Ray ray) {
    float a = 0.5*(ray.dir.y + 1.0);
    return mix(vec3(1.0, 1.0, 1.0), vec3(0.5, 0.7, 1.0), a);
}

uniform uint maxBounces;

vec3 trace(Ray cameraRay) {
    Ray ray = cameraRay;
    vec3 incomingLight = vec3(0.0);
    vec3 rayColour = vec3(1.0);

    for (uint bounce = 0; bounce <= maxBounces; bounce++) {
        HitRecord record = intersectScene(ray);

        if (record.hit) {
            ray.origin = record.pos + record.normal * 0.001;
            ray.dir = randomCosineHemisphere(record.normal);

            incomingLight += (record.material.emissionColour * record.material.emissionStrength) * rayColour;
            rayColour *= record.material.colour;
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
