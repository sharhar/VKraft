#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( set = 0, binding = 0 ) uniform sampler2D tex;

layout (location = 0) out vec4 uFragColor0;
layout (location = 1) out vec4 uFragColor1;

layout ( location = 0 ) in struct fragment_in {
    vec2 uv;
	float brightness;
} IN;

void main() {
	vec4 cbColor = vec4(0.0, 0.0, 0.0, 1.0);

	cbColor = texture(tex, IN.uv);
	
	if (cbColor.a < 0.5) {
		discard;
	}

	uFragColor0 = cbColor * IN.brightness;
	uFragColor1 = uFragColor0;
}
