#include <algorithm>
#include <stdlib.h>
#include "renderer.h"
#include "rasterizer.h"
#include "Camera.h"
#include "mesh.h"
#include "math.h"

#define Z_NEAR 0.1f
#define Z_FAR 500.0f

using std::min;
using std::max;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;
using glm::scale;
using glm::rotate;
using glm::translate;

const char* Renderer::ShadingToString(int shading)
{
	switch (shading)
	{
	case FLAT_SHADING:
		return "Flat";
		break;
	case GOURAUD_SHADING:
		return "Gouraud";
		break;
	case PHONG_SHADING:
		return "Phong";
		break;
	default:
		return NULL;
		break;
	}
}

const char* Renderer::TexWrapToString(int texCoordWrap)
{
	switch (texCoordWrap)
	{
	case TEX_COORD_CLAMP:
		return "Clamp";
		break;
	case TEX_COORD_REPEAT:
		return "Repeat";
		break;
	default:
		return NULL;
		break;
	}
}

static Object CreateObject(RenderContext *context, vec3 color, float diameter, float distFromSun, float orbitalPeriod)
{
	Object object = {};
	object.color = color;
	object.diameter = diameter;
	object.distanceFromSun = distFromSun;
	object.orbitalPeriod = orbitalPeriod;
	object.currentSunRotation = float(std::rand()) / RAND_MAX * 2 * 3.1415f;

	object.mesh = UtilMesh::MakeUVSphere(context->sphereSubdivisions, color);
	return object;
}

void Renderer::Init(RenderContext* context, Uint32 width, Uint32 height)
{
	context->width = width;
	context->height = height;
	context->shading = FLAT_SHADING;
	context->solarSystem = false;
	context->previousSolarSystem = false;
	context->texCoordWrap = TEX_COORD_REPEAT;
	context->texturingOn = true;
	context->shininess = 16;
	context->sphereSubdivisions = 20;
	context->previousSphereSubdivisions = context->sphereSubdivisions;
	context->backFaceCulling = true;
	context->sceneCameraPos = vec3(-4.8f, 2.56f, 6.51f);
	context->solarCameraPos = vec3(-22.0f, 15.0f, 33.0f);

	Rasterization::Init(&context->rasterizer, width, height, Z_NEAR);

	CameraControl::SetCameraProjectionMatrix(&context->camera, float(width) / float(height), 45.0f, Z_NEAR, Z_FAR);
	CameraControl::SetCameraViewMatrix(&context->camera, context->sceneCameraPos, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));

	// Create meshes
	context->cubeMesh = UtilMesh::MakeCubeCentered(2.0f);
	context->sphereMesh = UtilMesh::MakeUVSphere(context->sphereSubdivisions, vec3(0.0f, 0.0f, 1.0f));
	context->bunnyMesh = UtilMesh::MakeBunnyMesh();

	U::worldLightDirection = glm::normalize(vec3(0.0f, 0.0f, -1.0f));
	U::directionalLightOn = true;
	U::worldLightPosition = vec3(0.0f, 0.0f, 0.0f);
	U::sunMesh = false;

	// Set checkerboard texture
	Texture texture = {};
	texture.width = 32;
	texture.height = 32;
	texture.data = (Uint8*)malloc(texture.width * texture.height * sizeof(Uint8));
	for (Uint32 j = 0; j < texture.height; ++j)
	{
		Uint8 *P = &texture.data[j * texture.width];
		for (Uint32 i = 0; i < texture.width; ++i)
		{
			int c = (((i & 0x08) == 0) ^ ((j & 0x08) == 0)) * 0xff;
			*P = (Uint8)c;
			*P++;
		}
	}

	Rasterization::SetTexture(&context->rasterizer, &texture);
	free(texture.data);

	// The Solar System
	context->objects.push_back(CreateObject(context, vec3(252 / 255.f, 224 / 255.0f, 32 / 255.0f), 4.2f, 0.0f, 0.0f));		// sun
	context->objects.push_back(CreateObject(context, vec3(250 / 255.f, 251 / 255.0f, 186 / 255.0f), 0.8f, 4.0f, 0.241f));	// mercury 
	context->objects.push_back(CreateObject(context, vec3(234 / 255.f, 201 / 255.0f, 134 / 255.0f), 1.2f, 6.0f, 0.615f));	// venus 
	context->objects.push_back(CreateObject(context, vec3(51 / 255.f, 62 / 255.0f, 91 / 255.0f), 1.3f, 8.0f, 1.0f));		// earth
	context->objects.push_back(CreateObject(context, vec3(116 / 255.f, 18 / 255.0f, 3 / 255.0f), 0.7f, 10.0f, 1.88f));		// mars
	context->objects.push_back(CreateObject(context, vec3(125 / 255.f, 58 / 255.0f, 26 / 255.0f), 2.3f, 13.0f, 11.9f));		// jupiter
	context->objects.push_back(CreateObject(context, vec3(251 / 255.f, 238 / 255.0f, 186 / 255.0f), 2.1f, 17.0f, 29.4f));	// saturn
	context->objects.push_back(CreateObject(context, vec3(110 / 255.f, 207 / 255.0f, 250 / 255.0f), 1.8f, 20.0f, 83.7f));	// uranus
	context->objects.push_back(CreateObject(context, vec3(99 / 255.f, 138 / 255.0f, 241 / 255.0f), 1.6f, 23.0f, 163.7f));	// neptune
}

static void DrawTriangleMesh(RenderContext *context, Mesh *mesh, mat4 modelMatrix)
{
	U::modelMatrix = modelMatrix;
	U::viewMatrix = context->camera.viewMatrix;
	U::mvpMatrix = context->camera.projectionMatrix * context->camera.viewMatrix * modelMatrix;

	Rasterization::DrawTriangleMesh(&context->rasterizer, mesh);
}

// Debug functionality
static void DrawNormalMesh(RenderContext *context, Mesh *mesh, mat4 modelMatrix)
{
	U::modelMatrix = modelMatrix;
	U::mvpMatrix = context->camera.projectionMatrix * context->camera.viewMatrix * modelMatrix;

	Mesh normalMesh = UtilMesh::MakeNormalMesh(mesh, 1.0f);
	Rasterization::DrawLineMesh(&context->rasterizer, &normalMesh);
	UtilMesh::Release(normalMesh);
}

// Debug functionality
static void DrawLineMesh(RenderContext *context, Mesh *mesh, mat4 modelMatrix)
{
	U::modelMatrix = modelMatrix;
	U::mvpMatrix = context->camera.projectionMatrix * context->camera.viewMatrix * modelMatrix;

	Mesh copyMesh = UtilMesh::MakeMeshCopy(mesh);
	Rasterization::DrawLineMesh(&context->rasterizer, &copyMesh);
	UtilMesh::Release(copyMesh);
}

static void UpdateContext(RenderContext *context, double dt)
{
	Camera *camera = &context->camera;
	CameraControl::UpdateCamera(camera, dt, context->mouseRelX, context->mouseRelY, context->mouseWheel);

	if (context->solarSystem && !context->previousSolarSystem)
	{
		U::directionalLightOn = false;
		context->previousSolarSystem = true;
		CameraControl::SetCameraViewMatrix(&context->camera, context->solarCameraPos, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	}
	else if (!context->solarSystem && context->previousSolarSystem)
	{
		U::sunMesh = false;
		U::directionalLightOn = true;
		context->previousSolarSystem = false;
		CameraControl::SetCameraViewMatrix(&context->camera, context->sceneCameraPos, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	}

	if (context->solarSystem)
	{
		context->rasterizer.clearColor = vec3(0, 0, 0);
	}
	else
	{
		context->rasterizer.clearColor = vec3(1.0f, 1.0f, 1.0f)*0.05f;
	}

	if (context->sphereSubdivisions != context->previousSphereSubdivisions)
	{
		UtilMesh::Release(context->sphereMesh);
		context->sphereMesh = UtilMesh::MakeUVSphere(context->sphereSubdivisions, vec3(0, 0, 1));
		context->previousSphereSubdivisions = context->sphereSubdivisions;
	}

	if (context->width != context->rasterizer.width || context->height != context->rasterizer.height)
	{
		CameraControl::SetCameraProjectionMatrix(&context->camera, float(context->width) / float(context->height), 45.0f, Z_NEAR, Z_FAR);
		Rasterization::Resize(&context->rasterizer, context->width, context->height);
	}

	if (context->backFaceCulling != context->rasterizer.backFaceCulling)
	{
		context->rasterizer.backFaceCulling = context->backFaceCulling;
	}

	U::worldCameraPosition = context->camera.position;
	U::shading = context->shading;
	U::texturingOn = context->texturingOn;
	U::shininess = context->shininess;
	U::texCoordWrap = context->texCoordWrap;
}

static void RenderObject(RenderContext *context, Object object, double dt)
{
	if (object.orbitalPeriod != 0.0f)
		object.currentSunRotation += 1.5f * dt / object.orbitalPeriod;

	float s = (object.diameter / 2.0f);
	mat4 scaleMat = scale(mat4(1.0f), vec3(s, s, s));
	mat4 translMat = translate(mat4(1.0f), vec3(1, 0, 0) * object.distanceFromSun);
	mat4 rotateMat = rotate(mat4(1.0f), float(object.currentSunRotation), vec3(0, 1, 0));
	mat4 model = rotateMat * translMat * scaleMat;

	DrawTriangleMesh(context, &context->sphereMesh, model);
}

static void RenderObjects(RenderContext *context, double dt)
{
	static float time = 0.0f;
	time += float(dt);

	if (context->solarSystem)
	{
		for (Uint32 i = 0; i < context->objects.size(); ++i)
		{
			Object &object = context->objects[i];

			if (i == 0)	// sun
				U::sunMesh = true;

			if (object.orbitalPeriod != 0.0f)
				object.currentSunRotation += 1.5f * dt / object.orbitalPeriod;

			float s = (object.diameter / 2.0f);
			mat4 scaleMat = scale(mat4(1.0f), vec3(s, s, s));
			mat4 translMat = translate(mat4(1.0f), vec3(1, 0, 0) * object.distanceFromSun);
			mat4 rotateMat = rotate(mat4(1.0f), float(object.currentSunRotation), vec3(0, 1, 0));
			mat4 model = rotateMat * translMat * scaleMat;

			DrawTriangleMesh(context, &object.mesh, model);

			if (i == 0)
				U::sunMesh = false;
		}
	}
	else
	{
		mat4 model = rotate(translate(mat4(1.0f), vec3(0.0f, 0.0f, -4.0f)), 0.0f, vec3(0.0f, 1.0f, 0.0f));
		DrawTriangleMesh(context, &context->cubeMesh, model);
		
		model = rotate(scale(translate(mat4(1.0f), vec3(5, 0, 0)), vec3(2.f, 2.f, 2.f)), 1.8f*float(time), vec3(0, 1, 0));
		DrawTriangleMesh(context, &context->sphereMesh, model);

		model = rotate(scale(translate(mat4(1.0f), vec3(0.0f, 0.0f, 0.0f)), vec3(1.4f, 1.4f, 1.4f)), 0.2f*float(time), glm::normalize(vec3(cosf(time), cosf(time), sinf(time))));
		DrawTriangleMesh(context, &context->bunnyMesh, model);
	}
}

void Renderer::Update(RenderContext *context, double dt, bool isRunning)
{
	UpdateContext(context, dt);

	Rasterization::Clear(&context->rasterizer, COLOR_BIT | DEPTH_BIT);
	RenderObjects(context, dt);
}

void Renderer::Release(RenderContext *context)
{
	UtilMesh::Release(context->cubeMesh);
	UtilMesh::Release(context->sphereMesh);
	UtilMesh::Release(context->bunnyMesh);

	for (Uint32 i = 0; i < context->objects.size(); ++i)
	{
		UtilMesh::Release(context->objects[i].mesh);
	}
	Rasterization::Release(&context->rasterizer);
}
