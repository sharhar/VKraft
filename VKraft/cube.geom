#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( points ) in;
layout ( triangle_strip, max_vertices = 36 ) out;

layout ( location = 0 ) in ivec4 rinf[1];

layout ( location = 0 ) out vec2 uv;
layout ( location = 1 ) out int out_selected;

layout( std140, binding = 0 ) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;

	vec3 selected;
} UBO;

void main() {
	mat4 trans = UBO.proj * UBO.view;
	int vids = rinf[0].w;

	int rct = vids & 15;
	int rit = 0;

	int selected = 0;

	if(gl_in[0].gl_Position.x == UBO.selected.x && gl_in[0].gl_Position.y == 0.5-UBO.selected.y && gl_in[0].gl_Position.z == UBO.selected.z) {
		selected = 1;
	}

	if(rct != 0) {
		int sx = rinf[0].x % 16;
		int sy = (rinf[0].x - sx)/16;

		vec2 tex0 = vec2((sx + 0.0)/16.0, (sy + 0.0)/16.0);
		vec2 tex1 = vec2((sx + 0.0)/16.0, (sy + 1.0)/16.0);
		vec2 tex2 = vec2((sx + 1.0)/16.0, (sy + 1.0)/16.0);
		vec2 tex3 = vec2((sx + 1.0)/16.0, (sy + 0.0)/16.0);

		rit = vids & 1;

		if (rit != 0) {
			//Front Face
			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, -0.5, 0.0));
			uv = tex3;
			out_selected = selected;
			EmitVertex();
	
			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, -0.5, 0.0));
			uv = tex1;
			out_selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, -0.5, -0.5, 0.0));
			uv = tex0;
			out_selected = selected;
			EmitVertex();
			EndPrimitive();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, 0.5, -0.5, 0.0));
			uv = tex2;
			out_selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, -0.5, 0.0));
			uv = tex1;
			out_selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, -0.5, 0.0));
			uv = tex3;
			out_selected = selected;
			EmitVertex();
			EndPrimitive();
		}

		rit = vids & 2;

		if (rit != 0) {
			//Back Face
			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, -0.5, 0.5, 0.0));
			uv = tex0;
			out_selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, 0.5, 0.0));
			uv = tex1;
			out_selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, 0.5, 0.0));
			uv = tex3;
			out_selected = selected;
			EmitVertex();
			EndPrimitive();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, 0.5, 0.0));
			uv = tex3;
			out_selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, 0.5, 0.0));
			uv = tex1;
			out_selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, 0.5, 0.5, 0.0));
			uv = tex2;
			out_selected = selected;
			EmitVertex();
			EndPrimitive();
		}

		rit = vids & 4;

		if (rit != 0) {
			//Right Face
			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, 0.5, 0.0));
			uv = tex3;
			out_selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, 0.5, -0.5, 0.0));
			uv = tex1;
			out_selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, -0.5, 0.0));
			uv = tex0;
			out_selected = selected;
			EmitVertex();
			EndPrimitive();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, 0.5, 0.5, 0.0));
			uv = tex2;
			out_selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, 0.5, -0.5, 0.0));
			uv = tex1;
			out_selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, 0.5, 0.0));
			uv = tex3;
			out_selected = selected;
			EmitVertex();
			EndPrimitive();
		}

		rit = vids & 8;

		if (rit != 0) {
			//Left Face
			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, -0.5, -0.5, 0.0));
			uv = tex0;
			out_selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, -0.5, 0.0));
			uv = tex1;
			out_selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, -0.5, 0.5, 0.0));
			uv = tex3;
			out_selected = selected;
			EmitVertex();
			EndPrimitive();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, -0.5, 0.5, 0.0));
			uv = tex3;
			out_selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, -0.5, 0.0));
			uv = tex1;
			out_selected = selected;
			EmitVertex();

			gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, 0.5, 0.0));
			uv = tex2;
			out_selected = selected;
			EmitVertex();
			EndPrimitive();
		}
	}

	rit = vids & 16;

	if (rit != 0) {
	
		int tx = rinf[0].y % 16;
		int ty = (rinf[0].y - tx)/16;

		vec2 tex0t = vec2((tx + 0.0)/16.0, (ty + 0.0)/16.0);
		vec2 tex1t = vec2((tx + 0.0)/16.0, (ty + 1.0)/16.0);
		vec2 tex2t = vec2((tx + 1.0)/16.0, (ty + 1.0)/16.0);
		vec2 tex3t = vec2((tx + 1.0)/16.0, (ty + 0.0)/16.0);
		
		//Top Face
		gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, 0.5, 0.0));
		uv = tex2t;
		out_selected = selected;
		EmitVertex();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, -0.5, -0.5, 0.0));
		uv = tex0t;
		out_selected = selected;
		EmitVertex();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, -0.5, 0.5, 0.0));
		uv = tex1t;
		out_selected = selected;
		EmitVertex();
		EndPrimitive();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, -0.5, 0.0));
		uv = tex3t;
		out_selected = selected;
		EmitVertex();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, -0.5, -0.5, 0.0));
		uv = tex0t;
		out_selected = selected;
		EmitVertex();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, -0.5, 0.5, 0.0));
		uv = tex2t;
		out_selected = selected;
		EmitVertex();
		EndPrimitive();
	}

	rit = vids & 32;

	if (rit != 0) {
		int bx = rinf[0].z % 16;
		int by = (rinf[0].z - bx)/16;

		vec2 tex0b = vec2((bx + 0.0)/16.0, (by + 0.0)/16.0);
		vec2 tex1b = vec2((bx + 0.0)/16.0, (by + 1.0)/16.0);
		vec2 tex2b = vec2((bx + 1.0)/16.0, (by + 1.0)/16.0);
		vec2 tex3b = vec2((bx + 1.0)/16.0, (by + 0.0)/16.0);

		//Bottom Face
		gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, 0.5, 0.0));
		uv = tex1b;
		out_selected = selected;
		EmitVertex();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, -0.5, 0.0));
		uv = tex0b;
		out_selected = selected;
		EmitVertex();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, 0.5, 0.5, 0.0));
		uv = tex2b;
		out_selected = selected;
		EmitVertex();
		EndPrimitive();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, 0.5, 0.5, 0.0));
		uv = tex2b;
		out_selected = selected;
		EmitVertex();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(-0.5, 0.5, -0.5, 0.0));
		uv = tex0b;
		out_selected = selected;
		EmitVertex();

		gl_Position = trans * (gl_in[0].gl_Position + vec4(0.5, 0.5, -0.5, 0.0));
		uv = tex3b;
		out_selected = selected;
		EmitVertex();
		EndPrimitive();
	}
}	