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

layout( location = 2 ) in struct fragment_in_vis {
    float visibility;
} VIS;

void main() {
	vec4 cbColor = vec4(0.0, 0.0, 0.0, 1.0);

	if(SEL.selected == 1) {
		cbColor = texture(tex, IN.uv)*1.35;
	} else {
		cbColor = texture(tex, IN.uv);
	}

	cbColor = mix(vec4(0.25, 0.45, 1.0, 1.0), cbColor, VIS.visibility);

	uFragColor = cbColor;
}