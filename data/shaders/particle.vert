uniform vec3 color_from;
uniform vec3 color_to;

#ifdef Explicit_Attrib_Location_Usable
layout(location=0) in vec3 Position;
layout(location = 1) in float lifetime;
layout(location = 2) in float size;

layout(location=3) in vec2 Texcoord;
layout(location = 4) in vec2 quadcorner;
#else
in vec3 Position;
in float lifetime;
in float size;

in vec2 Texcoord;
in vec2 quadcorner;
#endif

out float lf;
out vec2 tc;
out vec4 pc;

void main(void)
{
    tc = Texcoord;
    lf = lifetime;
    pc = vec4(vec3(color_from + (color_to - color_from) * lf), 1.0) * smoothstep(1., 0.8, lf);
#if defined(GL_ES) && !defined(Advanced_Lighting_Enabled)
    pc.rgb = pow(pc.rgb, vec3(1. / 2.2));
#endif
    vec3 newposition = Position;

    vec4 viewpos = ViewMatrix * vec4(newposition, 1.0);
    viewpos += size * vec4(quadcorner, 0., 0.);
    gl_Position = ProjectionMatrix * viewpos;
}
