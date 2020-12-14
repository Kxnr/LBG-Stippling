#version 410 core
layout(location = 0) in vec3 VertPosition;
layout(location = 1) in vec2 ConePosition;
layout(location = 2) in vec3 ConeColor;

layout(location = 0) out vec3 VertColor;

void main()
{
  VertColor = ConeColor;
  gl_Position = vec4(VertPosition.x + 2.0f*ConePosition.x - 1.0f, VertPosition.y + 2.0f*ConePosition.y - 1.0f, VertPosition.z, 1.0f);
}

//# const float height = 1.99f;
//# const mat4 projection = mat4(2.0f, 0.0f, 0.0f, 0.0f,
//#                              0.0f, -2.0f, 0.0f, 0.0f,
//#                              0.0f, 0.0f, -1.0f, 0.0f,
//#                              -1.0f, 1.0f, 0.0f, 1.0f);
//# void main()
//# {
//# 	VertColor = ConeColor;
//# 	gl_Position = projection * vec4(VertPosition.xy + ConePosition, VertPosition.z + (1.0 - height), 1.0f);
//# }
