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
    float3 color : COLOR0;
    float4 position : SV_Position;
};

static float3 colors[3] =
{
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0)
};

[shader("vertex")]
VertexOutput vertMain(VertexInput input,
                      uint vid : SV_VertexID)
{
    VertexOutput output;

    output.position = float4(
        input.xy.x,
        input.xy.y,
        input.z,
        1.0
    );

    output.position = mul(mvp, output.position);

    output.color = colors[vid % 3];

    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}
