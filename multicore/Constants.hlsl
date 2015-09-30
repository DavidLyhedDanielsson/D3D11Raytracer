static const int MAX_SPHERES = 64; 
static const int MAX_TRIANGLES = 64;
static const int MAX_LIGHTS = 10;

static const float FLOAT_MAX = 3.4e38f;

const static float3 LIGHT_DIR = normalize(float3(-0.5f, 0.5f, -0.5f));

#define Sample(id, tex) tex.SampleLevel(textureSampler, threadID.xy / float2(1280.0f, 720.0f) + float2(1.0f / 2560.0f, 1.0f / 1440.0f), 0)