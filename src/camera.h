#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL2/SDL.h>

struct UserInput;

struct Camera
{
	glm::vec3 up;
	glm::vec3 position;
	glm::vec3 target;

	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;

	float yaw;
	float pitch;
	float zoomAmount;
};

namespace CameraControl
{
	void SetCameraViewMatrix(Camera *camera, glm::vec3 position, glm::vec3 target, glm::vec3 up);
	void SetCameraProjectionMatrix(Camera *camera, float aspectRatio, float fovYDeg, float near, float far);
	void UpdateCamera(Camera *camera, double dt, Sint32 relX, Sint32 relY, Sint32 scroll);
}

#endif