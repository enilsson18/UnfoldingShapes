#ifndef FACE_H
#define FACE_H

//#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/closest_point.hpp>

#include <shader.h>

#include <iostream>
#include <string>
#include <vector>
#include <cmath>

#include "Mesh.h"
#include "Axis.h"

inline float getTriangleArea(glm::vec3 a, glm::vec3 b, glm::vec3 c);
inline glm::vec3 closestPointOnLine(glm::vec3 line, glm::vec3 pointOnLine, glm::vec3 target);

// graphics method for each face
class Face {
public:
	Mesh* mesh;

	glm::vec3 rot;

	vector<Axis<Face>*> axis;


	Face(Mesh* mesh) {
		this->mesh = mesh;

		initAxis();
	}

	void initAxis() {
		// find all vertices that only have two or less indices marked of them and then connect them within the triangle.
		vector<unsigned int> validVertices;

		// identify vertices that are part of 1 or 2 triangles
		for (int i = 0; i < mesh->vertices.size(); i++) {
			int count = 0;
			
			for (int j = 0; j < mesh->indices.size(); j++) {
				if (i == mesh->indices[j]) {
					count++;
				}
			}

			if (count != 0 && count <= 2) {
				validVertices.push_back(i);
			}
		}

		// generate axis by matching indicies
		// match two of the found vertices in the same triangle
		for (int i = 0; i < validVertices.size(); i++) {
			for (int j = 0; j < validVertices.size() && j != i; j++) {
				for (int g = 0; g < mesh->indices.size(); g += 3) {
					int verticesInTriangle = 0;

					for (int h = g; h < g + 3; h++) {
						if (validVertices[i] == mesh->indices[h] || validVertices[j] == mesh->indices[h]) {
							verticesInTriangle++;
						}
					}

					if (verticesInTriangle == 2) {
						axis.push_back(new Axis<Face>(mesh->vertices[validVertices[i]].Position, mesh->vertices[validVertices[j]].Position));
					}
				}
			}
		}

		// delete both duplicates if found because this means that triangles were sharing an inside edge (we only want outside edges)
		vector<unsigned int> toRemove;

		for (int i = 0; i < axis.size(); i++) {
			//bool collision = false;

			for (int j = 0; j < axis.size(); j++) {
				if (*axis[i] == *axis[j] && i != j) {
					toRemove.push_back(i);
					//toRemove.push_back(j);
					//collision = true;
				}
			}

			/*
			if (collision) {
				toRemove.push_back(i);
			}
			*/
		}

		// apply removal
		// first sort all of them
		for (unsigned int i = 0; i < toRemove.size(); i++) {
			bool swapped = false;
			for (unsigned int j = 0; j < toRemove.size() - 1 - i; j++) {
				if (toRemove[j] > toRemove[j+1]) {
					int temp = toRemove[j];
					toRemove[j] = toRemove[j + 1];
					toRemove[j + 1] = temp;

					swapped = true;
				}
			}

			//exit search
			if (swapped == false) {
				break;
			}
		}

		// finally remove from list
		for (int i = toRemove.size() - 1; i >= 0; i--) {
			axis.erase(axis.begin() + toRemove[i]);
		}
	}

	float getArea() {
		float area = 0;

		for (int i = 0; i < mesh->indices.size(); i += 3) {
			area += getTriangleArea(mesh->vertices[mesh->indices[i]].Position, mesh->vertices[mesh->indices[i+1]].Position, mesh->vertices[mesh->indices[i+2]].Position);
		}
		
		return area;
	}

	void printAxis() {

		std::cout << "Face has " << axis.size() << " axis. A center of mass at: " << glm::to_string(findCenterOfMass()) << std::endl;
		std::cout << "Including: " << std::endl;
		for (int i = 0; i < axis.size(); i++) {
			axis[i]->print();
		}
		std::cout << std::endl;
	}

	glm::vec3 findCenterOfMass() {
		glm::vec3 center = glm::vec3(0);

		for (int i = 0; i < mesh->vertices.size(); i++) {
			center += mesh->vertices[i].Position;
		}

		center /= mesh->vertices.size();

		return center;
	}
};

// returns the closes vec3 point to a 3d line
glm::vec3 closestPointOnLine(glm::vec3 line, glm::vec3 pointOnLine, glm::vec3 target) {
	glm::vec3 newPoint = target - pointOnLine;
	float dot = glm::dot(newPoint, line);
	return pointOnLine + line * dot;
}

float getTriangleArea(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
	glm::vec3 ortho = glm::cross(a - b, a - c);

	return sqrt(ortho.x*ortho.x + ortho.y*ortho.y + ortho.z*ortho.z) / 2;
}

#endif
