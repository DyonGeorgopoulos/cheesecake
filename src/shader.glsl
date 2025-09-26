@module sgp
@vs vs
layout(location=0) in vec4 pos;
layout(location=1) in vec4 color;
layout(location=0) out vec4 v_color;

void main() {
    gl_Position = pos;
    v_color = color;
}
@end

@fs fs
layout(location=0) in vec4 v_color;
layout(location=0) out vec4 frag_color;

void main() {
    frag_color = v_color;
}
@end
@program program vs fs