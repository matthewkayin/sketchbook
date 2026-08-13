struct VertexInput {
    float3 position;
    float3 normal;
    float2 tex_coord;
};

struct UniformBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};
ConstantBuffer<UniformBuffer> ubo;

struct VertexOutput {
    float4 sv_position: SV_Position;
    float2 tex_coord;
};

[shader("vertex")]
VertexOutput vertex_main(VertexInput input) {
    VertexOutput output;
    output.sv_position = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.position, 1.0))));
    output.tex_coord = input.tex_coord;
    return output;
}

// Sampler2D texture;

[shader("fragment")]
float4 fragment_main(VertexOutput vertex_output): SV_Target {
    return float4(1.0, 1.0, 1.0, 1.0);
    // return texture.Sample(vertex_output.tex_coord);
}
