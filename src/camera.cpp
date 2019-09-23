#include <stdio.h>
#include <SDL2/SDL.h>
#include "Camera.h"

using glm::vec3;
using glm::vec4;
using glm::mat4;
using glm::normalize;
using glm::length;
using glm::cross;

void CameraControl::SetCameraProjectionMatrix(Camera *camera, float aspectRatio, float fovYDeg, float near, float far)
{
	camera->projectionMatrix = glm::perspective(fovYDeg, aspectRatio, 0.1f, 100.0f);
}

void CameraControl::SetCameraViewMatrix(Camera *camera, glm::vec3 position, glm::vec3 target, glm::vec3 up)
{
	camera->position = position;
	camera->target = target;
	camera->up = up;
	camera->viewMatrix = glm::lookAt(position, target, up);
}

// Camera is locked on target and rotates around it.
void CameraControl::UpdateCamera(Camera *cam, double dt, Sint32 relX, Sint32 relY, Sint32 scroll)
{
	float &yaw = cam->yaw;		// rotate around Y(up) axis
	float &pitch = cam->pitch;	// rotate around X(right) axis

	float speed = 0.03f * float(dt);
	yaw = 0.89f * yaw + -relX * speed;
	pitch = 0.89f * pitch + -relY * speed;

	vec3 camForward = normalize(cam->target - cam->position);
	vec3 camRight = normalize(cross(camForward, vec3(0.0f, 1.0f, 0.0f)));
	vec3 camUp = normalize(cross(camRight, camForward));

	// Check whether the camera is going to get parallel with the (0, 1, 0) vector
	if (glm::dot(-glm::normalize(camForward), vec3(0.0f, 1.0f, 0.0f)) > 0.98f)
	{
		pitch = glm::max(0.0f, pitch);
	}
	else if (glm::dot(-glm::normalize(camForward), vec3(0.0f, -1.0f, 0.0f)) > 0.98f)
	{
		pitch = glm::min(0.0f, pitch);
	}

	float dist = length(cam->position - cam->target);
	vec4 toCamera = normalize(vec4((cam->position - cam->target), 0.0f));

	// Rotation
	toCamera = glm::rotate(yaw, vec3(0.0f, 1.0f, 0.0f)) * toCamera;
	toCamera = glm::rotate(pitch, camRight) * toCamera;
	cam->position = cam->target + dist * vec3(toCamera);

	//Recalculate up for the View Matrix
	camForward = cam->target - cam->position;
	camRight = normalize(cross(camForward, vec3(0.0f, 1.0f, 0.0f)));
	camUp = normalize(glm::cross(camRight, camForward));

	// Zoom
	float &zoom = cam->zoomAmount;
	vec3 forward = cam->target - cam->position;
	zoom = 0.89f * zoom + (scroll * 0.01f);
	if (zoom > 0 && glm::length(forward) > 0.f || zoom < 0 && glm::length(forward) < 90.0f)
	{
		cam->position += forward * zoom;
	}
	else
	{
		zoom = 0.0f;
	}

	cam->viewMatrix = glm::lookAt(cam->position, cam->target, camUp);
}
