[[vk::binding(0, 0)]]
cbuffer UniformBuffer : register(b0)
{
    float4x4 mvp;
};

struct VertexInput
{
    float2 xy : ATTRIBUTE0;
    float  z  : ATTRIBUTE1;
};

struct VertexOutput
{
    float z : TEXCOORD0;
    float4 position : SV_Position;
};

struct SurfacePushConstants{
    uint renderMode;
    float zMin;
    float zMax;
    float zScale;
};

[[vk::push_constant]]
SurfacePushConstants pc;

float3 heatmap(float t) {
    const float3 colors[7] =
    {
        float3(0.00, 0.00, 0.50),
        float3(0.00, 0.00, 1.00),
        float3(0.00, 1.00, 1.00),
        float3(0.00, 1.00, 0.00),
        float3(1.00, 1.00, 0.00),
        float3(1.00, 0.50, 0.00),
        float3(1.00, 0.00, 0.00)
    };

    t = saturate(t);

    float x = t * 6.0;
    int i = min((int)x, 5);
    float f = smoothstep(0.0, 1.0, frac(x));

    return lerp(colors[i], colors[i + 1], f);
}

[shader("vertex")]
VertexOutput vertMain(VertexInput input,
                      uint vid : SV_VertexID)
{
    VertexOutput output;

    output.position = float4(
        input.xy.x,
        input.xy.y,
        input.z * pc.zScale,
        1.0
    );

    output.position = mul(mvp, output.position);

    output.z = input.z;

    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target
{
    if (pc.renderMode == 0) {
        return float4(1.0, 1.0, 1.0, 1.0);
    } else if (pc.renderMode == 1) {
        float t = (input.z - pc.zMin) / (pc.zMax - pc.zMin);

        t = saturate(t);

        float3 blue = float3(0.0, 0.0, 1.0);
        float3 red  = float3(1.0, 0.0, 0.0);

        float3 color = lerp(blue, red, t);

        return float4(color, 1.0);
    } else if(pc.renderMode == 2) {
        float t = (input.z - pc.zMin) / (pc.zMax - pc.zMin);

        t = saturate(t);

        return float4(heatmap(t), 1.0);
    }

    return float4(0.5, 0.5, 0.5, 1.0);
}
