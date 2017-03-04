#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout ( points ) in;
layout ( triangle_strip, max_vertices = 36 ) out;

layout( location = 0 ) in struct geom_in {
    ivec4 rinf;
}[1] IN;

layout( location = 0 ) out struct geom_out {
    vec2 uv;
} OUT;

layout ( location = 1 ) out struct geom_out_sel {
	int selected;
} SEL;

layout( std140, binding = 0 ) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;

	vec3 selected;
} UBO;

void main() {
	mat4 trans = UBO.proj * UBO.view;
	int vids = IN[0].rinf.w;

	int rct = vids & 15;
	int rit = 0;

	int selected = 0;

	if(gl_in[0].gl_Position.x == UBO.selected.x && gl_in[0].gl_Position.y == 0.5-UBO.selected.y && gl_in[0].gl_Position.z == UBO.selected.z) {
		selected = 1;
	}

	if(rct != 0) {
		int sx = IN[0].rinf.x % 16;
		int sy = (IN[0].rinf.x - sx)/16;

		vec2 tex0 = vec2((sx + 0.0)/16.0, (sy + 0.0)/16.0);
		vec2 tex1 = vec2((sx + 0.0)/16.0, (sy + 1.0)/16.0);
		vec2 tex2 = vec2((sx + 1.0)/16.0, (sy + 1.0)/16.0);
		vec2 tex3 = vec2((sx + 1.0)/16.0, (sy + 0.0)/16.0);

		rit = vids & 1;

		if (rit != 0) {
			//Front Face
			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, -0.5, 0.0));
			OUT.uv = tex3;
			SEL.selected = selected;
			EmitVertex();
	
			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, -0.5, 0.0));
			OUT.uv = tex1;
			SEL.selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, -0.5, -0.5, 0.0));
			OUT.uv = tex0;
			SEL.selected = selected;
			EmitVertex();
			EndPrimitive();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, 0.5, -0.5, 0.0));
			OUT.uv = tex2;
			SEL.selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, -0.5, 0.0));
			OUT.uv = tex1;
			SEL.selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, -0.5, 0.0));
			OUT.uv = tex3;
			SEL.selected = selected;
			EmitVertex();
			EndPrimitive();
		}

		rit = vids & 2;

		if (rit != 0) {
			//Back Face
			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, -0.5, 0.5, 0.0));
			OUT.uv = tex0;
			SEL.selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, 0.5, 0.0));
			OUT.uv = tex1;
			SEL.selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, 0.5, 0.0));
			OUT.uv = tex3;
			SEL.selected = selected;
			EmitVertex();
			EndPrimitive();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, 0.5, 0.0));
			OUT.uv = tex3;
			SEL.selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, 0.5, 0.0));
			OUT.uv = tex1;
			SEL.selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, 0.5, 0.5, 0.0));
			OUT.uv = tex2;
			SEL.selected = selected;
			EmitVertex();
			EndPrimitive();
		}

		rit = vids & 4;

		if (rit != 0) {
			//Right Face
			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, 0.5, 0.0));
			OUT.uv = tex3;
			SEL.selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, 0.5, -0.5, 0.0));
			OUT.uv = tex1;
			SEL.selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, -0.5, 0.0));
			OUT.uv = tex0;
			SEL.selected = selected;
			EmitVertex();
			EndPrimitive();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, 0.5, 0.5, 0.0));
			OUT.uv = tex2;
			SEL.selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, 0.5, -0.5, 0.0));
			OUT.uv = tex1;
			SEL.selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, 0.5, 0.0));
			OUT.uv = tex3;
			SEL.selected = selected;
			EmitVertex();
			EndPrimitive();
		}

		rit = vids & 8;

		if (rit != 0) {
			//Left Face
			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, -0.5, -0.5, 0.0));
			OUT.uv = tex0;
			SEL.selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, -0.5, 0.0));
			OUT.uv = tex1;
			SEL.selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, -0.5, 0.5, 0.0));
			OUT.uv = tex3;
			SEL.selected = selected;
			EmitVertex();
			EndPrimitive();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, -0.5, 0.5, 0.0));
			OUT.uv = tex3;
			SEL.selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, -0.5, 0.0));
			OUT.uv = tex1;
			SEL.selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, 0.5, 0.0));
			OUT.uv = tex2;
			SEL.selected = selected;
			EmitVertex();
			EndPrimitive();
		}
	}

	rit = vids & 16;

	if (rit != 0) {
	
		int tx = IN[0].rinf.y % 16;
		int ty = (IN[0].rinf.y - tx)/16;

		vec2 tex0t = vec2((tx + 0.0)/16.0, (ty + 0.0)/16.0);
		vec2 tex1t = vec2((tx + 0.0)/16.0, (ty + 1.0)/16.0);
		vec2 tex2t = vec2((tx + 1.0)/16.0, (ty + 1.0)/16.0);
		vec2 tex3t = vec2((tx + 1.0)/16.0, (ty + 0.0)/16.0);
		
		//Top Face
		gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, 0.5, 0.0));
		OUT.uv = tex2t;
		SEL.selected = selected;
		EmitVertex();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, -0.5, -0.5, 0.0));
		OUT.uv = tex0t;
		SEL.selected = selected;
		EmitVertex();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, -0.5, 0.5, 0.0));
		OUT.uv = tex1t;
		SEL.selected = selected;
		EmitVertex();
		EndPrimitive();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, -0.5, 0.0));
		OUT.uv = tex3t;
		SEL.selected = selected;
		EmitVertex();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, -0.5, -0.5, 0.0));
		OUT.uv = tex0t;
		SEL.selected = selected;
		EmitVertex();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, 0.5, 0.0));
		OUT.uv = tex2t;
		SEL.selected = selected;
		EmitVertex();
		EndPrimitive();
	}

	rit = vids & 32;

	if (rit != 0) {
		int bx = IN[0].rinf.z % 16;
		int by = (IN[0].rinf.z - bx)/16;

		vec2 tex0b = vec2((bx + 0.0)/16.0, (by + 0.0)/16.0);
		vec2 tex1b = vec2((bx + 0.0)/16.0, (by + 1.0)/16.0);
		vec2 tex2b = vec2((bx + 1.0)/16.0, (by + 1.0)/16.0);
		vec2 tex3b = vec2((bx + 1.0)/16.0, (by + 0.0)/16.0);

		//Bottom Face
		gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, 0.5, 0.0));
		OUT.uv = tex1b;
		SEL.selected = selected;
		EmitVertex();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, -0.5, 0.0));
		OUT.uv = tex0b;
		SEL.selected = selected;
		EmitVertex();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, 0.5, 0.5, 0.0));
		OUT.uv = tex2b;
		SEL.selected = selected;
		EmitVertex();
		EndPrimitive();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, 0.5, 0.5, 0.0));
		OUT.uv = tex2b;
		SEL.selected = selected;
		EmitVertex();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, -0.5, 0.0));
		OUT.uv = tex0b;
		SEL.selected = selected;
		EmitVertex();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, 0.5, -0.5, 0.0));
		OUT.uv = tex3b;
		SEL.selected = selected;
		EmitVertex();
		EndPrimitive();
	}
}	