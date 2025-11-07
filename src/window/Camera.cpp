/*
 * Camera.cpp
 *
 *  Created on: Feb 11, 2020
 *      Author: MihailRis
 */

#include "Camera.h"
#include "Window.h"

#include <glm/ext.hpp>

Camera::Camera(vec3 position, float fov) : position(position), fov(fov), zoom(1.0f), rotation(1.0f) {
	updateVectors();
}

void Camera::updateVectors(){
	front = vec3(rotation * vec4(0,0,1,1));
	right = vec3(rotation * vec4(1,0,0,1));
	up = vec3(rotation * vec4(0,1,0,1));
	
	// Normalize vectors to ensure they are unit vectors
	front = normalize(front);
	right = normalize(right);
	up = normalize(up);
	
	// Вычисляем горизонтальное направление (dir) без Y компонента
	dir = front;
	dir.y = 0;
	float len = length(dir);
	if (len > 0.0f){
		dir.x /= len;
		dir.z /= len;
	}
}

void Camera::rotate(float x, float y, float z){
	rotation = glm::rotate(rotation, z, vec3(0,0,1));
	rotation = glm::rotate(rotation, y, vec3(0,1,0));
	rotation = glm::rotate(rotation, x, vec3(1,0,0));

	updateVectors();
}

mat4 Camera::getProjection(){
	float aspectRatio = this->aspect;
	if (aspectRatio == 0.0f){
		aspectRatio = (float)Window::width / (float)Window::height;
	}
	if (perspective)
		return glm::perspective(fov*zoom, aspectRatio, 0.05f, 1500.0f);
	else
		if (flipped)
			return glm::ortho(0.0f, fov*aspectRatio, fov, 0.0f);
		else
			return glm::ortho(0.0f, fov*aspectRatio, 0.0f, fov);
}

mat4 Camera::getView(){
	if (perspective)
		return glm::lookAt(position, position+front, up);
	else
		return glm::translate(glm::mat4(1.0f), position);
}
