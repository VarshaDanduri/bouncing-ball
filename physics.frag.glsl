#version 330 core

uniform sampler2D uState;    // current ball state texture
uniform int uNumBalls;       // total number of balls
uniform float uDelta;        // delta time
uniform float uRadius;       // ball radius

out vec4 newState;           // output: new state for ball i

void main() {
    // each fragment IS one ball
    // gl_FragCoord.x tells us which ball we are
    int i = int(gl_FragCoord.x);

    // read our current state from the texture
    vec4 self = texelFetch(uState, ivec2(i, 0), 0);
    vec2 pos = self.xy;
    vec2 vel = self.zw;

    // check collision against every other ball
    for (int j = 0; j < uNumBalls; j++) {
        if (j == i) continue;

        vec4 other = texelFetch(uState, ivec2(j, 0), 0);
        vec2 diff = pos - other.xy;
        float dist = length(diff);
        float minDist = uRadius * 2.0; // two radii = collision

        if (dist < minDist && dist > 0.0001) {
            vec2 n = diff / dist;
            vec2 v2 = other.zw;
            float dvDot = dot(vel - v2, n);
            if (dvDot < 0.0) {
                vel -= dvDot * n;
            }
        }
    }

    // wall bounce
    if (pos.x - uRadius < -1.0) { pos.x = -1.0 + uRadius; vel.x =  abs(vel.x); }
    if (pos.x + uRadius >  1.0) { pos.x =  1.0 - uRadius; vel.x = -abs(vel.x); }
    if (pos.y - uRadius < -1.0) { pos.y = -1.0 + uRadius; vel.y =  abs(vel.y); }
    if (pos.y + uRadius >  1.0) { pos.y =  1.0 - uRadius; vel.y = -abs(vel.y); }

    // damping so energy doesnt explode
    vel *= 0.99999;

    // integrate position
    pos += vel * uDelta;

    // write new state
    newState = vec4(pos, vel);
}
