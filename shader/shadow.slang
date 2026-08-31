#language slang 2026

struct VertexInput {
    float3 position;
};

struct UniformBuffer {
    float4x4 view;
    float4x4 proj;
    float4x4 depth_view_projection;
    float4 view_position;
    float4 light_position;
    float4 mode;
};

struct PushConstants {
    float4x4 model;
    float4x4 normal;
};

[[vk::binding(0, 0)]] ConstantBuffer<UniformBuffer> ubo;

[[vk::push_constant]] PushConstants push_constants;

[shader("vertex")]
float4 vertex_main(VertexInput input): SV_Position {
    return mul(ubo.depth_view_projection, mul(push_constants.model, float4(input.position, 1.0)));
}
