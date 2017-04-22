uniform mat4 ModelMatrix;

#ifdef Explicit_Attrib_Location_Usable
layout(location = 0) in vec3 Position;
layout(location = 3) in vec2 Texcoord;
layout(location = 4) in vec2 SecondTexcoord;
#else
in vec3 Position;
in vec2 Texcoord;
in vec2 SecondTexcoord;
#endif

out vec2 uv;
out vec2 uv_bis;
out float camdist;

void main() {
	gl_Position = ProjectionViewMatrix * ModelMatrix * vec4(Position, 1.);
	uv = Texcoord;
	uv_bis = SecondTexcoord;
	camdist = length(ViewMatrix * ModelMatrix *  vec4(Position, 1.));
}
