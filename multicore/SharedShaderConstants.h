#ifndef SharedShaderConstants_h__
#define SharedShaderConstants_h__

static const int MAX_SPHERES = 64;
const static int MAX_POINT_LIGHTS = 10;

const static int MAX_TRIANGLES = 512;
const static int MAX_VERTICES = MAX_TRIANGLES * 3;
const static int MAX_INDICIES = MAX_TRIANGLES * 3;

static const float FLOAT_MAX = 3.4e38f;

static const float SCREEN_RES_X = 1280.0f;
static const float SCREEN_RES_Y = 720.0f;

#endif // SharedShaderConstants_h__