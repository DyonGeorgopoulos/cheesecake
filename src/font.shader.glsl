@module font
@vs vs
layout(location=0) in vec4 coord;
layout(location=1) in vec4 color;
layout(location=0) out vec2 uv;
layout(location=1) out vec4 vert_color;
void main() {
    gl_Position = vec4(coord.xy, 0.0, 1.0);
    uv = coord.zw;
    vert_color = color;
}
@end

@fs fs
layout(location=0) in vec2 uv;
layout(location=1) in vec4 vert_color;
layout(binding=0) uniform texture2D tex;
layout(binding=0) uniform sampler smp;
layout(location=0) out vec4 frag_color;
void main() {
    float alpha = texture(sampler2D(tex, smp), uv).a;
    frag_color = vec4(vert_color.rgb, vert_color.a * alpha);
}
@end

@program program vs fs