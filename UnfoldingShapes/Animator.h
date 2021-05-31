#ifndef ANIMATOR_H
#define ANIMATOR_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

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
#include "Unfold.h"

class Animator {
public:
	struct Animation {
		Shape* shape;

		// controls related stuff
		bool paused;

		// which of the available algortihms is being used
		int activeAlgorithm;

		// milliseconds for animation to complete
		float speed;

		float progress;

		Animation(Shape* shape, bool paused = false, int algorithm = 1, float speed = 1) {
			this->shape = shape;
			this->speed = speed;
			progress = 0.0f;

			this->activeAlgorithm = algorithm;
			this->paused = paused;
		}

		bool isPaused() {
			return paused;
		}

		void pause() {
			paused = true;
		}

		void play() {
			paused = false;
		}

		void stop() {
			paused = true;
			progress = 0;
		}

		// input a decimal 0.0-1.0 to represet percentage
		void scrub(float delta) {
			progress += delta;
		}

		void incrementSpeed(float delta) {
			speed += delta;
		}

		void shuffleAlgorithm() {
			int algorithmCount = 2;

			activeAlgorithm = (activeAlgorithm + 1) % algorithmCount;
		}

		void setAlgorithm(int n) {
			activeAlgorithm = n;
		}
	};

	Animator() {
		animations = new vector<Animation>();
	}

	// main update function for all animations
	void update() {
		for (int i = 0; i < animations->size(); i++) {
			if ((*animations)[i].shape->unfold != nullptr && !(*animations)[i].paused) {
				if ((*animations)[i].progress < 0.0f) {
					// revert the shape to default position since we round up to 0 from negative progress
					(*animations)[i].shape->revert();

					(*animations)[i].progress = 0.0f;
				}
				else if ((*animations)[i].progress < 1.0f) {
					// identify which algorithm to use
					switch ((*animations)[i].activeAlgorithm) {
					case 0: {
						Unfold::stepBasedUpdate((*animations)[i].shape, (*animations)[i].shape->unfold, (*animations)[i].progress);
						break;
					}
					case 1: {
						Unfold::breadthFirstUpdate((*animations)[i].shape, (*animations)[i].shape->unfold, (*animations)[i].progress);
						break;
					}
					}

					//if (!(*animations)[i].paused) {
					//std::cout << (*animations)[i].progress << std::endl;
					(*animations)[i].progress += (*animations)[i].speed / (100 * 7.5f);
					//}
				}
				else if ((*animations)[i].progress > 1.0f) {
					(*animations)[i].progress = 1.0f;

					Unfold::breadthFirstUpdate((*animations)[i].shape, (*animations)[i].shape->unfold, (*animations)[i].progress);
				}

				// rebuild the mesh for each shape
				(*animations)[i].shape->model->rebuildMeshes();
			}
		}
	}

	// check if the animation for the shape already exists and if not then create it
	void addAnimation(Shape* shape, bool paused = false, int algorithm = 1, float speed = 1) {
		bool found = false;

		if (!found) {
			animations->push_back(Animation(shape, paused, algorithm, speed));
		}
	}

	// becomes invalid after more animations are added
	Animation* getAnimation(Shape* shape) {
		for (int i = 0; i < animations->size(); i++) {
			if ((*animations)[i].shape == shape) {
				return &(*animations)[i];
			}
		}
	}

private:
	vector<Animation>* animations;
};

#endif