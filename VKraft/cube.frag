#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( set = 0, binding = 1 ) uniform sampler2D tex;

layout ( location = 0 ) out vec4 uFragColor;

layout ( location = 0 ) in vec2 uv;
layout ( location = 1 ) in flat int selected;

void main() {
	if(selected == 1) {
		uFragColor = texture(tex, uv)*1.35;
	} else {
		uFragColor = texture(tex, uv);
	}
}