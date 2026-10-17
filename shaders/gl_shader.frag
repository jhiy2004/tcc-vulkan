
#version 330 core

in float vZ;

out vec4 FragColor;

uniform int renderMode;
uniform float zMin;
uniform float zMax;

float smoothStepValue(float t)
{
    return t * t * (3.0 - 2.0 * t);
}

vec3 heatmap(float t)
{
    const vec3 colors[7] = vec3[7](
        vec3(0.00, 0.00, 0.50),
        vec3(0.00, 0.00, 1.00),
        vec3(0.00, 1.00, 1.00),
        vec3(0.00, 1.00, 0.00),
        vec3(1.00, 1.00, 0.00),
        vec3(1.00, 0.50, 0.00),
        vec3(1.00, 0.00, 0.00)
    );

    t = clamp(t, 0.0, 1.0);

    float x = t * 6.0;
    int i = min(int(x), 5);
    float f = smoothStepValue(fract(x));

    return mix(colors[i], colors[i + 1], f);
}

void main()
{
    if (renderMode == 0)
    {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    }
    else if (renderMode == 1 || renderMode == 2)
    {
        float t = 0.5;

        if (zMax > zMin)
        {
            t = clamp(
                (vZ - zMin) / (zMax - zMin),
                0.0,
                1.0
            );
        }

        if (renderMode == 1)
        {
            vec3 blue = vec3(0.0, 0.0, 1.0);
            vec3 red  = vec3(1.0, 0.0, 0.0);

            FragColor = vec4(mix(blue, red, t), 1.0);
        }
        else
        {
            FragColor = vec4(heatmap(t), 1.0);
        }
    }
    else
    {
        FragColor = vec4(0.5, 0.5, 0.5, 1.0);
    }
}