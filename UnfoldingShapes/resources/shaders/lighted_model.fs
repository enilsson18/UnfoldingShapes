#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec2 TexCoords;
in vec3 FragPos;

//override color
uniform bool overrideColorEnabled = false;
uniform vec3 overrideColor = vec3(1.0);

//effect info
//color changer
uniform vec3 effectColor = vec3(1.0,1.0,1.0);
uniform float effectColorStrength = 0;

//light info
uniform bool enableLighting = false;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float lightBrightness;
uniform float lightDistance;

//camera data
uniform vec3 viewPos;

//textures
uniform bool hasDiffuseTex = false;
uniform bool hasSpecularTex = false;
uniform bool hasNormalTex = false;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;

//lighting info
uniform vec3 diffuse_color = vec3(1.0,1.0,1.0);
uniform vec3 specular_color = vec3(1.0,1.0,1.0);
uniform vec3 ambient_color = vec3(1.0,1.0,1.0);
uniform float specular_shine = 1.0;
uniform float specular_strength = 1.0;
uniform float opacity = 1.0;

void main()
{   
	//base color
	vec3 objectColor;

	//default depth components for the textures
	float depthComponent = 1.0;

	//texture management
	//diffuse
	if (hasDiffuseTex){
		vec4 tex = texture(texture_diffuse1, TexCoords);
    	objectColor = tex.xyz;
    	depthComponent = tex.w;

    	//objectColor = vec3(1.0,0.0,0.0);
    }
    else {
    	//material colors: diffuse
    	objectColor = diffuse_color;
    }

    vec3 lightingResults = vec3(1.0,1.0,1.0);
    //texture lighting
    if (enableLighting){
    	//specular shading
    	if (hasSpecularTex){

    	}

    	//normal lighting
    	if (hasNormalTex){

    	}
    }

    //non-texture based lighting
    if (enableLighting){
    	float diff = 0;

    	//ambient lighting
		float ambientStrength = 0.3;
    	vec3 ambient = ambientStrength * (lightColor * ambient_color);

    	//diffuse lighting (normals)
		vec3 norm = normalize(Normal);
		vec3 lightDir = normalize(lightPos - FragPos);
		diff = max(dot(norm, lightDir), 0.0);
		vec3 diffuse = diff * lightColor * diffuse_color;

		//specular lighting
		//basically just make the surface brighter if more light reflects more into the viewers eyes
    	vec3 viewDir = normalize(viewPos - FragPos);
    	vec3 reflectDir = reflect(-lightDir, norm);  
    	float spec = pow(max(dot(viewDir, reflectDir), 0.0), specular_shine);
    	vec3 specular = 1 * spec * (lightColor * specular_color); 
    	//(EDIT) removed specular_strength where 1 is because it was recieved invalid many times

    	//combine the lighting output colors
    	lightingResults = (specular + diffuse) + ambient;
    	lightingResults *= lightBrightness;
    }

    //adjust for color effect multiplier
    objectColor = objectColor * (1-effectColorStrength) + (effectColor * effectColorStrength);

    FragColor = vec4(objectColor * lightingResults, depthComponent * opacity);

    //override for color
    if (overrideColorEnabled){
        FragColor = vec4(objectColor, depthComponent * opacity);
    }
}