#version 330 core

out float vInstanceID;
layout(location = 0) in vec2 aQuadPos;  // unit quad vertex (-1 to 1)

uniform sampler2D uState;   // ball state texture
uniform float uRadius;      // ball radius

out vec2 vLocalPos;         // pass to frag shader to draw circle

void main() {
    // gl_InstanceID tells us which ball we are (0 to NUM_BALLS-1)
    vec4 state = texelFetch(uState, ivec2(gl_InstanceID, 0), 0);
    vec2 ballPos = state.xy;

    //this is to send data to render to set unique colors
	vInstanceID = float(gl_InstanceID);
    vLocalPos = aQuadPos; // pass local coords so frag can clip to circle

    // scale the unit quad by radius and offset to ball position
    vec2 worldPos = ballPos + aQuadPos * uRadius;

    gl_Position = vec4(worldPos, 0.0, 1.0);
}
