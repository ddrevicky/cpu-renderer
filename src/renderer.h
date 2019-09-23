#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "Camera.h"
#include "rasterizer.h"
#include "mesh.h"

struct Object
{
	glm::vec3 color;
	float diameter;
	float distanceFromSun;
	float orbitalPeriod;
	double currentSunRotation;
	Mesh mesh;
};

struct RenderContext
{
	Rasterizer rasterizer;
	int width;
	int height;

	Camera camera;

	// User input
	Sint32 mouseRelX;
	Sint32 mouseRelY;
	Sint32 mouseWheel;
	Uint32 shading;
	Uint32 texCoordWrap;
	bool texturingOn;
	bool backFaceCulling;
	bool solarSystem;
	bool previousSolarSystem;
	int shininess;
	int previousSphereSubdivisions;
	int sphereSubdivisions;

	// Meshes
	Mesh cubeMesh;
	Mesh sphereMesh;
	Mesh bunnyMesh;

	// Objects
	std::vector<Object> objects;

	// Camera positions
	glm::vec3 solarCameraPos;
	glm::vec3 sceneCameraPos;
};



namespace Renderer
{
	void Init(RenderContext *context, Uint32 width, Uint32 height);
	void Update(RenderContext *context, double deltaTime, bool isRunning);
	void Release(RenderContext *context);
	const char* ShadingToString(int shading);
	const char* TexWrapToString(int texCoordWrap);
}
#endif
