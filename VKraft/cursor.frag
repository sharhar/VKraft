#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout ( set = 0, binding = 1 ) uniform sampler2D tex;

layout (location = 0) out vec4 uFragColor;

layout ( location = 0 ) in struct fragment_in {
    vec2 uv;
} IN;

void main() {
	vec4 col = texture(tex, IN.uv);

	if(col.a == 0.0) {
		discard;
	}

	uFragColor = vec4(col.rgb, 1.0);
}