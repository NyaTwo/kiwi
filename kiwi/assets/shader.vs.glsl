#version 330

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_texcoord;
layout (location = 2) in vec4 a_color;

uniform mat4 u_projection;
uniform mat4 u_world;

out vec2 f_texcoord;
out vec4 f_color;

void main() {
   gl_Position = u_projection * u_world * vec4(a_position, 1.0);
   f_texcoord = a_texcoord;
   f_color = a_color;
}