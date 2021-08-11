#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 uFragColor;

layout ( location = 0 ) in struct fragment_in {
    vec2 uv;
} IN;

void main() {
	//vec4 col = texture(tex, IN.uv);

	uFragColor = vec4(1.0, 1.0, 1.0, 1.0);//vec4(1 - col.r, 1 - col.g, 1 - col.b, 1.0);
}