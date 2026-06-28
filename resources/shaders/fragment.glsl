#version 330 core

in vec3 originalRayDir;

out vec4 FragColor;

struct Ray {
    vec3 origin;
    vec3 dir;
};

struct Sphere {
    vec3 origin;
    float radius;
};

struct HitRecord {
    bool hit;
    float t;
    vec3 pos;
    vec3 normal;
};

HitRecord intersectSphere(Ray ray, Sphere sphere) {
    HitRecord record;
    vec3 offsetRayOrigin = ray.origin - sphere.origin;
    float a = dot(ray.dir, ray.dir);
    float b = 2 * dot(offsetRayOrigin, ray.dir);
    float c = dot(offsetRayOrigin, offsetRayOrigin) - sphere.radius * sphere.radius;

    float discriminant = b * b - 4 * a * c;

    if (discriminant >= 0) {
        float dist = (-b - sqrt(discriminant)) / (2 * a);
        if (dist >= 0) {
            record.hit = true;
            record.t = dist;
            record.pos = ray.origin + ray.dir * dist;
            record.normal = normalize(record.pos - sphere.origin);
        }
    }
    return record;
}

void main() {
    FragColor = vec4(normalize(originalRayDir), 1.0);

    Ray ray = {vec3(-0.5, 1.0, 3.0), normalize(originalRayDir)};

    Sphere sphere = {vec3(0.0, 0.0, 0.0), 1.0};

    HitRecord record = intersectSphere(ray, sphere);

    if (record.hit) {
        FragColor = vec4(1.0);
    }
}
