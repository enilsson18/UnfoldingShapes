#ifndef MESH_H
#define MESH_H

//#include <glad/glad.h> // holds all OpenGL type declarations
#include <QtWidgets/qopenglwidget.h>
#include <QtGui/qopenglfunctions_3_3_core.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/normal.hpp>

#include <shader.h>

#include <string>
#include <vector>
using namespace std;

struct Vertex {
	// position
	glm::vec3 Position;
	// normal
	glm::vec3 Normal;
	// texCoords
	glm::vec2 TexCoords;
	// tangent
	glm::vec3 Tangent;
	// bitangent
	glm::vec3 Bitangent;

	bool operator==(const Vertex& v2)
	{
		if (Position == v2.Position &&
			Normal == v2.Normal &&
			TexCoords == v2.TexCoords &&
			Tangent == v2.Tangent &&
			Bitangent == v2.Bitangent) 
		{
			return true;
		}

		return false;
	}
};

struct Texture {
	unsigned int id;
	string type;
	string path;
};

struct Material {
	glm::vec3 diffuse;
	glm::vec3 specular;
	glm::vec3 ambient;

	//shine is the exponent of the specular shaded equation
	float shine;

	//scales the specular strength of lighting
	float specularStrength;

	//how opaque the model is
	float opacity;
};

class Mesh {
public:
	//mesh Data
	vector<Vertex>       vertices;
	vector<unsigned int> indices;
	vector<Texture>      textures;

	vector<Material> materials;

	unsigned int VAO;

	//number of samples for multisampling
	int samples;

	// backup data
	vector<Vertex> backupVertices;

	Mesh(QOpenGLFunctions_3_3_Core **f, vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures, vector<Material> materials, int samples)
	{
		this->f = f;

		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;
		this->materials = materials;
		this->samples = samples;

		this->backupVertices = vertices;

		//set the vertex buffers and its attribute pointers.
		setupMesh();
	}

	//render the mesh
	void Draw(Shader &shader)
	{
		shader.use();

		//default
		shader.setBool("hasDiffuseTex", false);
		shader.setBool("hasSpecularTex", false);
		shader.setBool("hasNormalTex", false);
		shader.setBool("hasHeightTex", false);

		//bind textures
		if (textures.size() != 0) {
			unsigned int diffuseNr = 1;
			unsigned int specularNr = 1;
			unsigned int normalNr = 1;
			unsigned int heightNr = 1;
			for (int i = 0; i < textures.size(); i++)
			{
				(*f)->glActiveTexture(GL_TEXTURE0 + i); //active texture unit before binding
				//retrieve texture number (the N in diffuse_textureN)
				string number;
				string name = textures[i].type;
				if (name == "texture_diffuse") {
					number = std::to_string(diffuseNr++);
					shader.setBool("hasDiffuseTex", true);
				}
				else if (name == "texture_specular") {
					number = std::to_string(specularNr++); //transfer unsigned int to stream
					shader.setBool("hasSpecularTex", true);
				}
				else if (name == "texture_normal") {
					// std::cout << "has normal" << std::endl;
					number = std::to_string(normalNr++); //transfer unsigned int to stream
					shader.setBool("hasNormalTex", true);
				}
				else if (name == "texture_height") {
					//std::cout << "has height" << std::endl;
					number = std::to_string(heightNr++); //transfer unsigned int to stream
					shader.setBool("hasHeightTex", true);
				}

				//set the sampler to the correct texture unit
				shader.setFloat((name + number).c_str(), i);
				//finally bind the texture
				//multisampling
				if (samples > 1) {
					(*f)->glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textures[i].id);
				}
				else {
					(*f)->glBindTexture(GL_TEXTURE_2D, textures[i].id);
				}
			}
		}

		//handle material settings
		if (materials.size() > 0) {
			for (int i = 0; i < materials.size(); i++) {
				shader.setVec3("diffuse_color", materials[i].diffuse);
				shader.setVec3("specular_color", materials[i].specular);
				shader.setVec3("ambient_color", materials[i].ambient);
				shader.setFloat("specular_shine", materials[i].shine);
				shader.setFloat("specular_strength", materials[i].specularStrength);
				shader.setFloat("opacity", materials[i].opacity);
			}
		}
		//default to set all the colors to 1 so they don't change the values
		// (EDIT) I think there is a way for glsl to automatically have default values for these in the glsl shader code now
		else {
			//shader.setVec3("diffuse_color", glm::vec3(1.0f));
			//shader.setVec3("specular_color", glm::vec3(1.0f));
			//shader.setVec3("ambient_color", glm::vec3(1.0f));
		}

		//draw mesh
		render();

		//reset back to default settings
		(*f)->glActiveTexture(GL_TEXTURE0);
	}

	void render() {
		(*f)->glBindVertexArray(VAO);
		(*f)->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		(*f)->glBindVertexArray(0);
	}

	// reset the mesh and also recalculate the normals
	void rebuild() {
		recompileNormals();

		//clearBuffers();

		//setupMesh();
		rebuildMesh();
	}

	glm::vec3 getAvgPos() {
		glm::vec3 total(0.0f);

		for (int i = 0; i < vertices.size(); i++) {
			total += vertices[i].Position;
		}

		total /= vertices.size();

		return total;
	}

	// returns the normal to the shape
	glm::vec3 getNormal() {
		glm::vec3 normalSum = glm::vec3(0);

		for (int i = 0; i < indices.size(); i += 3) {
			glm::vec3 newNormal = glm::triangleNormal(vertices[indices[i]].Position, vertices[indices[i + 1]].Position, vertices[indices[i + 2]].Position);

			normalSum += newNormal;
		}

		return normalSum * (1.0f / (indices.size() / 3));
	}

private:
	QOpenGLFunctions_3_3_Core **f;

	//render data 
	unsigned int VBO, EBO;

	void clearBuffers() {
		// clear data to preserve memory
		(*f)->glDeleteVertexArrays(1, &VAO);
		(*f)->glDeleteBuffers(1, &VBO);
		(*f)->glDeleteBuffers(1, &EBO);
	}

	//initializes all the buffer objects/arrays
	void setupMesh()
	{
		//create buffers/arrays
		(*f)->glGenVertexArrays(1, &VAO);
		(*f)->glGenBuffers(1, &VBO);
		(*f)->glGenBuffers(1, &EBO);

		rebuildMesh();
	}

	void rebuildMesh() {
		(*f)->glBindVertexArray(VAO);
		// load data into vertex buffers
		(*f)->glBindBuffer(GL_ARRAY_BUFFER, VBO);

		//Tip from learnopengl.com
		//A great thing about structs is that their memory layout is sequential for all its items.
		//The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
		//again translates to 3/2 floats which translates to a byte array.
		(*f)->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

		(*f)->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		(*f)->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		//set the vertex attribute pointers
		//vertex Positions
		(*f)->glEnableVertexAttribArray(0);
		(*f)->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		//vertex normals
		(*f)->glEnableVertexAttribArray(1);
		(*f)->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
		//vertex texture coords
		(*f)->glEnableVertexAttribArray(2);
		(*f)->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
		//vertex tangent
		(*f)->glEnableVertexAttribArray(3);
		(*f)->glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
		//vertex bitangent
		(*f)->glEnableVertexAttribArray(4);
		(*f)->glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

		(*f)->glBindVertexArray(0);
	}

	void recompileNormals() {
		glm::vec3 newNormal = getNormal();

		for (int i = 0; i < indices.size(); i+=3) {
			vertices[indices[i]].Normal = newNormal;
			vertices[indices[i+1]].Normal = newNormal;
			vertices[indices[i+2]].Normal = newNormal;
		}
	}

	void printVertices() {
		std::cout << "Vertices: " << std::endl;
		for (int i = 0; i < vertices.size(); i++) {
			std::cout << glm::to_string(vertices[i].Position) << std::endl;
		}

		std::cout << "Indices: ";
		for (int i = 0; i < indices.size(); i++) {
			std::cout << indices[i] << ", ";
		}
	}
};
#endif
