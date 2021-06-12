#ifndef UNFOLDINGSHAPES_H
#define UNFOLDINGSHAPES_H

#include <filesystem>

#include <QtWidgets/QMainWindow>
#include <QMouseEvent>
#include <QtWidgets/qfiledialog.h>
#include <QtCore/qtimer.h>
#include "ui_UnfoldingShapes.h"
#include "OpenGLWidget.h"

#include "Shape.h"
#include "Animator.h"

class UnfoldingShapes : public QMainWindow
{
    Q_OBJECT

public:
    UnfoldingShapes(QWidget *parent = Q_NULLPTR) : QMainWindow(parent)
	{
		ui.setupUi(this);

		// local init (the rest must be delayed because gl initializes after this)
		focusedShape = nullptr;

		// apply button
		connect(ui.applyProperties, &QPushButton::released, this, &UnfoldingShapes::applySettings);

		// add shape button
		connect(ui.selectFileButton, &QPushButton::released, this, &UnfoldingShapes::selectFile);

		// add folder shape button
		connect(ui.selectFolderButton, &QPushButton::released, this, &UnfoldingShapes::selectFolder);

		// select shape in list
		connect(ui.listWidget, &QListWidget::itemClicked, this, &UnfoldingShapes::selectShape);

		// render menu connections
		connect(ui.enableTable, &QCheckBox::stateChanged, this, &UnfoldingShapes::checkTable);

		// enable controls
		ui.openGLWidget->installEventFilter(this);

		// setup properties page
		ui.propertiesTabWidget->setCurrentIndex(0);
	}

	OpenGLWidget* getGraphics() {
		return ui.openGLWidget;
	}

	// setup
	// set the callback to be called after gl has initialized within the widget
	void setDelayedSetup(void m(OpenGLWidget*)) {
		ui.openGLWidget->setAfterGLInit(m);
	}

	// the function that should be called once the runner and gl have been initialized
	void delayedSetup() {
		mouse = new Mouse();

		cameraSetup();

		tableBounds = glm::vec2(15, 18) * 0.75f;
	}

	void linkShapes(vector<Shape*>* shapes) {
		this->shapes = shapes;

		//setupBackBoard();
	}

	void linkAnimator(Animator* animator) {
		this->animator = animator;
	}

	void cameraSetup() {
		origin = glm::vec3(0);
		
		maxZoom = 100;
		minZoom = 1;
		maxSteps = 10000;
		currentZoomStep = 7640;
		zoom = calculateZoom(currentZoomStep);
		currentMousePos = glm::vec2(0);

		// set camera starting pos
		//ui.openGLWidget->camera.setPos(origin + glm::normalize(glm::vec3(0.0f, 0.5f, -1.0f)) * zoom);
		ui.openGLWidget->camera.setPos(origin + glm::normalize(glm::vec3(0.0f, 0.75f, -1.0f)) * zoom);

		ui.openGLWidget->camera.lookAtTarget(origin);
	}

	bool eventFilter(QObject *obj, QEvent *event) override {
		if (obj == ui.openGLWidget) {
			if (event->type() == QEvent::MouseMove
				|| event->type() == QEvent::MouseButtonPress
				|| event->type() == QEvent::MouseButtonRelease) {
				updateMouse(event);
			}
			else if (event->type() == QEvent::Wheel) {
				processScroll(static_cast<QWheelEvent *>(event));
			}
		}

		return false;
	}

	void updateMouse(QEvent* event) {
		if (mouse == nullptr) {
			return;
		}

		// MOUSE MOVEMENT of CAMERA
		// Make the mouse input addative (so the camera stays in the same position when the mouse releases and then presses again)
		// this way when the button is first placed, the large jump is ignored since mouse resets the positon and pos - mouse->pos is zero again
		if (event->type() == QEvent::MouseMove) {
			QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
			glm::vec2 pos = glm::vec2(mouseEvent->localPos().x(), mouseEvent->localPos().y());
			currentMousePos = currentMousePos + pos - mouse->pos;
		}

		mouse->update(event);

		// apply camera rotation for the viewer and transformation
		ui.openGLWidget->camera.mouseInputPOV(currentMousePos.x, currentMousePos.y);

		// apply limits to the rotation (so we don't clip the table
		if (ui.openGLWidget->camera.pitch > 0) {
			ui.openGLWidget->camera.pitch = 0;
		}

		ui.openGLWidget->camera.setPos(origin + ui.openGLWidget->camera.Front * -1.0f * zoom);
	}

	// apply zoom
	void processScroll(QWheelEvent *event) {
		currentZoomStep -= event->angleDelta().y();

		zoom = calculateZoom(currentZoomStep);

		if (zoom < 0) {
			zoom = 0;
		}

		ui.openGLWidget->camera.setPos(origin + ui.openGLWidget->camera.Front * -1.0f * zoom);
	}

	bool mouseInsideWindow(glm::vec2 pos) {
		if (pos.x > 0 && pos.y > 0 && pos.x < width() && pos.y < height()) {
			return true;
		}

		return false;
	}

	// runtime shape viewer things
	void selectShape(QListWidgetItem* item) {
		// ignore if nothing is selected
		if (ui.listWidget->currentRow() != -1) {
			// set current selection
			Shape* shape = (*shapes)[ui.listWidget->currentRow()];

			focusShape(shape);
		}
	}

	// assumes shape pointer is already added to the shapes list
	void focusShape(Shape* shape) {
		// check if the client is emptying the focus shape
		if (shape == nullptr) {
			focusedShape = nullptr;
			return;
		}

		// move the current focus back to its position
		//backboard->applyTransform();

		// stop current animation of the focused shape
		if (focusedShape != nullptr) {
			animator->getAnimation(focusedShape)->stop();
			focusedShape->asset->visible = false;
		}

		focusedShape = shape;
		
		focusedShape->asset->visible = true;
		focusedShape->asset->setRotation(glm::vec3(0));

		// autostart animation
		applySettings();
	}

	// runtime stuff
	void applySettings() {
		// retrieve info

		// make sure a shape is selected in the menu
		if (focusedShape != nullptr) {
			Shape* current = focusedShape;

			int unfoldSetting = ui.unfoldMethodInput->currentIndex();
			int animationSetting = ui.animationMethodInput->currentIndex();
			float speed = ui.speedInput->value();
			float scale = ui.scaleInput->value();

			setUnfold(current, unfoldSetting);

			// align the y position correctly
			focusedShape->asset->position = origin - focusedShape->getBasePos();

			// measure unfold bounds to adjust position to
			orientUnfoldShape(current, glm::vec2(origin.x, origin.y) - (tableBounds * 0.5f), glm::vec2(origin.x, origin.y) + (tableBounds * 0.5f));

			// startup animator
			Animator::Animation* animation = animator->getAnimation(current);
			animation->setAlgorithm(animationSetting);
			animation->speed = speed;

			animation->progress = 0;

			animation->play();
		}
	}

	void selectFile() {
		QString fileName = QFileDialog::getOpenFileName(this, tr("Open Shape"), "", tr("OBJ File (*.obj)"));

		// exit if there is no input
		if (fileName == "") { return; }

		string strFileName = fileName.toLocal8Bit().data();

		string formatted = formatPath(strFileName);

		std::cout << std::endl << "Added shape from: " << formatted << std::endl;

		ui.openGLWidget->makeCurrent();
		addShapeFromFile(formatted.c_str());
		ui.openGLWidget->doneCurrent();
	}

	// add all obj files from the specified directory
	void selectFolder() {
		QString fileName = QFileDialog::getExistingDirectory(this, tr("Open Directory"), "", QFileDialog::ShowDirsOnly);

		// exit if there is no input
		if (fileName == "") { return; }

		string strFileName = fileName.toLocal8Bit().data();

		addShapesFromFolder(strFileName);
	}

	// return the filename of a file (eg: input="file.txt" output="file")
	string getFileName(string path) {
		string output = "";

		bool passed = false;
		for (int i = path.length() - 1; i >= 0; i--) {
			if (path[i] == '.') {
				passed = true;
				i--;
			}
			if (path[i] == '/' || path[i] == '\\') {
				break;
			}
			if (passed == true) {
				output = path[i] + output;
			}
		}

		return output;
	}

	// get the file type off the end of an extension (eg: input="file.txt" output="txt")
	string getFileType(string path) {
		string output = "";

		for (int i = path.length() - 1; i >= 0; i--) {
			if (path[i] == '.') {
				break;
			}

			output = path[i] + output;
		}

		return output;
	}

	// enter filepath and it will turn all '/' into "\\"
	string formatPath(string str) {
		// convert file into a readable format for the program
		string formatted = "";
		for (int i = str.length(); i >= 0; i--) {
			if (str[i] == '/') {
				formatted = "\\" + formatted;
			}
			else {
				formatted = str[i] + formatted;
			}
		}

		return formatted;
	}

	// enter the unformatted filepath (eg: "C:/Users/user1/shapes")
	void addShapesFromFolder(string folderPath) {
		for (const auto & file : std::filesystem::directory_iterator::directory_iterator(folderPath)) {
			string path = file.path().string();

			if (getFileType(path) == "obj") {
				ui.openGLWidget->makeCurrent();
				addShapeFromFile(formatPath(path).c_str());
				ui.openGLWidget->doneCurrent();
			}
		}
	}

	void addShapeFromFile(const char* str) {
		Shape* newShape = new Shape(str, ui.openGLWidget);
		
		shapes->push_back(newShape);
		ui.openGLWidget->addAsset((*shapes)[shapes->size() - 1]->asset);

		addShapeToList((*shapes)[shapes->size() - 1]);
	}

	// set the visibility of the table based on check box
	void checkTable(int state) {
		bool enabled;
		switch (state) {
		case 0:
			enabled = false;
			break;
		default:
			enabled = true;
			break;
		}

		Asset* asset = ui.openGLWidget->getAsset("LP_worksplace");
		if (asset != nullptr) {
			asset->visible = enabled;
		}
	}

	// Change the position and scale of a shape so that when it is unfolded, it fits within the specified bounds
	// specify the bounds with the rectangle formed by corner 1 and corner 2
	void orientUnfoldShape(Shape* shape, glm::vec2 corner1, glm::vec2 corner2) {
		glm::vec2 unfoldCorner1, unfoldCorner2;
		std::tie(unfoldCorner1, unfoldCorner2) = Unfold::findUnfoldSize(shape);

		glm::vec2 bounds = glm::abs(corner2 - corner1);
		glm::vec2 unfoldBounds = glm::abs(unfoldCorner2 - unfoldCorner1);

		// POSITION
		glm::vec2 newBounds = glm::vec2(0);
		// decide which dimension to lock the unfold to (width or height)
		// if unfoldBounds height is larger proportionally than the table bounds
		float scaleFactor;
		if (bounds.y / bounds.x < unfoldBounds.y / unfoldBounds.x) {
			// process height based
			newBounds.x = bounds.y * (unfoldBounds.x / unfoldBounds.y);
			newBounds.y = bounds.y;

			scaleFactor = newBounds.y / unfoldBounds.y;
		}
		else {
			// process width based
			newBounds.y = bounds.x * (unfoldBounds.y / unfoldBounds.x);
			newBounds.x = bounds.x;

			scaleFactor = newBounds.x / unfoldBounds.x;
		}
		shape->asset->setScale(glm::vec3(scaleFactor));

		//std::cout << glm::to_string(bounds) << " " << glm::to_string(newBounds) << std::endl;

		// unfold corner 1 is the smallest x and y so we use that to orient the center
		// find offset of center and then scale it
		glm::vec2 transformCenter = -1.0f * vec2Mult((unfoldCorner1 + unfoldBounds * 0.5f), glm::vec2(newBounds.x / unfoldBounds.x, newBounds.y / unfoldBounds.y));

		// orient the corner to corresponding position
		//shape->asset->setPosition(glm::vec3(transformCenter.x, shape->asset->position.y, transformCenter.y) + origin - glm::vec3((bounds.x - newBounds.x) / 2.0f, 0.0f, (bounds.y - newBounds.y) / 2.0f));
		shape->asset->setPosition(glm::vec3(transformCenter.x, shape->asset->position.y, transformCenter.y) + origin);

		// adjust y pos so it stays ontop of the table
		shape->asset->setPosition(shape->asset->position * glm::vec3(1.0f, scaleFactor, 1.0f));

		//std::cout << std::endl << "Final Bounds at pos " << glm::to_string(shape->asset->position) << " and a scale factor of " << scaleFactor << std::endl;
		Unfold::findUnfoldSize(shape);
	}

	void setupBackBoard() {
		//backboard = new Backboard(this, shapes, glm::vec3(0, 10, 50));
	}

	// utility
	glm::vec2 vec2Mult(glm::vec2 mult1, glm::vec2 mult2) {
		return glm::vec2(mult1.x*mult2.x, mult1.y*mult2.y);
	}

	float calculateZoom(int step) {
		float logMinZoom = log(minZoom);
		float logMaxZoom = log(maxZoom);

		float logZoom = logMinZoom + (logMaxZoom - logMinZoom) * step / (maxSteps - 1);
		
		return exp(logZoom);
	}

	bool setUnfold(Shape* shape, int index) {
		switch (index) {
		case 0:
			shape->setUnfold(Unfold::basic(shape));
			break;
		case 1:
			shape->setUnfold(Unfold::randomBasic(shape));
			break;
		case 2:
			shape->setUnfold(Unfold::breadthUnfold(shape));
			break;
		case 3:
			shape->setUnfold(Unfold::randomBreadthUnfold(shape));
			break;
		default:
			return false;
			break;
		}

		return true;
	}

	void addShapeToList(Shape* shape) {
		shape->asset->visible = false;
		ui.listWidget->addItem(shape->name.c_str());

		// apply settings for base setup
		// add the animation if the unfold generates successfully.
		if (setUnfold(shape, 2)) {
			animator->addAnimation(shape, true);
		}

		//backboard->applyTransform();
	}

	struct Backboard : QObject {
		UnfoldingShapes* parent;

		vector<Shape*>* shapes;

		// update timer for backboard animation
		QTimer* timer;
		int updateLoopHZ = 30;
		float rotationSpeed = 2;

		// top left
		glm::vec3 position;

		// width of board before wrapping to the bottom
		int width;

		//seperation between the shapes
		glm::vec2 seperation;

		// position is the top center of the backboard
		Backboard(UnfoldingShapes *parent, vector<Shape*>* shapes, glm::vec3 position, int width = 5, glm::vec2 seperation = glm::vec2(5,5)) : QObject(nullptr) {
			this->parent = parent;
			this->shapes = shapes;
			this->width = width;
			this->seperation = seperation;
			this->position = position;

			applyTransform();

			// setup update loop
			timer = new QTimer(this);
			QObject::connect(timer, &QTimer::timeout, this, &Backboard::updateLoop);
			timer->start(updateLoopHZ);
		}

		// move all the assets of the shapes to their proper position
		void applyTransform() {
			glm::vec3 pos = getTopLeft(position);

			int col = 0;
			int row = 0;

			for (int i = 0; i < shapes->size(); i++) {
				(*shapes)[i]->asset->setPosition(pos + glm::vec3(col*seperation.x, -1 * row * seperation.y, 0));
				(*shapes)[i]->asset->setRotation(glm::vec3(0));

				col++;

				// new line
				if (col >= width) {
					col = 0;
					row++;
				}
			}
		}

		void updateLoop() {
			// rotate the shapes on the backboard if it is not focused
			for (int i = 0; i < shapes->size(); i++) {
				if ((*shapes)[i] != parent->focusedShape) {
					(*shapes)[i]->asset->setRotation((*shapes)[i]->asset->rotation + glm::normalize(glm::vec3(1, 2, 3)) * rotationSpeed);
				}
				else {
					// do nothing
				}
			}
		}

		// utility
		// transform so the top left of the board is marked for transforms
		glm::vec3 getTopLeft(glm::vec3 ) {
			return position - glm::vec3((width - 1) * this->seperation.x, 0, 0);
		}
	};

private:
    Ui::UnfoldingShapesClass ui;

	vector<Shape*>* shapes;
	Animator* animator;

	// viewer pointers
	Shape* focusedShape;

	//Backboard* backboard;

	// camera settings
	glm::vec3 origin;
	float zoom;
	float currentZoomStep;
	float minZoom;
	float maxZoom;
	float maxSteps;

	// Shape bounds
	glm::vec2 tableBounds;

	// mouse stuff

	struct Mouse {
		UnfoldingShapes* parent = nullptr;

		glm::vec2 pos;

		bool left;
		bool right;

		Mouse() {
			this->parent = parent;

			left = false;
			right = true;

			pos = glm::vec2(0);
		}

		void update(QEvent* event) {
			QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

			if (event->type() == QEvent::MouseButtonPress) {
				left = true;
			}

			if (event->type() == QEvent::MouseButtonRelease) {
				left = false;
			}

			pos = glm::vec2(mouseEvent->localPos().x(), mouseEvent->localPos().y());
		}
	};

	Mouse* mouse = nullptr;

	glm::vec2 currentMousePos;
};

#endif
