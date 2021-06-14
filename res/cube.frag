#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( set = 0, binding = 1 ) uniform sampler2D tex;

layout (location = 0) out vec4 uFragColor;

layout ( location = 0 ) in struct fragment_in {
    vec2 uv;
} IN;

void main() {
	vec4 cbColor = vec4(0.0, 0.0, 0.0, 1.0);

	cbColor = texture(tex, IN.uv);

	uFragColor = cbColor;
}
