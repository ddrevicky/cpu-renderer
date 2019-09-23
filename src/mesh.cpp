#include "mesh.h"
#include "common.h"
#include "bunny.h"

using glm::normalize;
using glm::vec2;
using glm::vec3;
using glm::vec4;

#define PI 3.14159265358979323846264338327950f

Mesh UtilMesh::MakeBunnyMesh()
{
	Mesh mesh = {};
	mesh.isTexturable = false;
	mesh.vertexCount = 0;
	mesh.vertices = (Vertex *)malloc(2092 * 3 * sizeof(Vertex));

	for (Uint32 i = 0; i < 2092; ++i)
	{
		for (Uint32 j = 0; j < 3; ++j)
		{
			short index = bunnyIndices[i][j];
			BunnyVertex bv = bunnyVertices[index];

			Vertex v = {};
			v.position = vec4(bv.position[0], bv.position[1], bv.position[2], 1.0f);
			v.normal = vec3(bv.normal[0], bv.normal[1], bv.normal[2]);
			v.vsOutColor = vec3(1.0f, 0.0f, 0.0f);

			mesh.vertices[i * 3 + j] = v;
			mesh.vertexCount++;
		}
	}

	return mesh;
}

// Contains lines created as normals of the original mesh (starting at the vertices of the original and going
// to the normal direction).
Mesh UtilMesh::MakeNormalMesh(Mesh *original, float normalLength)
{
	Mesh normalMesh = {};
	normalMesh.vertexCount = 0;
	normalMesh.vertices = (Vertex*)malloc(2 * original->vertexCount * sizeof(Vertex));
	normalMesh.isTexturable = false;

	for (Uint32 i = 0; i < original->vertexCount; ++i)
	{
		Vertex start = {};
		start.position = original->vertices[i].position;
		start.vsOutColor = vec3(1.0f, 1.0f, 0.0f);

		Vertex end = {};
		end.position = start.position + vec4(original->vertices[i].normal, 0.0f) * normalLength;
		end.vsOutColor = vec3(1.0f, 1.0f, 0.0f);

		normalMesh.vertices[normalMesh.vertexCount++] = start;
		normalMesh.vertices[normalMesh.vertexCount++] = end;
	}

	return normalMesh;
}

Mesh UtilMesh::MakeWorldAxesMesh()
{
	Mesh mesh = {};
	mesh.vertexCount = 3 * 2;
	mesh.vertices = (Vertex*)malloc(mesh.vertexCount * sizeof(Vertex));
	mesh.isTexturable = false;

	float axisLength = 3;

	Vertex center = {}, x = {}, y = {}, z = {};

	center.position = vec4(0, 0, 0, 1);
	x.position = vec4(axisLength, 0, 0, 1);
	x.vsOutColor = vec3(1, 0, 0);
	y.position = vec4(0, axisLength, 0, 1);
	y.vsOutColor = vec3(0, 1, 0);
	z.position = vec4(0, 0, axisLength, 1);
	x.vsOutColor = vec3(0, 1, 0);

	mesh.vertices[0] = center;
	mesh.vertices[1] = x;
	mesh.vertices[2] = center;
	mesh.vertices[3] = y;
	mesh.vertices[4] = center;
	mesh.vertices[5] = z;

	return mesh;
}

void AddLine(Mesh *mesh, Vertex v0, Vertex v1)
{
	mesh->vertices[mesh->vertexCount++] = v0;
	mesh->vertices[mesh->vertexCount++] = v1;
}

// Plane is visualized as a set of lines
Mesh UtilMesh::MakePlaneMesh()
{
	Uint32 numberOfLines = 60;
	float span = 5.0f;

	Mesh mesh = {};
	mesh.vertexCount = 0;
	mesh.vertices = (Vertex*)malloc(numberOfLines * 2 * sizeof(Vertex));
	mesh.isTexturable = false;

	// The plane is built in the xy plane (z == 0)
	for (Uint32 i = 0; i < numberOfLines; ++i)
	{
		float y = -(span / 2.0f) + i * (span / numberOfLines);
		float startX = -span / 2.0f;
		float endX = span / 2.0f;
		float z = 0.0f;

		Vertex start = {}, end = {};
		start.position = vec4(span / 2.0f, y, 0.0f, 1.0f);
		end.position = vec4(-span / 2.0f, y, 0.0f, 1.0f);
		start.vsOutColor = vec3(0, 0, 1);
		end.vsOutColor = vec3(0, 0, 1);

		AddLine(&mesh, start, end);
	}

	return mesh;
}

void UtilMesh::AddTriangle(Mesh *mesh, Vertex v0, Vertex v1, Vertex v2)
{
	mesh->vertices[mesh->vertexCount++] = v0;
	mesh->vertices[mesh->vertexCount++] = v1;
	mesh->vertices[mesh->vertexCount++] = v2;
}

// Phi is latitude. Theta longitude.
vec4 SphericalToCartesian(float r, float phi, float theta)
{
	vec4 cartesian;
	cartesian.x = r * sin(theta) * sin(phi);
	cartesian.y = r * cos(phi);
	cartesian.z = r * cos(theta) * sin(phi);
	cartesian.w = 1.0f;
	return cartesian;
}

Mesh UtilMesh::MakeUVSphere(Uint32 subdivisions, vec3 color)
{
	Uint32 stacks = subdivisions;
	Uint32 slices = subdivisions;
	float r = 1.0f;
	vec3 c = vec3(0.0f, 0.0f, 0.0f);

	Mesh mesh = {};
	mesh.vertexCount = 0;
	mesh.vertices = (Vertex*)malloc(3 * (slices * 2 + ((stacks - 2) * slices * 2)) * sizeof(Vertex));
	mesh.isTexturable = false;

	for (Uint32 p = 0; p < stacks; ++p)
	{
		float phi1 = (float(p) / stacks)*PI;
		float phi2 = (float(p + 1) / stacks)*PI;

		for (Uint32 t = 0; t < slices; ++t)
		{
			float theta1 = (float(t) / slices) * 2 * PI;
			float theta2 = (float(t + 1) / slices) * 2 * PI;

			Vertex v1 = {}, v2 = {}, v3 = {}, v4 = {};

			v1.position = SphericalToCartesian(r, phi1, theta1);
			v2.position = SphericalToCartesian(r, phi2, theta1);
			v3.position = SphericalToCartesian(r, phi2, theta2);
			v4.position = SphericalToCartesian(r, phi1, theta2);

			v1.normal = normalize(vec3(v1.position) - c);
			v2.normal = normalize(vec3(v2.position) - c);
			v3.normal = normalize(vec3(v3.position) - c);
			v4.normal = normalize(vec3(v4.position) - c);

			v1.vsOutColor = color;
			v2.vsOutColor = color;
			v3.vsOutColor = color;
			v4.vsOutColor = color;

			if (p == 0)							// First stack
				AddTriangle(&mesh, v1, v2, v3);
			else if (p + 1 == stacks)
				AddTriangle(&mesh, v2, v4, v1);	// Last stack
			else 
			{
				AddTriangle(&mesh, v1, v2, v3);	// Middle stacks
				AddTriangle(&mesh, v3, v4, v1);
			}
		}
	}

	return mesh;
}

Mesh UtilMesh::MakeTriangle()
{
	Mesh mesh = {};
	mesh.vertexCount = 3;
	mesh.vertices = (Vertex*)malloc(mesh.vertexCount * sizeof(Vertex));
	mesh.isTexturable = false;

	Vertex v0 = {}, v1 = {}, v2 = {};
	v0.position = vec4(0, 0, -1, 1);
	v1.position = vec4(-1, 0, 1, 1);
	v2.position = vec4(0, 0, 1, 1);

	mesh.vertices[0] = v1;
	mesh.vertices[1] = v2;
	mesh.vertices[2] = v0;

	return mesh;
}

Mesh UtilMesh::MakeCubeCentered(float edgeSize)
{
	float size = edgeSize;

	Vertex vertices[] =
	{
		// Front
        Vertex{ vec4(-size / 2, -size / 2, size / 2, 1.0f), vec2(1.5f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.5f, 0.0f, 0.0f) }, // LBN,
        Vertex{ vec4(size / 2, -size / 2, size / 2, 1.0f), vec2(0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.5f, 0.0f) }, // RBN,
        Vertex{ vec4(size / 2, size / 2, size / 2, 1.0f), vec2(0.0f, 1.5f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, 0.5f) }, // RTN,
        Vertex{ vec4(size / 2, size / 2, size / 2, 1.0f), vec2(0.0f, 1.5f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, 0.5f) }, // RTN,
        Vertex{ vec4(-size / 2, size / 2, size / 2, 1.0f), vec2(1.5f, 1.5f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.5f, 0.0f) }, // LTN,
        Vertex{ vec4(-size / 2, -size / 2, size / 2, 1.0f), vec2(1.5f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.5f, 0.0f, 0.0f) }, // LBN,

		// Top
        Vertex{ vec4(-size / 2, size / 2, size / 2, 1.0f), vec2(0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.5f, 0.0f, 0.0f) }, // LTN,
        Vertex{ vec4(size / 2, size / 2, size / 2, 1.0f), vec2(1.5f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.5f, 0.0f) }, // RTN,
        Vertex{ vec4(size / 2, size / 2, -size / 2, 1.0f), vec2(1.5f, 1.5f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 0.5f) }, // RTF,
        Vertex{ vec4(size / 2, size / 2, -size / 2, 1.0f), vec2(1.5f, 1.5f), vec3(0.0f, 1.0f, 0.0f), vec3(0.5f, 0.0f, 0.0f) }, // RTF,
        Vertex{ vec4(-size / 2, size / 2, -size / 2, 1.0f), vec2(0.0f, 1.5f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.5f, 0.0f) }, // LTF,
        Vertex{ vec4(-size / 2, size / 2, size / 2, 1.0f), vec2(0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 0.5f) }, // LTN,

		// Back
		Vertex{ vec4(size / 2, -size / 2, -size / 2, 1.0f), vec2(1.5f, 0.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 0.0f, 0.5f) }, // RBF,
		Vertex{ vec4(-size / 2, -size / 2, -size / 2, 1.0f), vec2(0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 0.5f, 0.0f) }, // LBF,
		Vertex{ vec4(-size / 2, size / 2, -size / 2, 1.0f), vec2(0.0f, 1.5f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 0.0f, 0.5f) }, // LTF,
		Vertex{ vec4(-size / 2, size / 2, -size / 2, 1.0f), vec2(0.0f, 1.5f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 0.0f, 0.5f) }, // LTF,
		Vertex{ vec4(size / 2, size / 2, -size / 2, 1.0f), vec2(1.5f, 1.5f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 0.5f, 0.0f) }, // RTF,
		Vertex{ vec4(size / 2, -size / 2, -size / 2, 1.0f), vec2(1.5f, 0.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 0.0f, 0.5f) }, // RBF,

		// Bottom
        Vertex{ vec4(-size / 2, -size / 2, -size / 2, 1.0f), vec2(0.0f, 1.5f), vec3(0.0f, -1.0f, 0.0f), vec3(0.5f, 0.0f, 0.0f) }, // LBF, 
        Vertex{ vec4(size / 2, -size / 2, -size / 2, 1.0f), vec2(1.5f, 1.5f), vec3(0.0f, -1.0f, 0.0f), vec3(0.0f, 0.5f, 0.0f) }, // RBF,
        Vertex{ vec4(size / 2, -size / 2, size / 2, 1.0f), vec2(1.5f, 0.0f), vec3(0.0f, -1.0f, 0.0f), vec3(0.0f, 0.0f, 0.5f) }, // RBN,
        Vertex{ vec4(size / 2, -size / 2, size / 2, 1.0f), vec2(1.5f, 0.0f), vec3(0.0f, -1.0f, 0.0f), vec3(0.5f, 0.0f, 0.0f) }, // RBN,
        Vertex{ vec4(-size / 2, -size / 2, size / 2, 1.0f), vec2(0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f), vec3(0.0f, 0.5f, 0.0f) }, // LBN,
        Vertex{ vec4(-size / 2, -size / 2, -size / 2, 1.0f), vec2(0.0f, 1.5f), vec3(0.0f, -1.0f, 0.0f), vec3(0.0f, 0.0f, 0.5f) }, // LBF,

		// Right
        Vertex{ vec4(size / 2, -size / 2, size / 2, 1.0f), vec2(0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.5f, 0.0f, 0.0f) }, // RBN,
        Vertex{ vec4(size / 2, -size / 2, -size / 2, 1.0f), vec2(1.5f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.5f, 0.0f) }, // RBF,
        Vertex{ vec4(size / 2, size / 2, -size / 2, 1.0f), vec2(1.5f, 1.5f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.5f) }, // RTF,
        Vertex{ vec4(size / 2, size / 2, -size / 2, 1.0f), vec2(1.5f, 1.5f), vec3(1.0f, 0.0f, 0.0f), vec3(0.5f, 0.0f, 0.0f) }, // RTF,
        Vertex{ vec4(size / 2, size / 2, size / 2, 1.0f), vec2(0.0f, 1.5f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.5f, 0.0f) }, // RTN,
        Vertex{ vec4(size / 2, -size / 2, size / 2, 1.0f), vec2(0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.5f) }, // RBN,

		// Left
		Vertex{ vec4(-size / 2, -size / 2, -size / 2, 1.0f), vec2(0.0f, 0.0f), vec3(-1.0f, 0.0f, 0.0f), vec3(0.5f, 0.0f, 0.0f) }, // LBF,
		Vertex{ vec4(-size / 2, -size / 2, size / 2, 1.0f), vec2(1.5f, 0.0f), vec3(-1.0f, 0.0f, 0.0f) , vec3(0.0f, 0.5f, 0.0f) }, // LBN,
		Vertex{ vec4(-size / 2, size / 2, size / 2, 1.0f), vec2(1.5f, 1.5f), vec3(-1.0f, 0.0f, 0.0f)  , vec3(0.0f, 0.0f, 0.5f) }, // LTN,
		Vertex{ vec4(-size / 2, size / 2, size / 2, 1.0f), vec2(1.5f, 1.5f), vec3(-1.0f, 0.0f, 0.0f)  , vec3(0.5f, 0.0f, 0.0f) }, // LTN,
		Vertex{ vec4(-size / 2, size / 2, -size / 2, 1.0f), vec2(0.0f, 1.5f), vec3(-1.0f, 0.0f, 0.0f) , vec3(0.0f, 0.5f, 0.0f) }, // LTF,
        Vertex{ vec4(-size / 2, -size / 2, -size / 2, 1.0f), vec2(0.0f, 0.0f), vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.5f) }, // LBF,
	};

	Mesh mesh = {};
	mesh.vertexCount = SDL_arraysize(vertices);
	mesh.vertices = (Vertex*) malloc(mesh.vertexCount * sizeof(Vertex));
	mesh.isTexturable = true;
	memcpy(mesh.vertices, vertices, sizeof(vertices));

	return mesh;
}

void UtilMesh::Release(Mesh mesh)
{
	 free(mesh.vertices); 
}

Mesh UtilMesh::MakeMeshCopy(Mesh *original)
{
	Mesh mesh = {};
	mesh.isTexturable = original->isTexturable;
	mesh.vertexCount = original->vertexCount;
	mesh.vertices = (Vertex*)malloc(original->vertexCount * sizeof(Vertex));
	memcpy(mesh.vertices, original->vertices, original->vertexCount * sizeof(Vertex));
	return mesh;
}

void UtilMesh::UpdateVertices(Mesh *mesh, Vertex *newVertices, Uint32 newVertexCount)
{
	free(mesh->vertices);
	mesh->vertexCount = newVertexCount;
	mesh->vertices = (Vertex*)malloc(newVertexCount * sizeof(Vertex));
	memcpy(mesh->vertices, newVertices, newVertexCount * sizeof(Vertex));
}