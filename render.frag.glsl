#version 330 core
in float vInstanceID;
in vec2 vLocalPos;   // local position within the quad (-1 to 1)

out vec4 FragColor;

void main() {
    // discard fragments outside the circle
    float dist = length(vLocalPos);
    if (dist > 1.0) discard;

    // simple shading - brighter in center, darker at edge
    float id = vInstanceID;
float r = fract(sin(id * 127.1) * 43758.5); //we will use Inigo Quilez random numbers
float g = fract(sin(id * 269.5) * 23421.6);
float b = fract(sin(id * 419.2) * 67890.3);

FragColor = vec4(r, g, b, 1.0);
}
