// @copyright Copyright (c) 2025 nsix. All rights reserved.

#version 450 core

layout(location=0) in vec4 coord;
layout(location=1) in vec4 color;

layout(location=0) out vec2 tex_uv;
layout(location=1) out vec4 i_color;

void main() {
  gl_Position = vec4(coord.xy, 0.0, 1.0);
  gl_PointSize = 1.0;

  tex_uv = coord.zw;
  i_color = color;
}
