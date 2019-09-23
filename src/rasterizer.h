#ifndef RASTERIZER_H
#define RASTERIZER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL2/SDL.h>
#include "mesh.h"

#define COLOR_BIT 1
#define DEPTH_BIT 2

#define FLAT_SHADING 1
#define GOURAUD_SHADING 2
#define PHONG_SHADING 3

#define TEX_COORD_CLAMP 1
#define TEX_COORD_REPEAT 2

struct Texture
{
    Uint8 *data;
    Uint32 width;
    Uint32 height;
};

struct Rasterizer
{
    Uint32 *frameBuffer;
    float *depthBuffer;
    Uint32 width;
    Uint32 height;
    Texture texture;

    glm::vec3 clearColor;
    bool backFaceCulling;
    float zNear;
};

namespace Rasterization
{
    void Init(Rasterizer *rasterizer, Uint32 width, Uint32 height, float zNear);
    void Clear(Rasterizer *rasterizer, Uint32 flags);
    void Release(Rasterizer *rasterizer);
    void Resize(Rasterizer *rasterizer, Uint32 newWidth, Uint32 newHeight);

    void SetTexture(Rasterizer *rasterizer, Texture *texture);

    void DrawTriangleMesh(Rasterizer *rasterizer, Mesh *mesh);
    void DrawLineMesh(Rasterizer *rasterizer, Mesh *mesh);
}

// Shader uniforms
namespace U
{
    extern glm::mat4 modelMatrix;
    extern glm::mat4 viewMatrix;
    extern glm::mat4 mvpMatrix;
    extern glm::vec3 worldCameraPosition;
    extern glm::vec3 worldLightDirection;
    extern glm::vec3 worldLightPosition;
    extern bool directionalLightOn;
    extern bool sunMesh;
    extern int shading;
    extern bool texturingOn;
    extern int shininess;
    extern int texCoordWrap;
}

#endif
