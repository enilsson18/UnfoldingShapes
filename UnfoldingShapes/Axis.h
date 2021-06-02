#ifndef AXIS_H
#define AXIS_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/closest_point.hpp>

#include <shader.h>

#include <iostream>
#include <string>
#include <vector>
#include <cmath>

#include "Mesh.h"

// Codependency fix with Face class was to make this generic
template<class T>
class Axis {
private:
	// smallest possible float
	const float marginOfError = 0.0001;
	const float sizeCap = 1000.0f;

public:
	// stores the axis with a point for refrence and the vector of the line
	glm::vec3 originalPoint;
	glm::vec3 originalLine;

	glm::vec3 point;
	glm::vec3 line;

	float originalAngle;

	Axis* sharedAxis;
	T* neighborFace;

	//testing
	glm::vec3 p1, p2;

	// default
	Axis() {
		defaultSetup();
	}

	Axis(glm::vec3 p1, glm::vec3 p2) {
		this->p1 = p1;
		this->p2 = p2;

		// normalize to standardize it
		originalLine = glm::normalize(p1 - p2);

		originalPoint = closestPointOnLine(originalLine, p1, glm::vec3(0));

		line = originalLine;
		point = originalPoint;
	}

	void defaultSetup() {
		sharedAxis = nullptr;
		neighborFace = nullptr;

		originalPoint = glm::vec3(0);
		originalLine = glm::vec3(0);
		point = originalPoint;
		line = originalLine;
		originalAngle = 0.0f;
	}

	void revert() {
		line = originalLine;
		point = originalPoint;
	}

	void setNeighbor(T* neighbor, Axis* axis) {
		sharedAxis = axis;
		neighborFace = neighbor;
	}

	glm::vec3 rotateAbout(glm::vec3 p, float angle) {
		glm::mat4 rotationMat(1.0f);

		rotationMat = glm::rotate(rotationMat, angle, line);

		// translate the point to its relative position on the axis and then back to is actual position.
		return glm::vec3(rotationMat * glm::vec4(p - point, 1.0f)) + point;
	}

	// transform this axis based on another axis
	void rotateAxisAbout(Axis* axis, float angle) {
		//std::cout << "1 " << glm::to_string(line) << " " << glm::to_string(point) << std::endl;
		glm::vec3 point1 = line * -0.1f + point;
		glm::vec3 point2 = line * 0.1f + point;

		point1 = axis->rotateAbout(point1, angle);
		point2 = axis->rotateAbout(point2, angle);

		// normalize to standardize it
		line = glm::normalize(point2 - point1);

		//point = closestPointOnLine(line, point1, glm::vec3(0));
		point = axis->rotateAbout(point, angle);

		//std::cout << "2 " << glm::to_string(line) << " " << glm::to_string(point) << std::endl;
	}

	float orientedAngle(glm::vec3 p1, glm::vec3 p2) {
		p1 = glm::normalize(p1 - point);
		p2 = glm::normalize(p2 - point);

		float result = glm::orientedAngle(p1, p2, line);
		result = (result / (sqrt(result*result)))*3.1415926535 - result;

		// std::cout << "result: " << result << std::endl;

		return result;
	}

	// check if the axis contains a point
	bool hasPoint(glm::vec3 p) {
		//std::cout << glm::to_string(line) << " " << glm::to_string(point) << std::endl;

		glm::vec3 pointOnLine = closestPointOnLine(line, point, p);

		//std::cout << glm::to_string(pointOnLine) << " " << glm::to_string(p) << " " << glm::to_string(line) << std::endl;

		if (glm::distance(pointOnLine, p) <= marginOfError) {
			return true;
		}

		return false;
	}

	bool operator==(Axis a) {
		if (hasPoint(a.point) && glm::distance(point, a.point) < marginOfError)
		{
			return true;
		}

		return false;
	}

	Axis& operator=(const Axis& a) {
		point = a.point;
		line = a.line;

		originalAngle = a.originalAngle;

		sharedAxis = a.sharedAxis;
		neighborFace = a.neighborFace;

		return *this;
	}

	void print() {
		std::cout << "Line: " << glm::to_string(line) << ", Point: " << glm::to_string(point) << ", Original Angle: " << originalAngle << ", Original Points: " << glm::to_string(p1) << " " << glm::to_string(p2) << std::endl;
	}
};

#endif