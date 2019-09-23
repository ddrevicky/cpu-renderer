#include <algorithm>
#include <vector>
#include <SDL2/SDL.h>
#include "rasterizer.h"
#include "renderer.h"
#include "main.h"
#include "common.h"
#include <vector>
#include <assert.h>

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;
using std::min;
using std::max;
using glm::normalize;
using glm::clamp;

mat4 U::modelMatrix;
mat4 U::viewMatrix;
mat4 U::mvpMatrix;
vec3 U::worldCameraPosition;
vec3 U::worldLightDirection;
vec3 U::worldLightPosition;
bool U::directionalLightOn;
bool U::texturingOn;
bool U::sunMesh;
int U::shading;
int U::shininess;
int U::texCoordWrap;

float EdgeFunction(vec4 &v0, vec4 &v1, vec2 &p);
void SwapVec4(vec4 &a, vec4 &b);
Uint32 Vec3ColorToUint32(vec3 col);
void RasterizeLines(Rasterizer *rasterizer, Mesh *mesh);
void VertexShader(Vertex &vertex);
void RasterizeTriangles(Rasterizer *rasterizer, Mesh *mesh);
void RasterizeLines(Rasterizer *rasterizer, Mesh *mesh);
vec3 SampleTexture(Rasterizer *rasterizer, Uint32 u, Uint32 v);

void Rasterization::Init(Rasterizer *rasterizer, Uint32 width, Uint32 height, float zNear)
{
    rasterizer->frameBuffer = (Uint32*)malloc(width * height * sizeof(Uint32));
    rasterizer->depthBuffer = (float*)malloc(width * height * sizeof(float));
    rasterizer->width = width;
    rasterizer->height = height;
    rasterizer->backFaceCulling = true;
    rasterizer->zNear = -zNear;
}

void Rasterization::SetTexture(Rasterizer *rasterizer, Texture *texture)
{
    rasterizer->texture.width = texture->width;
    rasterizer->texture.height = texture->height;
    rasterizer->texture.data = (Uint8*)malloc(texture->width * texture->height * sizeof(Uint8));
    memcpy(rasterizer->texture.data, texture->data, texture->width * texture->height * sizeof(Uint8));
}

void Rasterization::Resize(Rasterizer *rasterizer, Uint32 width, Uint32 height)
{
    if (rasterizer)
    {
        free(rasterizer->frameBuffer);
        free(rasterizer->depthBuffer);
        rasterizer->width = width;
        rasterizer->height = height;

        rasterizer->frameBuffer = (Uint32*)malloc(width * height * sizeof(Uint32));
        rasterizer->depthBuffer = (float*)malloc(width * height * sizeof(float));
    }
}

void Rasterization::Clear(Rasterizer *rasterizer, Uint32 flags)
{
    if (flags & COLOR_BIT)
    {
        Uint32 bytesPerPixel = 4;
        memset(rasterizer->frameBuffer, Vec3ColorToUint32(rasterizer->clearColor), rasterizer->width * rasterizer->height * bytesPerPixel);
    }
    if (flags & DEPTH_BIT)
    {
        std::fill(rasterizer->depthBuffer, rasterizer->depthBuffer + (rasterizer->width * rasterizer->height), FLT_MAX);
    }
}

void Rasterization::Release(Rasterizer *rasterizer)
{
    free(rasterizer->frameBuffer);
    free(rasterizer->depthBuffer);
    free(rasterizer->texture.data);
}

void Rasterization::DrawLineMesh(Rasterizer *rasterizer, Mesh *mesh)
{
    for (Uint32 i = 0; i < mesh->vertexCount; ++i)
    {
        vec4 &vertexPos = mesh->vertices[i].position;

        // Clip space - Model(object) space -> World space -> View(camera space -> Perspective projection
        vertexPos = U::mvpMatrix * vertexPos;

        // Normalized Device Coordinates - (Perspective) Division by the w coordinate (OpenGL does this for us after the VS)
        vertexPos.x /= vertexPos.w;
        vertexPos.y /= vertexPos.w;
        vertexPos.z /= vertexPos.w;

        // Viewport transform (from NDC = [-1, 1] to [0, width -1 or height - 1]
        vertexPos.x = (vertexPos.x * 0.5f + 0.5f) * float(rasterizer->width);
        vertexPos.y = (vertexPos.y * -0.5f + 0.5f) * float(rasterizer->height);
    }
    RasterizeLines(rasterizer, mesh);
}

// Interpolate all atributes using t
static Vertex LerpVertices(Vertex v0, Vertex v1, float t)
{
    Vertex result = {};

    result.position = v0.position + t * (v1.position - v0.position);
    result.position.w = 1.0f;
    result.normal = v0.normal + t * (v1.normal - v0.normal);
    result.textureCoords = v0.textureCoords + t * (v1.textureCoords - v0.textureCoords);
    result.vsOutColor = v0.vsOutColor + t * (v1.vsOutColor - v0.vsOutColor);

    return result;
}

static Uint32 OutCode(vec3 position, float zn)
{
    const Uint32 INSIDE = 0;
    const Uint32 OUTSIDE = 1;

    Uint32 outCode = INSIDE;

    if (position.z > zn)
        outCode |= OUTSIDE;

    return outCode;
}

static void Clip2Vertices(Uint32 codes[3], Vertex *inTriangle, Mesh *outMesh, float zn)
{
    /*
   a|\  | zn
    |  \|
    |	\
    |	|\
    |	| \  notClipped
    |	| /
    |	|/
    |  /|
   b|/  |
    */  

    Vertex notClipped = {};	// Vertex that will not be clipped
    Vertex clippedA = {}; // Vertices that will be clipped
    Vertex clippedB = {};

    // Preserve CCW order
    if (codes[1] & codes[2]) // v0 inside
    {
        notClipped = inTriangle[0];
        clippedA = inTriangle[1];
        clippedB = inTriangle[2];
    }
    if (codes[0] & codes[2]) // v1 inside
    {
        notClipped = inTriangle[1];
        clippedA = inTriangle[2];
        clippedB = inTriangle[0];
    }
    if (codes[0] & codes[1]) // v2 inside
    {
        notClipped = inTriangle[2];
        clippedA = inTriangle[0];
        clippedB = inTriangle[1];
    }

    vec3 ncPos = notClipped.position;
    vec3 aPos = clippedA.position;
    vec3 bPos = clippedB.position;

    float tA = (aPos.z - zn) / (aPos.z - ncPos.z);
    float tB = (bPos.z - zn) / (bPos.z - ncPos.z);

    Vertex aToNotClipped = LerpVertices(clippedA, notClipped, tA);
    Vertex bToNotClipped = LerpVertices(clippedB, notClipped, tB);

    UtilMesh::AddTriangle(outMesh, notClipped, aToNotClipped, bToNotClipped);
}

static void Clip1Vertex(Uint32 codes[3], Vertex *inTriangle, Mesh *outMesh, float zn)
{
    std::vector<Vertex> outVert;

    // Ordering is always: a, clipped, b
    Vertex notClippedA = {};
    Vertex clipped = {};
    Vertex notClippedB = {};

    /*
          |  a
          |/|
clipped  /| |
         \| |
          |\|
          |  b
    */

    if (codes[0]) // v0 outside
    {
        notClippedA = inTriangle[2];
        clipped = inTriangle[0];
        notClippedB = inTriangle[1];
    }
    if (codes[1]) // v1 outside
    {
        notClippedA = inTriangle[0];
        clipped = inTriangle[1];
        notClippedB = inTriangle[2];
    }
    if (codes[2]) // v2 outside
    {
        notClippedA = inTriangle[1];
        clipped = inTriangle[2];
        notClippedB = inTriangle[0];
    }

    vec3 aPos = notClippedA.position;
    vec3 bPos = notClippedB.position;
    vec3 clippedPos = clipped.position;

    float tA = (clippedPos.z - zn) / (clippedPos.z - aPos.z);		// zn should be negative
    float tB = (clippedPos.z - zn) / (clippedPos.z - bPos.z);
    
    // Newly spawned vertices
    Vertex clippedA = LerpVertices(clipped, notClippedA, tA);
    Vertex clippedB = LerpVertices(clipped, notClippedB, tB);

    UtilMesh::AddTriangle(outMesh, notClippedA, clippedA, notClippedB);
    UtilMesh::AddTriangle(outMesh, notClippedB, clippedA, clippedB);
}

static void ClipTriangle(Vertex *inTriangle, Mesh *outMesh, float zn)
{
    Uint32 codes[3] = {};

    codes[0] = OutCode(inTriangle[0].position, zn);
    codes[1] = OutCode(inTriangle[1].position, zn);
    codes[2] = OutCode(inTriangle[2].position, zn);

    if (!(codes[0] | codes[1] | codes[2])) // Trivial accept
    {
        UtilMesh::AddTriangle(outMesh, inTriangle[0], inTriangle[1], inTriangle[2]);
        return;
    }

    if (codes[0] & codes[1] & codes[2]) // Trivial reject
        return;

    // 2 vertices outside
    if (codes[0] & codes[1] || codes[0] & codes[2] || codes[1] & codes[2])
        return Clip2Vertices(codes, inTriangle, outMesh, zn);

    // 1 vertex outside (generate 2 new triangles)
    return Clip1Vertex(codes, inTriangle, outMesh, zn);
}

static void ClipToNear(Mesh *mesh, float zNear)
{
    mat4 modelView = U::viewMatrix * U::modelMatrix;
    mat4 inverseModelView = glm::inverse(U::viewMatrix * U::modelMatrix);
    
    // To View Space
    for (Uint32 i = 0; i < mesh->vertexCount; ++i)
    {	
        mesh->vertices[i].position = modelView * mesh->vertices[i].position;
    }

    // Worst case scenario - result has 2 times as many vertices
    Mesh clippedMesh = {};
    clippedMesh.isTexturable = mesh->isTexturable;
    clippedMesh.vertices = (Vertex*)malloc(2 * mesh->vertexCount * sizeof(Vertex));
    clippedMesh.vertexCount = 0;

    Vertex *firstTriangleVertex = mesh->vertices;
    for (Uint32 i = 0; i < mesh->vertexCount; i += 3)
    {
        ClipTriangle(firstTriangleVertex, &clippedMesh, zNear);
        firstTriangleVertex += 3;
    }

    // Back to Object Space
    for (Uint32 i = 0; i < clippedMesh.vertexCount; ++i)
    {
        clippedMesh.vertices[i].position = inverseModelView * clippedMesh.vertices[i].position;
    }

    UtilMesh::Release(*mesh);
    *mesh = clippedMesh;
}

void Rasterization::DrawTriangleMesh(Rasterizer *rasterizer, Mesh *original)
{
    Mesh mesh = UtilMesh::MakeMeshCopy(original);
    ClipToNear(&mesh, rasterizer->zNear);

    for (Uint32 i = 0; i < mesh.vertexCount; ++i)
    {
        VertexShader(mesh.vertices[i]);
        
        vec4 &vertexPos = mesh.vertices[i].position;

        // Normalized Device Coordinates - (Perspective) Division by the w coordinate (OpenGL does this for us after the VS)
        vertexPos.x /= vertexPos.w;
        vertexPos.y /= vertexPos.w;
        vertexPos.z /= vertexPos.w;

        // Viewport transform (from NDC = [-1, 1] to [0, width -1 or height - 1]
        vertexPos.x = (vertexPos.x * 0.5f + 0.5f) * float(rasterizer->width);
        vertexPos.y = (vertexPos.y * -0.5f + 0.5f) * float(rasterizer->height);
    }

    RasterizeTriangles(rasterizer, &mesh);
    UtilMesh::Release(mesh);
}

static void VertexShader(Vertex &vertex)
{
    // NOTE: Should really use a inverse transpose matrix to transform normals, but it is not necessary in our case (we don't have non-uniform scaling)
    if (U::shading == FLAT_SHADING || U::shading == GOURAUD_SHADING)
    {
        // Light is computed in world space coordinates. Vectors and positions have to be transformed by the model matrix.

        // Phong reflection model
        vec4 worldPos = U::modelMatrix * vertex.position;
        vec3 worldNormal = U::modelMatrix * vec4(vertex.normal, 0.0f);
        vec3 N = normalize(worldNormal);
        vec3 V = normalize(U::worldCameraPosition - vec3(worldPos));

        vec3 albedo = vertex.vsOutColor;
        vec3 specColor = vec3(1.0f, 1.0f, 1.0f);
        float ambient = 0.2f;
        vec3 L;
        if (U::directionalLightOn)
        {
            L = U::worldLightDirection;
        }
        else // Solar system
        {
            L = normalize(worldPos - vec4(U::worldLightPosition, 1.0f));
            if (U::sunMesh)
            {
                L = -V;
                ambient += 0.4f;
            }
            specColor = vec3(0, 0, 0);
        }

        float NLdot = max(dot(-L, N), 0.0f);
        float diffuse = NLdot;

        vec3 R = normalize(glm::reflect(L, N));
        float specular = NLdot * pow(max(dot(R, V), 0.0f), U::shininess);
        vec3 shadedColor = (ambient + diffuse) * albedo + specular * specColor;

        // Clamp and set value
        vertex.vsOutColor = clamp(shadedColor, 0.0f, 1.0f);
    }
    else // PHONG_SHADING
    {
        vertex.vsOutWorldPos = U::modelMatrix * vertex.position;
        vertex.vsOutWorldNormal = U::modelMatrix * vec4(vertex.normal, 0.0f);
        vertex.vsOutColor = vertex.vsOutColor;
    }

    vertex.position = U::mvpMatrix * vertex.position;
}

static void RasterizeTriangles(Rasterizer *rasterizer, Mesh *mesh)
{
    unsigned width = rasterizer->width;
    unsigned height = rasterizer->height;
    for (unsigned i = 0; i < mesh->vertexCount; i += 3)	// 3 vertices per triangle
    {
        // Triangle vertices (positions)
        vec4 v0 = mesh->vertices[i].position;
        vec4 v1 = mesh->vertices[i + 1].position;
        vec4 v2 = mesh->vertices[i + 2].position;

        vec2 pointC = vec2(v2.x, v2.y);
        float triangleArea = EdgeFunction(v0, v1, pointC);
        // EdgeFunction based backface culling
        if (rasterizer->backFaceCulling && triangleArea < 0)
            continue;

        // Iterate just over triangle Minimum Bounding Box
        Sint32 minX = (Sint32)clamp(min(v0.x, min(v1.x, v2.x)), 0.0f, float(width - 1));
        Sint32 maxX = (Sint32)clamp(max(v0.x, max(v1.x, v2.x)), 0.0f, float(width - 1));
        Sint32 minY = (Sint32)clamp(min(v0.y, min(v1.y, v2.y)), 0.0f, float(height - 1));
        Sint32 maxY = (Sint32)clamp(max(v0.y, max(v1.y, v2.y)), 0.0f, float(height - 1));

        const float x0 = v0.x;
        const float x1 = v1.x;
        const float x2 = v2.x;

        const float y0 = v0.y;
        const float y1 = v1.y;
        const float y2 = v2.y;

        // Edge equation:
        // (x0-x1)(y-y0) - (y0-y1)(x-x0)

        /* These terms are constant for each triangle so it's enough to calculate them once for all pixels.
        Then for every movement right in the x direction we have to add (y0 - y1), and for every movement
        down in the y direction we add (x0 - x1) (See the edge equation above). */
        // x0 - x1
        const float e0_diffX = x0 - x1;
        const float e1_diffX = x1 - x2;
        const float e2_diffX = x2 - x0;

        // y0 - y1
        const float e0_diffY = y0 - y1;
        const float e1_diffY = y1 - y2;
        const float e2_diffY = y2 - y0;

        float e0_y = e0_diffX * (minY - y0) - e0_diffY * (minX - x0);
        float e1_y = e1_diffX * (minY - y1) - e1_diffY * (minX - x1);
        float e2_y = e2_diffX * (minY - y2) - e2_diffY * (minX - x2);

        // Precompute interpolation constants
        float v0RecW = (1.0f / v0.w);
        float v1RecW = (1.0f / v1.w);
        float v2RecW = (1.0f / v2.w);

        unsigned *frameBuffer = rasterizer->frameBuffer + minY * width;

        for (Sint32 y = minY; y <= maxY; ++y)
        {
            float e0 = e0_y;
            float e1 = e1_y;
            float e2 = e2_y;

            for (Sint32 x = minX; x <= maxX; ++x)
            {
                float area0 = e0;
                float area1 = e1;
                float area2 = e2;

                unsigned col32 = 0;

                if (area0 >= 0 && area1 >= 0 && area2 >= 0 || (!rasterizer->backFaceCulling && area0 <= 0 && area1 <= 0 && area2 <= 0))
                {
                    // Barycentric coordinates
                    float w0 = area1 / triangleArea;
                    float w1 = area2 / triangleArea;
                    float w2 = area0 / triangleArea;

                    /*
                        See OpenGL Spec 4.4 page 427
                        Perspective correct vertex attribute interpolation for a fragment on a triangle
                        f = (w0*f0/v0.w + w1*f1/v1.w + w2*f2/v2.x) / 
                                (w0/v0.w + w1/v1.w + w2/v2.v)
                     */

                    // TODO: Move to the beginning, also remove division by W from code below
                    float recInterpolationDenominator = 1.0f / (w0 * v0RecW + w1 * v1RecW + w2 * v2RecW);

                    float depth = w0 * v0.z + w1 * v1.z + w2 * v2.z;
                    float &currentDepth = rasterizer->depthBuffer[y * width + x];

                    if (depth < 1.0f && depth < currentDepth)
                    {
                        currentDepth = depth;	// Depth write

                        /// Attribute Interpolation Block
                        // Color
                        vec3 &col0 = mesh->vertices[i].vsOutColor;
                        vec3 &col1 = mesh->vertices[i + 1].vsOutColor;
                        vec3 &col2 = mesh->vertices[i + 2].vsOutColor;

                        vec3 col;
                        col.r = (w0 * (col0.r * v0RecW)  + w1 * (col1.r * v1RecW) + w2 * (col2.r * v2RecW)) * recInterpolationDenominator;
                        col.g = (w0 * (col0.g * v0RecW)  + w1 * (col1.g * v1RecW) + w2 * (col2.g * v2RecW)) * recInterpolationDenominator;
                        col.b = (w0 * (col0.b * v0RecW)  + w1 * (col1.b * v1RecW) + w2 * (col2.b * v2RecW)) * recInterpolationDenominator;
                        
                        // World position (transformed by model matrix)
                        vec3 &v0Pos = mesh->vertices[i].vsOutWorldPos;
                        vec3 &v1Pos = mesh->vertices[i + 1].vsOutWorldPos;
                        vec3 &v2Pos = mesh->vertices[i + 2].vsOutWorldPos;

                        vec3 worldPos;
                        worldPos.x = (w0 * (v0Pos.x * v0RecW) + w1 * (v1Pos.x * v1RecW) + w2 * (v2Pos.x * v2RecW)) * recInterpolationDenominator;
                        worldPos.y = (w0 * (v0Pos.y * v0RecW) + w1 * (v1Pos.y * v1RecW) + w2 * (v2Pos.y * v2RecW)) * recInterpolationDenominator;
                        worldPos.z = (w0 * (v0Pos.z * v0RecW) + w1 * (v1Pos.z * v1RecW) + w2 * (v2Pos.z * v2RecW)) * recInterpolationDenominator;

                        // World normal (transformed by model matrix)
                        vec3 &v0Normal = mesh->vertices[i].vsOutWorldNormal;
                        vec3 &v1Normal = mesh->vertices[i + 1].vsOutWorldNormal;
                        vec3 &v2Normal = mesh->vertices[i + 2].vsOutWorldNormal;

                        vec3 worldNormal;
                        worldNormal.x = (w0 * (v0Normal.x * v0RecW) + w1 * (v1Normal.x * v1RecW) + w2 * (v2Normal.x * v2RecW)) * recInterpolationDenominator;
                        worldNormal.y = (w0 * (v0Normal.y * v0RecW) + w1 * (v1Normal.y * v1RecW) + w2 * (v2Normal.y * v2RecW)) * recInterpolationDenominator;
                        worldNormal.z = (w0 * (v0Normal.z * v0RecW) + w1 * (v1Normal.z * v1RecW) + w2 * (v2Normal.z * v2RecW)) * recInterpolationDenominator;

                        // Texture coords
                        vec2 &v0Tex = mesh->vertices[i].textureCoords;
                        vec2 &v1Tex = mesh->vertices[i + 1].textureCoords;
                        vec2 &v2Tex = mesh->vertices[i + 2].textureCoords;

                        vec2 texCoords;
                        texCoords.x = (w0 * (v0Tex.x * v0RecW) + w1 * (v1Tex.x * v1RecW) + w2 * (v2Tex.x * v2RecW)) * recInterpolationDenominator;
                        texCoords.y = (w0 * (v0Tex.y * v0RecW) + w1 * (v1Tex.y * v1RecW) + w2 * (v2Tex.y * v2RecW)) * recInterpolationDenominator;

                        if (U::texCoordWrap == TEX_COORD_CLAMP)
                        {
                            texCoords.x = clamp(texCoords.x, 0.0f, 1.0f);
                            texCoords.y = clamp(texCoords.y, 0.0f, 1.0f);

                        }
                        else // TEX_COORD_REPEAT
                        {	
                            texCoords.x = texCoords.x - floorf(texCoords.x);
                            texCoords.y = texCoords.y - floorf(texCoords.y);
                        }

                        unsigned u = (Uint32)(texCoords.x * (rasterizer->texture.width - 1));
                        Uint32 v = (Uint32)(texCoords.y * (rasterizer->texture.height - 1));
                        
						/// Fragment shader
                        if (U::shading == FLAT_SHADING)
                        {
                            col32 = Vec3ColorToUint32(col0);
                        }
                        else if (U::shading == GOURAUD_SHADING) 
                        {
                            // Alpha is not used
                            col32 = Uint8(col.b * 255.0f) << 16 | Uint8(col.g * 255.0f) << 8 | Uint8(col.r * 255.0f);
                        }
                        else // PHONG_SHADING
                        {
                            // Phong reflection model
                            
                            vec3 albedo = col;
                            if (U::texturingOn && mesh->isTexturable)
                            {
                                albedo = SampleTexture(rasterizer, u, v);
                            }
                            vec3 specColor = vec3(1.0f, 1.0f, 1.0f);

                            vec3 N = normalize(worldNormal);
                            vec3 V = normalize(U::worldCameraPosition - worldPos);
                            vec3 L;
                            float ambient = 0.2f;

                            if (U::directionalLightOn)
                            {
                                L = U::worldLightDirection;
                            }
                            else // Solar system
                            {
                                L = normalize(worldPos - U::worldLightPosition);
                                if (U::sunMesh)
                                {
                                    L = -V;
                                    ambient += 0.4f;
                                }
                                specColor = vec3(0, 0, 0);
                            }

                            float NLdot = max(dot(-L, N), 0.0f);
                            float diffuse = NLdot;

                            vec3 R = normalize(glm::reflect(L, N));
                            float specular = NLdot * pow(max(dot(R, V), 0.0f), U::shininess);
                            vec3 shadedColor = (ambient + diffuse) * albedo + specular * specColor;

                            // Clamp
                            shadedColor = clamp(shadedColor, 0.0f, 1.0f);
                            col32 = Vec3ColorToUint32(shadedColor);
                        }

                        frameBuffer[x] = col32;
                    }
                }
                e0 -= e0_diffY;
                e1 -= e1_diffY;
                e2 -= e2_diffY;
            }
            e0_y += e0_diffX;
            e1_y += e1_diffX;
            e2_y += e2_diffX;

            frameBuffer += width;
        }
    }
}

/*  For (CCW)CounterClockWise triangle vertex winding order
    Positive result indicates that the point is to the left of the edge formed by the vector (v1-v0) where v1 and v0 are the vertices
    of the triangle.A positive result for the point and all triangle edges means that the point is within the triangle. */
static float EdgeFunction(vec4 &v0, vec4 &v1, vec2 &p)
{
    float result = (v0.x - v1.x)*(p.y - v0.y) - (v0.y - v1.y)*(p.x - v0.x);	// Remapped because y is inverted in raster space
    return result;
}

static void SwapVec4(vec4 &a, vec4 &b)
{
    vec4 tmp = a;
    a = b;
    b = tmp;
}

static Uint32 Vec3ColorToUint32(vec3 col)
{
    Uint8 a8 = 0;
    Uint8 r8 = Uint8(col.r * 255.0f);
    Uint8 g8 = Uint8(col.g * 255.0f);
    Uint8 b8 = Uint8(col.b * 255.0f);

    unsigned col32 = a8 << 24 | b8 << 16 | g8 << 8 | r8;
    return col32;
}

// Only used for debugging, not a fully correct implementation
static void RasterizeLines(Rasterizer *rasterizer, Mesh *mesh)
{
    unsigned width = rasterizer->width;
    unsigned height = rasterizer->height;
    for (Uint32 i = 0; i < mesh->vertexCount; i += 2)	// 2 vertices per line
    {
        vec4 v0 = mesh->vertices[i].position;
        vec4 v1 = mesh->vertices[i + 1].position;
        Uint32 lineColor = Vec3ColorToUint32(mesh->vertices[i + 1].vsOutColor);

        if (v0.x > v1.x)
            SwapVec4(v0, v1);

        // Out of the screen
        if (v1.x < 0 || v0.x > width - 1)
        {
            continue;
        }

        float slope = (v1.y - v0.y) / (v1.x - v0.x);
        float b = v0.y - slope * v0.x;

        Uint32 minX = (Uint32)max(v0.x, 0.0f);
        Uint32 maxX = (Uint32)min(v1.x, float(width - 1));

        // y = a*slope + b
        float y = slope * minX + b;
        for (Uint32 x = minX; x < maxX; ++x)
        {
            if (y >= 0 && y < float(height))
                rasterizer->frameBuffer[Uint32(y) * width + x] = lineColor;
            y += slope;
        }
    }
}

static vec3 SampleTexture(Rasterizer *rasterizer, Uint32 u, Uint32 v)
{
    int coord = v * rasterizer->texture.width + u;
    Uint8 texColor = Uint8(rasterizer->texture.data[coord]);
    vec3 color = vec3(float(texColor) / 255.0f, float(texColor) / 255.0f, float(texColor) / 255.0f);
    return color;
}