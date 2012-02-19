#version 330

struct PointLight {
   vec3  position; //in eye space !!
   vec3  color;
   float attenuation0;
   float attenuation1;
   float attenuation2;
   float angle;
};

struct DirectionalLight {
   vec3 direction;   //in eye space !!
   vec3 color;
};


struct MaterialProperties {
   vec3  ambientColor;
   vec3  diffuseColor;
   vec3  specularColor;
   float specularExponent;
};


uniform PointLight light1;
uniform MaterialProperties material;

in vec4 vVaryingColor;
smooth in vec3  positionInEyeSpace3;
smooth in vec3  normalInEyeSpace;

out vec4 vFragColor;

void main() {
	vFragColor.rgb = material.ambientColor;
    vFragColor.a = 1.0;

	vec3  toLight = light1.position - positionInEyeSpace3;
	float r = length(toLight);
	float intensity = 1.0/(light1.attenuation0+light1.attenuation1*r+light1.attenuation2*r*r);

	vec3 lightDirection = normalize(toLight);
	float diffuse=max(0.0, dot(normalInEyeSpace, lightDirection));

	vFragColor.rgb += intensity*diffuse*material.diffuseColor*light1.color;

	if(diffuse>0.0) {
	   vec3 halfvector = normalize(lightDirection  -normalize(positionInEyeSpace3));
	   float specular = max(0.0,dot(halfvector, normalInEyeSpace));
	   float fSpecular = pow(specular, material.specularExponent);
	   vFragColor.rgb += intensity*fSpecular*light1.color*material.specularColor;
	}
}