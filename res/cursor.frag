#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 uFragColor;

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputColor;

layout ( location = 0 ) in struct fragment_in {
    vec2 uv;
} IN;

void main() {
	vec4 col = subpassLoad(inputColor);

	uFragColor = vec4(1 - col.r, 1 - col.g, 1 - col.b, 1.0);
}
