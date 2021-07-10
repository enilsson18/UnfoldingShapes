# Unfolding Shapes

This project aims to take in 3d models and unwrap them onto a 2d plane in one piece with an animation. In other words, it is creating a geometric net of any 3d model. 

![Geometric Net Image](https://www.onlinemathlearning.com/image-files/nets-of-solids.png)

The project also contains a UI that can manipulate the settings and conditions of the pattern formed by the unfolded shape and can control the animation style of the transformation between the shape and the unfold.

## Note

Due to issues with removing git lfs from the project, some builds were lost. Two integral builds were luckily salvaged so you can see what the current state of the project is and what it started out as.

## Dependencies

All dependencies other than QT are included within the project files and are compiled statically through lib files. If you want to import the project and its dependencies then please follow the instruction in the section labeled "Importing Project Instructions".

OpenGL: https://www.opengl.org//
GLFW: https://www.glfw.org/
QT: https://www.qt.io/
Assimp: http://assimp.org/

## Downloading Compiled Project Instructions

Head to the releases page of the Github repo and download the latest version. After downloading, extract the zip file into a folder and try to run the EXE file. If the console throws an error, try running the included VC_redist.x64.exe installer. If further problems persist then feel free to contact me through github.

Once you have the program running, there are many sample models to try in the resources/object/ folder within the downloaded files.

## Importing Project Instructions

When cloning the repo and opening the Visual Studios project file, you must follow several steps.

1. Download the [Qt Online Installer](https://www.qt.io/download-qt-installer). 

2. Make an account and click through the prompts until you reach the installation folder.

3. Select "C:\Qt" as the installation folder and check the custom installation box. Click Next.

4. When selecting components, expand the Qt tab followed by the Qt 5.14.2 tab. Select MSVC 2017 x64 and click next.

5. Click through the remaining prompts and then click Install.

6. Open Visual Studios 2017 and install the [Qt Visual Studio tools extension](https://doc.qt.io/qtvstools/qtvstools-getting-started.html#:~:text=one%20Qt%20version.-,Installing%20Qt%20VS%20Tools,or%20update%20Qt%20VS%20Tools.)

7. Set the runtime versions to RELEASE and x64.

8. Attempt to build the code once and then copy the contents of "/LibResources/runtime bin" from the cloned repo into "/x64/Release"

## Notable Code Overview

An integral part of the development was creating a method to represent the loaded shape which could be manipulated. The solution I came up with is the Shape Class. It takes in the path to an OBJ 3d model file and then uses Assimp to view the model's raw data. The faces and edges are then calculated to create a skeleton for which the unfolding occurs. The unfolding patterns can then be generated and finally animated through the Transformation struct. Since the animation is based on the increment of a progress decimal 0.0-1.0, there must be a way to calculate frames in the animation. We can identify the angle on each edge of an unfold through the vertices of the neighboring faces. The transformation of rotation about each edge is then processed for each descending child face of unfolding pattern about a root/base face which starts flat on the table.

## Algorithms

As of now, several key algorithms are used to generate both unfold patterns and animations.

* Unfold
	* Depth-First: An unfolding pattern that follows a depth-first graph traversal design.
	* Breadth-First: An unfolding pattern that follows a breadth-first graph traversal design.
	* Randomness: Both Depth-First and Breadth-First use the first available child of a node when searching. This means that the result is the same every time. Adding randomness to these algorithms changes the order of the children processed for each node in the graph.
* Animation
	* Step Based: Each unfold pattern can be viewed as a tree data structure. The Step Based animation applies the transformation to each layer of the tree sequentially. This creates the effect of the shape unrolling one joint layer at a time.
	* Continuous: All joints in the animation are rotated simultaneously.