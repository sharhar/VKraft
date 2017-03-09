#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( set = 0, binding = 1 ) uniform sampler2D tex;

layout (location = 0) out vec4 uFragColor;

layout ( location = 0 ) in struct fragment_in {
    vec2 uv;
} IN;

layout ( location = 1 ) in flat struct fragment_in_sel {
	int selected;
} SEL;

void main() {
	if(SEL.selected == 1) {
		uFragColor = texture(tex, IN.uv)*1.35;
	} else {
		uFragColor = texture(tex, IN.uv);
	}
}