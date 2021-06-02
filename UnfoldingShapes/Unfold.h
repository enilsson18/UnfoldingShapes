#ifndef UNFOLD_H
#define UNFOLD_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <iostream>
#include <vector>

#include "Model.h"
#include "Mesh.h"

#include "Face.h"
#include "Graph.h"
#include "Shape.h"
#include "Axis.h"

//prototypes
template<class RandomIt>
void random_shuffle(RandomIt first, RandomIt last);

// computes unfold solutions
static class Unfold {
private:
	static void basicRecusivePopulation(Graph<Face>::Node* mapNode, Graph<Face>* solution, Graph<Face>::Node* parent) {
		for (int i = 0; i < mapNode->connections.size(); i++) {
			if (solution->findNode(solution->rootNode, mapNode->connections[i]->data) == nullptr) {
				// set new parent
				parent = solution->newNode(parent, mapNode->connections[i]->data);
				
				// recursive
				basicRecusivePopulation(mapNode->connections[i], solution, parent);
			}
		}
	}

	static void randomBasicRecusivePopulation(Graph<Face>::Node* mapNode, Graph<Face>* solution, Graph<Face>::Node* parent) {
		// randomly shuffle the connections order
		vector<Graph<Face>::Node*> randConnections = mapNode->connections;
		random_shuffle(randConnections.begin(), randConnections.end());

		for (int i = 0; i < randConnections.size(); i++) {
			if (solution->findNode(solution->rootNode, randConnections[i]->data) == nullptr) {
				// set new parent
				parent = solution->newNode(parent, randConnections[i]->data);

				// recursive
				basicRecusivePopulation(randConnections[i], solution, parent);
			}
		}
	}

	static void breadthPopulation(Graph<Face>::Node* root, Graph<Face>* solution) {
		vector<Graph<Face>::Node*> queue;
		vector<Graph<Face>::Node*> visited;

		queue.push_back(root);
		visited.push_back(root);

		Graph<Face>::Node* current;

		while (!queue.empty()) {
			current = queue[0];
			queue.erase(queue.begin());

			//std::cout << "New Animation Frame:" << std::endl;

			Graph<Face>::Node* currentSolutionNode = solution->findNode(solution->rootNode, current->data);
			for (int i = 0; i < current->connections.size(); i++) {
				bool inList = false;

				for (int x = 0; x < visited.size(); x++) {
					if (visited[x] == current->connections[i]) {
						inList = true;
						break;
					}
				}

				if (!inList) {
					solution->newNode(currentSolutionNode, current->connections[i]->data);

					visited.push_back(current->connections[i]);
					queue.push_back(current->connections[i]);
				}
			}
		}
	}

	static void randomBreadthPopulation(Graph<Face>::Node* root, Graph<Face>* solution) {
		vector<Graph<Face>::Node*> queue;
		vector<Graph<Face>::Node*> visited;

		queue.push_back(root);
		visited.push_back(root);

		Graph<Face>::Node* current;

		while (!queue.empty()) {
			current = queue[0];
			queue.erase(queue.begin());

			Graph<Face>::Node* currentSolutionNode = solution->findNode(solution->rootNode, current->data);

			// randomly shuffle the connections order
			vector<Graph<Face>::Node*> randConnections = current->connections;
			random_shuffle(randConnections.begin(), randConnections.end());

			for (int i = 0; i < randConnections.size(); i++) {
				bool inList = false;

				for (int x = 0; x < visited.size(); x++) {
					if (visited[x] == randConnections[i]) {
						inList = true;
						break;
					}
				}

				if (!inList) {
					solution->newNode(currentSolutionNode, randConnections[i]->data);

					visited.push_back(randConnections[i]);
					queue.push_back(randConnections[i]);
				}
			}
		}
	}

public:
	static Graph<Face>* basic(Shape* shape) {
		// init solution with the base 
		// std::cout << shape->faceMap.rootNode << std::endl;

		Graph<Face>* solution = new Graph<Face>(shape->faceMap.rootNode->data);

		basicRecusivePopulation(shape->faceMap.rootNode, solution, solution->rootNode);

		return solution;
	}

	static Graph<Face>* randomBasic(Shape* shape) {
		// init solution with the base 
		// std::cout << shape->faceMap.rootNode << std::endl;

		Graph<Face>* solution = new Graph<Face>(shape->faceMap.rootNode->data);

		randomBasicRecusivePopulation(shape->faceMap.rootNode, solution, solution->rootNode);

		return solution;
	}

	static Graph<Face>* breadthUnfold(Shape* shape) {
		// init solution with the base 
		// std::cout << shape->faceMap.rootNode << std::endl;

		Graph<Face>* solution = new Graph<Face>(shape->faceMap.rootNode->data);

		breadthPopulation(shape->faceMap.rootNode, solution);

		// std::cout << solution->size << std::endl;

		return solution;
	}

	static Graph<Face>* randomBreadthUnfold(Shape* shape) {
		// init solution with the base 
		// std::cout << shape->faceMap.rootNode << std::endl;

		Graph<Face>* solution = new Graph<Face>(shape->faceMap.rootNode->data);

		randomBreadthPopulation(shape->faceMap.rootNode, solution);

		// std::cout << solution->size << std::endl;

		return solution;
	}

	// returns the minimum and maximum corners of an unfold on a flat plane (returns "0,0 0,0" if there are no vertices)
	// Shape must have an unfold Assigned!
	// Assumes that the shape is rotated so the root unfold node is perfectly aligned with the xz plane
	static std::tuple<glm::vec2, glm::vec2> findUnfoldSize(Shape* shape) {
		// find the position of each vertex in the unraveled 
		float minx = 0;
		float miny = 0;
		float maxx = 0;
		float maxy = 0;

		// unwrap the shape to measure
		if (shape->unfold == nullptr) {
			return std::make_tuple(glm::vec2(0), glm::vec2(0));
		}

		// must ensure that the shape is normalized before calculations (reverted-see shape transformation struct)
		shape->revert();
		breadthFirstUpdate(shape, shape->unfold, 1.0);

		for (int i = 0; i < shape->faces.size(); i++) {
			vector<Vertex>* vertices = &(shape->faces[i]->mesh->vertices);
			
			for (int j = 0; j < vertices->size(); j++) {
				glm::vec3 pos = (*vertices)[j].Position;

				if (pos.x < minx) {
					minx = pos.x;
				}
				if (pos.x > maxx) {
					maxx = pos.x;
				}
				if (pos.z < miny) {
					miny = pos.z;
				}
				if (pos.z > maxy) {
					maxy = pos.z;
				}
			}
		}

		// reset shape to defualt shape and return
		shape->revert();

		// output the bounds
		//std::cout << "Calculated Bounds -- Minimum (-1, -1) Point: " + glm::to_string(glm::vec2(minx, miny)) + ", Maximum (1, 1) Point: " + glm::to_string(glm::vec2(maxx, maxy)) << std::endl;
		return std::make_tuple(glm::vec2(minx,miny), glm::vec2(maxx,maxy));
	}

	// Functions to apply the unfold

	// Enter a pointer to the list of faces you want to generate and the current root node 
	// The function will add all of the children of the entered node to the list
	static void recursiveChildCompilation(vector<Face*>* list, Graph<Face>::Node* root) {
		// add face of node
		list->push_back(root->data);

		// process children of node
		for (int i = 0; i < root->connections.size(); i++) {
			recursiveChildCompilation(list, root->connections[i]);
		}
	}

	// Enter the shape to manipulate and the root node of the generated unfold graph followed by the progress of the unfold (0.0-1.0)
	// Automatically reverts the shape at the beginning of method
	static void stepBasedUpdate(Shape* shape, Graph<Face>* graph, float progress) {
		// set shape to default orientation before manipulation
		shape->revert();

		// begin manipulation
		// the progress required for each level of faces to unfold
		float miniProgress = 1.0f / graph->size;

		vector<Graph<Face>::Node*> queue;

		queue.push_back(graph->rootNode);

		Graph<Face>::Node* current;

		int index = 0;

		while (queue.size() <= floor(progress / miniProgress)) {
			current = queue[index];

			//std::cout << "New Animation Frame:" << std::endl;

			for (int i = 0; i < current->connections.size(); i++) {
				queue.push_back(current->connections[i]);
			}

			index += 1;
		}

		// catchup all the queue indicies less than current
		for (int z = 0; z < queue.size() && z != floor(progress / miniProgress); z++) {
			current = queue[z];

			// find axis
			Axis<Face>* axis = nullptr;

			for (int i = 0; i < current->connections.size(); i++) {
				for (int x = 0; x < current->data->axis.size(); x++) {
					if (current->data->axis[x]->neighborFace == current->connections[i]->data) {
						axis = current->data->axis[x];

						//axis->print();

						break;
					}
				}

				// apply to children
				if (axis != nullptr) {
					// use the method to add all child faces attatched to the current face for the transformation of the shape.
					vector<Face*> appliedFaces = vector<Face*>();
					recursiveChildCompilation(&appliedFaces, current->connections[i]);

					shape->transform(1 * (axis->originalAngle), axis, appliedFaces);
				}
			}
		}

		// handle latest update
		current = queue[floor(progress / miniProgress)];

		// find axis
		Axis<Face>* axis = nullptr;

		for (int i = 0; i < current->connections.size(); i++) {
			for (int x = 0; x < current->data->axis.size(); x++) {
				if (current->data->axis[x]->neighborFace == current->connections[i]->data) {
					axis = current->data->axis[x];

					//axis->print();

					break;
				}
			}

			// apply to children
			if (axis != nullptr) {
				// use the method to add all child faces attatched to the current face for the transformation of the shape.
				vector<Face*> appliedFaces = vector<Face*>();
				recursiveChildCompilation(&appliedFaces, current->connections[i]);

				shape->transform(1 * (axis->originalAngle) * (fmod(progress, miniProgress) / miniProgress), axis, appliedFaces);
			}
		}
	}

	// Current working solution
	// Enter the shape to manipulate and the root node of the generated unfold graph followed by the progress of the unfold (0.0-1.0)
	// Automatically reverts the shape at the beginning of method
	static void breadthFirstUpdate(Shape* shape, Graph<Face>* graph, float progress) {
		// set shape to default orientation before manipulation
		shape->revert();

		// begin manipulation
		vector<Graph<Face>::Node*> queue;

		queue.push_back(graph->rootNode);

		Graph<Face>::Node* current;

		// debugging tool to identify axis
		int facesVisited = 0;

		while (!queue.empty()) {
			current = queue[0];
			queue.erase(queue.begin());

			//std::cout << "New Animation Frame:" << std::endl;

			for (int i = 0; i < current->connections.size(); i++) {
				facesVisited += 1;

				// find axis
				Axis<Face>* axis = nullptr;

				for (int x = 0; x < current->data->axis.size(); x++) {
					if (current->data->axis[x]->neighborFace == current->connections[i]->data) {
						axis = current->data->axis[x];

						//axis->print();

						break;
					}
				}

				// apply to children
				if (axis != nullptr) {
					// use the method to add all child faces attatched to the current face for the transformation of the shape.
					vector<Face*> appliedFaces = vector<Face*>();
					recursiveChildCompilation(&appliedFaces, current->connections[i]);

					// apply to the shape
					shape->transform(1 * (axis->originalAngle) * progress, axis, appliedFaces);

					queue.push_back(current->connections[i]);
				}
			}
		}
	}
};

template<class RandomIt>
void random_shuffle(RandomIt first, RandomIt last)
{
	srand(time(NULL));
	typename std::iterator_traits<RandomIt>::difference_type i, n;
	n = last - first;
	for (i = n - 1; i > 0; --i) {
		using std::swap;
		swap(first[i], first[std::rand() % (i + 1)]);
		// rand() % (i+1) isn't actually correct, because the generated number
		// is not uniformly distributed for most values of i. A correct implementation
		// will need to essentially reimplement C++11 std::uniform_int_distribution,
		// which is beyond the scope of this example.
	}
}

#endif