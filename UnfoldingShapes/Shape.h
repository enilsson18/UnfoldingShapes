#ifndef SHAPE_H
#define SHAPE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <iostream>
#include <vector>

// graphics tools
#include "Camera.h"
#include "Light.h"
#include "Asset.h"
#include "Model.h"
#include "Mesh.h"
#include "Skybox.h"
#include "TextManager.h"
#include "GraphicsEngine.h"

#include "Face.h"
#include "Graph.h"

class Shape {
public:
	Asset* asset;
	Model* model;
	
	vector<Face*> faces;

	Graph<Face> faceMap;

	// inactive
	Shape() {
		asset = nullptr;
	}

	// init Shape by setting the asset and registering all of the faces.
	// Copies the model because we are manipulating the face and vertex info.
	Shape(string const &path, glm::vec3 pos = glm::vec3(0), glm::vec3 rot = glm::vec3(0), glm::vec3 scale = glm::vec3(1)) {
		this->model = new Model(path, 1);
		asset = new Asset(this->model, pos, rot, scale);

		initFaces();
	}
private:
	// recursivley populate the faceMap
	void populateFaceMap(Graph<Face>::Node* node, vector<Face*> &faces) {
		vector<Face*> tempFaces = vector<Face*>();

		std::cout << "faces size: " << faces.size() << std::endl;
		for (int i = 0; i < faces.size(); i++) {
			for (int g = 0; g < faces[i]->axis.size(); g++) {
				for (int h = 0; h < node->data->axis.size(); h++) {
					// if the axis match and they are not the same face
					//std::cout << "axis 1 " << glm::to_string(faces[i]->axis[g]->point1) << ", " << glm::to_string(faces[i]->axis[g]->point2) << endl;
					//std::cout << "axis 2 " << glm::to_string(node->data->axis[h]->point1) << ", " << glm::to_string(node->data->axis[h]->point2) << endl;
					if (*faces[i]->axis[g] == *node->data->axis[h] && node->data != faces[i]) {
						tempFaces.push_back(faces[i]);
					}
				}
			}
		}


		// add all children and proccess them too
		std::cout << "tempFaces size: " << tempFaces.size() << std::endl;
		for (int i = 0; i < tempFaces.size(); i++) {
			// this prevents the recursive method from getting stuck in a loop (only run populateFaceMap once for each face)
			bool exists = false;
			if (node->graph->findNode(node->graph->rootNode, tempFaces[i]) != nullptr) {
				exists = true;
			}

			Graph<Face>::Node* newNode = node->graph->newNode(node, tempFaces[i]);

			if (exists == false) {
				populateFaceMap(newNode, faces);
			}
		}
	}

	void initFaces() {
		for (int i = 0; i < model->meshes.size(); i++) {
			faces.push_back(new Face(&model->meshes[i]));
		}
		
		// Largest face is the base
		Face* largest = faces[0];

		for (int i = 1; i < faces.size(); i++) {
			if (faces[i]->getArea() > largest->getArea()) {
				largest = faces[i];
			}
		}
		
		// make the faceMap
		faceMap = Graph<Face>(largest);
		populateFaceMap(faceMap.rootNode, faces);

		// use the facemap to assign original angles to each of the faces
		for (int i = 0; i < faces.size(); i++) {
			Graph<Face>::Node* current = faceMap.findNode(faceMap.rootNode, faces[i]);

			// search through all of the nodes and find the angles between them and their connections.
			for (int j = 0; j < current->connections.size(); j++) {
				// identify shared axis
				for (int g = 0; g < current->connections[j]->data->axis.size(); g++) {
					for (int h = 0; h < current->data->axis.size(); h++) {
						if (*current->data->axis[h] == *current->connections[j]->data->axis[g]) {
							// identify leftover points to compare to find the angle
							glm::vec3 vertex1;
							glm::vec3 vertex2;

							for (int k = 0; k < current->data->mesh->indices.size(); k++) {
								// if point is not on axis
								vertex1 = current->data->mesh->vertices[current->data->mesh->indices[k]].Position;
								if (vertex1 != current->data->axis[h]->point1 && vertex1 != current->data->axis[h]->point2) {
									break;
								}
							}

							for (int k = 0; k < current->connections[j]->data->mesh->indices.size(); k++) {
								// if point is not on axis
								vertex2 = current->connections[j]->data->mesh->vertices[current->connections[j]->data->mesh->indices[k]].Position;
								if (vertex2 != current->connections[j]->data->axis[g]->point1 && vertex2 != current->connections[j]->data->axis[g]->point2) {
									break;
								}
							}

							// set axis original angle.
							float angle = current->data->axis[h]->orientedAngle(vertex1, vertex2);
							current->data->axis[h]->originalAngle = angle;
							current->connections[j]->data->axis[g]->originalAngle = angle;
						}
					}
				}
			}
		}
	}
};

#endif
