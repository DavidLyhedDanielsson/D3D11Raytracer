static const int MAX_SPHERES = 64; 
static const int MAX_LIGHTS = 10;

const static int MAX_TRIANGLES = 256;
const static int MAX_VERTICES = MAX_TRIANGLES * 3;
const static int MAX_INDICIES = MAX_TRIANGLES * 3;

static const float FLOAT_MAX = 3.4e38f;

static const float SCREEN_RES_X = 1280.0f;
static const float SCREEN_RES_Y = 720.0f;

struct Sphere
{
	float3 position;
	float radius;
	float4 color;
};

struct Vertex
{
	float3 position;
	float padding;
};

struct Triangle
{
	int3 indicies;
	float4 color;
	float padding;
};