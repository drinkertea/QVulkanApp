#version 450

layout (binding = 0) uniform sampler2DArray samplerArray;

layout (location = 0) in vec3 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
    vec4 tex_color = texture(samplerArray, inUV);
	if (inUV.z > 12.5f)
	{
		tex_color.a =0.5f;
	}
	outFragColor = tex_color;
}