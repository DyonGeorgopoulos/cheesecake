@module font
@vs vs
layout(location=0) in vec2 pos;
layout(location=1) in vec2 tex;
layout(location=2) in vec4 color;

layout(binding=0) uniform vs_params {
    mat4 mvp;
};

layout(location=0) out vec2 uv;
layout(location=1) out vec4 vert_color;

void main() {
    gl_Position = mvp * vec4(pos, 0.0, 1.0);
    uv = tex;
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
    float alpha = texture(sampler2D(tex, smp), uv).r;
    frag_color = vec4(vert_color.rgb, vert_color.a * alpha);
}
@end
@program program vs fs