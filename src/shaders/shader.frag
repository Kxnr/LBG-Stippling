#version 410 core
layout(location = 0) in vec3 VertColor;

layout(location = 0) out vec4 fragColor;

void main()
{
  fragColor = vec4(VertColor, 1);
}
