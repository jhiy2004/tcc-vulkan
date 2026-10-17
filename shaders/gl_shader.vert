#version 330 core

layout (location = 0) in vec2 aXY;
layout (location = 1) in float aZ;

uniform mat4 mvp;
uniform float zScale;

out float vZ;

void main()
{
    vec4 position = vec4(
        aXY.x,
        aXY.y,
        aZ * zScale,
        1.0
    );

    gl_Position = mvp * position;

    // Preserva o z original para a coloração.
    vZ = aZ;
}
