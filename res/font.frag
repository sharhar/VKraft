#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( set = 0, binding = 0 ) uniform sampler2D tex;

layout ( location = 0 ) out vec4 uFragColor0;
layout ( location = 1 ) out vec4 uFragColor1;

layout ( location = 0 ) in struct fragment_in {
    vec2 uv;
} IN;

void main() {
	vec4 ftColor = texture(tex, IN.uv);

	if(ftColor.a < 0.5) {
		discard;
	}

	uFragColor0 = vec4(ftColor.rgb, 1.0);
	uFragColor1 = vec4(ftColor.rgb, 1.0);
}
