#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 tangent;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform bool hasHeightTex = false;
uniform sampler2D texture_height1;

float height_multiplier = 0.1f;

void main()
{
    TexCoords = aTexCoords;    

    Normal = transpose(inverse(mat3(model))) * aNormal;

    vec4 heightTexture = vec4(0.0);
    if (hasHeightTex){
        heightTexture = texture(texture_height1, aTexCoords);
    }

    vec3 position = aPos + Normal * (height_multiplier * (heightTexture.x + heightTexture.y + heightTexture.z) / 3.0);
    gl_Position = projection * view * model * vec4(position, 1.0);

    FragPos = vec3(model * vec4(position, 1.0));
}