#ifndef MESH_H
#define MESH_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL2/SDL.h>

struct Vertex
{
    glm::vec4 position;
    glm::vec2 textureCoords;
    glm::vec3 normal;

    // Output "Vertex Shader" attributes. Values not defined in the mesh! Only stored
    // here for convenience.
    glm::vec3 vsOutColor;
    glm::vec3 vsOutWorldNormal;
    glm::vec3 vsOutWorldPos;
};

struct Mesh
{
    Vertex *vertices;
    Uint32 vertexCount;
    bool isTexturable;
};

namespace UtilMesh
{
    Mesh MakeNormalMesh(Mesh *mesh, float normalLength);
    Mesh MakeWorldAxesMesh();
    Mesh MakePlaneMesh();
    Mesh MakeCubeCentered(float edgeSize);
    Mesh MakeMeshCopy(Mesh *original);
    Mesh MakeUVSphere(Uint32 subdivisions, glm::vec3 color);
    Mesh MakeTriangle();
    Mesh MakeBunnyMesh();
    void UpdateVertices(Mesh *mesh, Vertex *newVertices, Uint32 newVertexCount);
    void AddTriangle(Mesh *mesh, Vertex v0, Vertex v1, Vertex v2);
    void Release(Mesh mesh);
}

#endif
