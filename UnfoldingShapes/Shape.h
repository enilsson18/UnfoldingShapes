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

#include "Face.h"
#include "Graph.h"
#include "Axis.h"

#include "OpenGLWidget.h"

inline string getNameFromPath(string path);

class Shape {
public:
	struct Transformation {
		float deltaAngle;
		Axis<Face> axis;

		vector<Face*> appliedFaces;

		Transformation(float deltaAngle, Axis<Face>* axis, Face* appliedFace) {
			this->deltaAngle = deltaAngle;
			this->axis = *axis;

			// clear and add one
			this->appliedFaces = vector<Face*>();
			this->appliedFaces.push_back(appliedFace);
		}

		Transformation(float deltaAngle, Axis<Face>* axis, vector<Face*> appliedFaces = vector<Face*>()) {
			this->deltaAngle = deltaAngle;
			this->axis = *axis;
			this->appliedFaces = appliedFaces;
		}

		// apply the transformation
		void apply() {
			//std::cout << deltaAngle << std::endl;
			for (int i = 0; i < appliedFaces.size(); i++) {
				// rotate vertices
				for (int j = 0; j < appliedFaces[i]->mesh->vertices.size(); j++) {
					appliedFaces[i]->mesh->vertices[j].Position = axis.rotateAbout(appliedFaces[i]->mesh->vertices[j].Position, deltaAngle);
				}

				// rotate axis
				for (int j = 0; j < appliedFaces[i]->axis.size(); j++) {
					appliedFaces[i]->axis[j]->rotateAxisAbout(&axis, deltaAngle);
				}
			}
		}

		// revert the transformation
		void revert() {
			for (int i = 0; i < appliedFaces.size(); i++) {
				// rotate vertices
				for (int j = 0; j < appliedFaces[i]->mesh->vertices.size(); j++) {
					//appliedFaces[i]->mesh->vertices[j].Position = appliedFaces[i]->mesh->backupVertices[j].Position;

					appliedFaces[i]->mesh->vertices[j].Position = axis.rotateAbout(appliedFaces[i]->mesh->vertices[j].Position, -deltaAngle);
					//std::cout << glm::to_string(appliedFaces[i]->mesh->vertices[j].Position) << std::endl;
				}

				// rotate axis
				for (int j = 0; j < appliedFaces[i]->axis.size(); j++) {
					appliedFaces[i]->axis[j]->rotateAxisAbout(&axis, -deltaAngle);
					appliedFaces[i]->axis[j]->revert();
				}
			}
		}
	};

	Asset* asset;
	Model* model;
	
	vector<Face*> faces;
	Graph<Face> faceMap;

	Graph<Face>* unfold;

	// stores the transformations applied to the shape so we can revert.
	vector<Transformation> appliedTransformations;

	string name;

	// inactive
	Shape() {
		asset = nullptr;
	}

	// init Shape by setting the asset and registering all of the faces.
	// Copies the model because we are manipulating the face and vertex info.
	Shape(string const &path, OpenGLWidget* graphics, glm::vec3 pos = glm::vec3(0), glm::vec3 rot = glm::vec3(0), glm::vec3 scale = glm::vec3(1)) {
		name = getNameFromPath(path);
		std::cout << "started loading: " << name << std::endl;

		this->model = new Model(&(graphics->f), path, graphics->samples);
		std::cout << "Meshes: " << this->model->meshes.size() << std::endl;
		asset = new Asset(this->model, pos, rot, scale);

		initFaces();

		std::cout << "finished loading: " << name << std::endl;
	}

	void setUnfold(Graph<Face>* newSolution) {
		revert();

		unfold = newSolution;
	}

	// add transformation to the shape
	void transform(float deltaAngle, Axis<Face>* axis, vector<Face*> appliedFaces) {
		appliedTransformations.push_back(Transformation(deltaAngle, axis, appliedFaces));
		appliedTransformations[appliedTransformations.size() - 1].apply();
	}

	// undo transformations
	void revert() {
		// revert transformations and clear list
		for (int i = appliedTransformations.size() - 1; i >= 0; i--) {
			appliedTransformations[i].revert();

			appliedTransformations.erase(appliedTransformations.begin() + i);
		}
	}

	// returns the local position of the base
	glm::vec3 getBasePos() {
		if (faceMap.rootNode == nullptr) {
			return glm::vec3(0);
		}
		
		return faceMap.rootNode->data->mesh->getAvgPos();
	}

private:
	// recursivley populate the faceMap
	void populateFaceMap(Graph<Face>::Node* node, vector<Face*> &faces) {
		vector<Face*> tempFaces = vector<Face*>();

		addNeighbors(tempFaces, node, faces);

		// std::cout << tempFaces.size() << std::endl;

		// add all children and proccess them too
		// std::cout << "tempFaces size: " << tempFaces.size() << std::endl;
		for (int i = 0; i < tempFaces.size(); i++) {
			// this prevents the recursive method from getting stuck in a loop (only run populateFaceMap once for each face)
			bool exists = false;
			if (node->graph->findNode(node->graph->rootNode, tempFaces[i]) != nullptr) {
				exists = true;
			}

			Graph<Face>::Node* newNode = node->graph->newNode(node, tempFaces[i], true);

			if (exists == false) {
				populateFaceMap(newNode, faces);
			}
		}
	}

	// set the neighbors for every axis
	void addNeighbors(vector<Face*> &outputFaces, Graph<Face>::Node* node, vector<Face*> &faces) {
		// std::cout << "faces size: " << faces.size() << std::endl;
		for (int i = 0; i < faces.size(); i++) {
			for (int g = 0; g < faces[i]->axis.size(); g++) {
				for (int h = 0; h < node->data->axis.size(); h++) {
					//std::cout << glm::to_string(node->data->axis[g]->line) << " " << glm::to_string(node->data->axis[g]->point) << std::endl;
					//std::cout << glm::to_string(node->data->axis[g]->line) << " " << glm::to_string(node->data->axis[g]->point) << std::endl << std::endl;

					// if the axis match and they are not the same face
					if (*faces[i]->axis[g] == *node->data->axis[h] && node->data != faces[i]) {
						// set neighbors of each axis for pairing
						faces[i]->axis[g]->setNeighbor(node->data, node->data->axis[h]);
						node->data->axis[h]->setNeighbor(faces[i], faces[i]->axis[g]);

						outputFaces.push_back(faces[i]);
					}
				}
			}
		}
	}

	// set the pointers of each axis for the other connected axis and the pointer to the other neighboring face
	void initAxisInfo() {
		// use the facemap to assign original angles to each of the faces
		for (int i = 0; i < faces.size(); i++) {
			Graph<Face>::Node* current = faceMap.findNode(faceMap.rootNode, faces[i]);

			if (current != nullptr) {
				// search through all of the nodes and find the angles between them and their connections.
				for (int j = 0; j < current->connections.size(); j++) {
					for (int h = 0; h < current->data->axis.size(); h++) {
						// make sure the axis is valid and has a neighbor
						if (current->data->axis[h]->sharedAxis != nullptr) {
							// identify leftover points to compare to find the angle
							glm::vec3 vertex1 = current->data->mesh->getAvgPos();
							glm::vec3 vertex2 = current->data->axis[h]->neighborFace->mesh->getAvgPos();

							/*
							for (int k = 0; k < current->data->mesh->indices.size(); k++) {
								// if point is not on axis
								vertex1 = current->data->mesh->vertices[current->data->mesh->indices[k]].Position;
								if (current->data->axis[h]->hasPoint(vertex1) == false) {
									break;
								}
							}

							for (int k = 0; k < current->data->axis[h]->neighborFace->mesh->indices.size(); k++) {
								// if point is not on axis
								vertex2 = current->data->axis[h]->neighborFace->mesh->vertices[current->data->axis[h]->neighborFace->mesh->indices[k]].Position;
								if (current->data->axis[h]->sharedAxis->hasPoint(vertex2) == false) {
									break;
								}
							}

							*/

							// set axis original angle.
							float angle = current->data->axis[h]->orientedAngle(vertex1, vertex2);
							current->data->axis[h]->originalAngle = angle;
							// current->data->axis[h]-sharedAxis->originalAngle = angle;
						}
					}
				}
			}
		}
	}

	// rotate the entire shape so that the base face is perfectly level to the ground
	void levelBase() {
		// find normal of base
		glm::vec3 normal = faceMap.rootNode->data->mesh->getNormal();
		glm::vec3 line = normal * glm::vec3(1,0,1);
		if (line.x != 0) {
			line.x = 1 / line.x;
		}
		if (line.z != 0) {
			line.z = 1 / line.z;
		}

		line = glm::vec3(1 / line.x, line.y, 1 / line.z);

		glm::vec3 pos = faceMap.rootNode->data->mesh->getAvgPos();

		Axis<Face>* axis = new Axis<Face>(line * -1.0f, line * 1.0f);

		float deltaAngle = axis->orientedAngle(pos, glm::vec3(0, -1, 0));

		// FIND A WAY TO ROTATE EULER ANGLES BY AXIS
	}

	void initFaces() {
		for (int i = 0; i < model->meshes.size(); i++) {
			faces.push_back(new Face(&model->meshes[i]));

			// faces[i]->printAxis();
		}

		// if there are no faces the return null
		/*
		if (faces.size() == 0) {
			return;
		}
		*/
		
		// Largest face is the base (starting with the face furthest downward)
		Face* largest = faces[0];

		for (int i = 1; i < faces.size(); i++) {
			if (faces[i]->mesh->getAvgPos().y < largest->mesh->getAvgPos().y) {
				largest = faces[i];
			}
		}

		/*
		// sort by the largest side aswell
		for (int i = 1; i < faces.size(); i++) {
			if (faces[i]->getArea() > largest->getArea()) {
				largest = faces[i];
			}
		}
		*/

		// make sure that the model is rotated so the base is parallel to the ground
		//levelBase();

		// make the faceMap
		faceMap = Graph<Face>(largest);
		populateFaceMap(faceMap.rootNode, faces);

		initAxisInfo();
	}
};

string getNameFromPath(string path) {
	string fileName = "";

	for (int i = 0; i < path.size(); i++) {
		if (path[i] == '.') {
			for (int j = i; j >= 0; j--) {
				if (path[j] == '\\' || path[j] == '/') {
					fileName = path.substr(j + 1, i - (j + 1));
					return fileName;
				}
			}
		}
	}

	return "";
}

#endif
