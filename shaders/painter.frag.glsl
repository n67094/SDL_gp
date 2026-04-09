// @copyright Copyright (c) 2025 nsix. All rights reserved.

#version 450 core

layout(location=0) in vec2 tex_uv;
layout(location=1) in vec4 i_color;

layout(location=0) out vec4 frag_color;

layout(set = 2, binding=0) uniform sampler2D uni_tex_sampler;

void main() {
  frag_color = texture(uni_tex_sampler, tex_uv) * i_color;
}

