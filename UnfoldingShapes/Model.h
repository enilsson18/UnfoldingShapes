#ifndef MODEL_H
#define MODEL_H

//#include <glad/glad.h>
//#include <GLFW/glfw3.h>

#include <QtWidgets/qopenglwidget.h>
#include <QtGui/qopenglfunctions_3_3_core.h>

#include "img/stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <shader.h>

#include "Mesh.h"
#include "Camera.h"

#include <vector>
#include <map>
#include <iostream>

inline unsigned int TextureFromFile(QOpenGLFunctions_3_3_Core **f, const char *path, const string &directory, int samples = 1, bool gamma = false);

// prototypes
bool tangantFace(vector<Vertex> vertices1, vector<Vertex> vertices2);

//Do not reinitialize the model
class Model {
public:
	//model data
	vector<Texture> textures_loaded;
	vector<Mesh> meshes;
	string directory;
	string name;
	bool gammaCorrection;

	//multisampling
	int samples;

	//expects file path to 3d model with multisampling
	Model(QOpenGLFunctions_3_3_Core **f, string const &path, int samples, bool gamma = false) : gammaCorrection(gamma)
	{
		this->f = f;

		this->samples = samples;

		loadModel(path);
	}

	//expects file path to 3d model
	Model(QOpenGLFunctions_3_3_Core **f, string const &path, bool gamma = false) : gammaCorrection(gamma)
	{
		this->f = f;

		//default 1 sample
		this->samples = 1;

		loadModel(path);
	}

	//draws the model and all meshes with it according to the shader
	void Draw(Shader &shader, Camera &camera) {
		for (unsigned int i = 0; i < meshes.size(); i++) {
			meshes[i].Draw(shader);
		}
	}

	//sort the meshes before drawing based on camera position (furthest first)
	void DrawSorted(Shader &shader, Camera &camera) {
		vector<int> sorted = vector<int>();
		for (unsigned int i = 0; i < meshes.size(); i++) {
			sorted.push_back(i);
		}

		//ignore the sort if there is only 1 item anyways
		if (meshes.size() > 1) {
			//sort the list of components so the largest distance is first
			bool swapped;
			for (unsigned int i = 0; i < meshes.size(); i++) {
				swapped = false;
				for (unsigned int j = 0; j < meshes.size() - 1 - i; j++) {
					if (glm::distance(meshes[sorted[j]].getAvgPos(), camera.pos) > glm::distance(meshes[sorted[j + 1]].getAvgPos(), camera.pos)) {
						int temp = sorted[j];
						sorted[j] = sorted[j + 1];
						sorted[j + 1] = temp;

						swapped = true;
					}
				}

				//exit search
				if (swapped == false) {
					break;
				}
			}
		}

		for (unsigned int i = 0; i < sorted.size(); i++) {
			meshes[sorted[i]].Draw(shader);
		}
	}

	void rebuildMeshes() {
		for (int i = 0; i < meshes.size(); i++) {
			meshes[i].rebuild();
		}
	}

private:
	QOpenGLFunctions_3_3_Core **f;

	void loadModel(string const &path)
	{
		//read file via ASSIMP
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

		//check for errors
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
		{
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
			return;
		}

		//retrieve the directory path of the filepath
		directory = path.substr(0, path.find_last_of('\\'));

		//retrieve the name of the model file
		//std::size_t found = path.find_last_of('\\');
		//name = path.substr(found + 1);
		name = getNameFromPath(path);
		//std::cout << "Name: " << name << std::endl;

		//process ASSIMP's root node recursively
		processNode(scene->mRootNode, scene);
	}

	//seperate and process each mesh from the nodes in the scene
	void processNode(aiNode *node, const aiScene *scene)
	{
		//process all the node's meshes (if any)
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];

			vector<Mesh> tempMeshes = processMesh(mesh, scene);

			for (int x = 0; x < tempMeshes.size(); x++) {
				meshes.push_back(tempMeshes[x]);
			}
		}
		//do the same for each of its children
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			processNode(node->mChildren[i], scene);
		}
	}

	vector<Mesh> processMesh(aiMesh *mesh, const aiScene *scene)
	{
		vector<Vertex> vertices;
		vector<unsigned int> indices;
		vector<Texture> textures;
		vector<Material> materials;



		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			//process vertex positions, normals and texture coordinates
			//position
			glm::vec3 vector;
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Position = vector;

			//normals
			if (mesh->HasNormals()) {
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex.Normal = vector;
			}
			
			//texture coords
			if (mesh->mTextureCoords[0]) //does the mesh contain texture coordinates?
			{
				//usual coords
				glm::vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;

				//tangent
				vector.x = mesh->mTangents[i].x;
				vector.y = mesh->mTangents[i].y;
				vector.z = mesh->mTangents[i].z;
				vertex.Tangent = vector;

				//bitangent
				vector.x = mesh->mBitangents[i].x;
				vector.y = mesh->mBitangents[i].y;
				vector.z = mesh->mBitangents[i].z;
				vertex.Bitangent = vector;
			}
			else {
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);
			}

			vertices.push_back(vertex);
		}

		//process material
		if (mesh->mMaterialIndex >= 0)
		{
			//assimp material
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

			//setup material data for shader
			Material mat;

			//diffuse
			aiColor3D diffuse(0.0f, 0.0f, 0.0f);
			material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
			mat.diffuse = glm::vec3(diffuse.r, diffuse.g, diffuse.b);

			//specular
			aiColor3D specular(0.0f, 0.0f, 0.0f);
			material->Get(AI_MATKEY_COLOR_SPECULAR, specular);
			mat.specular = glm::vec3(specular.r, specular.g, specular.b);

			//specular shininess
			float shine;
			material->Get(AI_MATKEY_SHININESS, shine);
			mat.shine = shine;

			//specular strength
			float specularStrength;
			material->Get(AI_MATKEY_SHININESS_STRENGTH, specularStrength);
			mat.specularStrength = specularStrength;

			//ambient
			aiColor3D ambient(0.0f, 0.0f, 0.0f);
			material->Get(AI_MATKEY_COLOR_AMBIENT, ambient);
			mat.ambient = glm::vec3(ambient.r, ambient.g, ambient.b);

			//opacity
			float opacity;
			material->Get(AI_MATKEY_OPACITY, opacity);
			mat.opacity = opacity;

			//cout << mat.ambient.x << " " << mat.ambient.y << " " << mat.ambient.z << endl;

			//add material to mesh
			materials.push_back(mat);
			
			//material map setups
			//diffuse maps
			vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
			//specular maps
			vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
			textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
			//normal maps
			std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal");
			textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
			//height maps
			std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_height");
			textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
		}

		/*
		std::cout << "vertices(" << vertices.size() << "): ";
		for (int x = 0; x < vertices.size(); x++) {
			std::cout << vertices[x].Position.x << "," << vertices[x].Position.y << "," << vertices[x].Position.z << " ";
		}
		std::cout << std::endl;

		std::cout << "faces: " << mesh->mNumFaces << std::endl;

		
		std::cout << "indices(" << indices.size() << "): ";
		for (int x = 0; x < indices.size(); x++) {
			std::cout << indices[x] << ", ";
		}
		std::cout << std::endl;
		*/

		//process indices
		//go through each face and retrieve vertex indicies
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

		// consolidate indicies to decide faces
		vector<vector<unsigned int>> consolidatedIndices;

		// populate the temp list
		for (int i = 0; i < indices.size(); i+=3) {
			consolidatedIndices.push_back(vector<unsigned int>());
			consolidatedIndices[consolidatedIndices.size() - 1].push_back(indices[i]);
			consolidatedIndices[consolidatedIndices.size() - 1].push_back(indices[i + 1]);
			consolidatedIndices[consolidatedIndices.size() - 1].push_back(indices[i + 2]);
		}

		// if two faces are tangent then combine them
		for (int i = 0; i < consolidatedIndices.size(); i++) {
			vector<int> toRemove;
			for (int j = i+1; j < consolidatedIndices.size(); j++) {
				vector<Vertex> vertices1;
				vector<Vertex> vertices2;

				vertices1.push_back(vertices[consolidatedIndices[i][0]]);
				vertices1.push_back(vertices[consolidatedIndices[i][1]]);
				vertices1.push_back(vertices[consolidatedIndices[i][2]]);

				vertices2.push_back(vertices[consolidatedIndices[j][0]]);
				vertices2.push_back(vertices[consolidatedIndices[j][1]]);
				vertices2.push_back(vertices[consolidatedIndices[j][2]]);

				// if the two faces are tangant, then add j to i and remove j.
				if (tangantFace(vertices1, vertices2)) {
					toRemove.push_back(j);

					consolidatedIndices[i].push_back(consolidatedIndices[j][0]);
					consolidatedIndices[i].push_back(consolidatedIndices[j][1]);
					consolidatedIndices[i].push_back(consolidatedIndices[j][2]);
				}
			}

			// erase consolidated scrap
			for (int j = toRemove.size() - 1; j >= 0; j--) {
				consolidatedIndices.erase(consolidatedIndices.begin() + toRemove[j]);
			}
		}

		// std::cout << "init packing" << std::endl;

		// pack output meshes with the proper vertices and indicies based on face
		vector<Mesh> output;

		for (int i = 0; i < consolidatedIndices.size(); i++) {
			// std::cout << "Packing face: " << i + 1 << " of " << consolidatedIndices.size() << std::endl;

			vector<Vertex> consolidatedVertices;
			vector<unsigned int> compressedIndices = consolidatedIndices[i];

			/*
			std::cout << "indices 1 (" << compressedIndices.size() << "): ";
			for (int x = 0; x < compressedIndices.size(); x++) {
				std::cout << compressedIndices[x] << ", ";
			}
			std::cout << std::endl;
			*/

			// sort indicies (bubble sort)
			for (int j = 0; j < compressedIndices.size(); j++) {
				for (int g = 0; g < compressedIndices.size() - j - 1; g++) {
					if (compressedIndices[g] > compressedIndices[g+1]) {
						unsigned int temp = compressedIndices[g + 1];
						compressedIndices[g + 1] = compressedIndices[g];
						compressedIndices[g] = temp;
					}
				}
			}

			/*
			std::cout << "indices 2 (" << compressedIndices.size() << "): ";
			for (int x = 0; x < compressedIndices.size(); x++) {
				std::cout << compressedIndices[x] << ", ";
			}
			std::cout << std::endl;
			*/

			// remove duplicates
			for (int j = compressedIndices.size() - 2; j >= 0; j--) {
				if (compressedIndices[j] == compressedIndices[j + 1]) {
					compressedIndices.erase(compressedIndices.begin() + j + 1);
				}
			}

			/*
			std::cout << "indices 3 (" << compressedIndices.size() << "): ";
			for (int x = 0; x < compressedIndices.size(); x++) {
				std::cout << compressedIndices[x] << ", ";
			}
			std::cout << std::endl;
			*/

			// populate vertices
			for (int j = 0; j < compressedIndices.size(); j++) {
				consolidatedVertices.push_back(vertices[compressedIndices[j]]);
			}

			// repair indice list (match compressed to uncompressed)
			vector<unsigned int> repairedIndices = consolidatedIndices[i];
			for (int j = 0; j < consolidatedIndices[i].size(); j++) {
				for (int g = 0; g < compressedIndices.size(); g++) {
					if (compressedIndices[g] == repairedIndices[j]) {
						repairedIndices[j] = g;
					}
				}
			}

			/*
			std::cout << "vertices(" << consolidatedVertices.size() << "): ";
			for (int x = 0; x < consolidatedVertices.size(); x++) {
				std::cout << consolidatedVertices[x].Position.x << "," << consolidatedVertices[x].Position.y << "," << consolidatedVertices[x].Position.z << " ";
			}
			std::cout << std::endl;

			std::cout << "indices condensed(" << consolidatedIndices[i].size() << "): ";
			for (int x = 0; x < consolidatedIndices[i].size(); x++) {
				std::cout << consolidatedIndices[i][x] << ", ";
			}
			std::cout << std::endl;

			std::cout << "indices(" << repairedIndices.size() << "): ";
			for (int x = 0; x < repairedIndices.size(); x++) {
				std::cout << repairedIndices[x] << ", ";
			}
			std::cout << std::endl;
			std::cout << std::endl;
			*/

			output.push_back(Mesh(f, consolidatedVertices, repairedIndices, textures, materials, samples));
		}

		std::cout << "finished packing " << output.size() << " faces" << std::endl;

		return output;
	}

	//unpacks the textures from assimp into the texture struct so it's more manipulatable
	vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName) {
		vector<Texture> textures;
		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);
			bool skip = false;
			for (unsigned int j = 0; j < textures_loaded.size(); j++)
			{
				if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
				{
					textures.push_back(textures_loaded[j]);
					skip = true;
					break;
				}
			}
			if (!skip)
			{   // if texture hasn't been loaded already, load it
				Texture texture;
				texture.id = TextureFromFile(f, str.C_Str(), directory, samples);
				texture.type = typeName;
				texture.path = str.C_Str();
				textures.push_back(texture);
				textures_loaded.push_back(texture); // add to loaded textures
			}
		}
		return textures;
	}

	// utility
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

	glm::vec4 getPlane(vector<Vertex> vert) {
		float a1 = vert[1].Position.x - vert[0].Position.x;
		float b1 = vert[1].Position.y - vert[0].Position.y;
		float c1 = vert[1].Position.z - vert[0].Position.z;
		float a2 = vert[2].Position.x - vert[0].Position.x;
		float b2 = vert[2].Position.y - vert[0].Position.y;
		float c2 = vert[2].Position.z - vert[0].Position.z;
		float a = b1 * c2 - b2 * c1;
		float b = a2 * c1 - a1 * c2;
		float c = a1 * b2 - b1 * a2;
		float d = (-a * vert[0].Position.x - b * vert[0].Position.y - c * vert[0].Position.z);

		return glm::vec4(a, b, c, d);
	}

	bool tangantFace(vector<Vertex> vertices1, vector<Vertex> vertices2) {
		glm::vec4 plane1 = getPlane(vertices1);
		glm::vec4 plane2 = getPlane(vertices2);

		plane1 = glm::vec4(glm::normalize(plane1));
		plane2 = glm::vec4(glm::normalize(plane2));

		//std::cout << glm::distance(plane1, plane2) << std::endl;
		if (glm::distance(plane1, plane2) <= 0.0001f) {
			//std::cout << "here" << std::endl;
			return true;
		}

		return false;
	}
};

//method from stb_image.h
unsigned int TextureFromFile(QOpenGLFunctions_3_3_Core **f, const char *path, const string &directory, int samples, bool gamma)
{
	string filename = string(path);
	filename = directory + "\\" + filename;
	cout << filename << endl;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		//multisampling (samples)x
		if (samples > 1) {
			(*f)->glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureID);
			(*f)->glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_TRUE);
			(*f)->glGenerateMipmap(GL_TEXTURE_2D_MULTISAMPLE);

			(*f)->glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, GL_REPEAT);
			(*f)->glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, GL_REPEAT);
			(*f)->glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			(*f)->glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			(*f)->glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

			(*f)->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureID, 0);
		}
		else {
			(*f)->glBindTexture(GL_TEXTURE_2D, textureID);
			(*f)->glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			(*f)->glGenerateMipmap(GL_TEXTURE_2D);

			(*f)->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			(*f)->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			(*f)->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			(*f)->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
};

#endif
