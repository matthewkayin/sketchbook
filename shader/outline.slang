#language slang 2026

struct VertexInput {
    float3 position;
    float3 normal;
    float2 tex_coord;
};

struct UniformBuffer {
    float4x4 view;
    float4x4 proj;
    float4 view_position;
    float4 mode;
};

struct LightData {
    float4 light_position;
    float4 light_color;
};

struct PushConstants {
    float4x4 model;
    float4x4 normal;
};

[[vk::binding(0, 0)]] StructuredBuffer<LightData> light_data;
[[vk::binding(1, 0)]] ConstantBuffer<UniformBuffer> ubo;
[[vk::binding(2, 0)]] Sampler2D hatch_texture0;
[[vk::binding(3, 0)]] Sampler2D hatch_texture1;
[[vk::binding(4, 0)]] Sampler2D hatch_texture2;
[[vk::binding(5, 0)]] Sampler2D hatch_texture3;

[[vk::binding(0, 1)]] Sampler2D color_texture;

[[vk::push_constant]] PushConstants push_constants;

struct VertexOutput {
    float4 sv_position: SV_Position;
};

[shader("vertex")]
VertexOutput vertex_main(VertexInput input) {
    VertexOutput output;

    float4 world_position = mul(push_constants.model, float4(input.position, 1.0));
    float3 world_normal = normalize(mul(push_constants.model, float4(input.normal, 0.0)).xyz);
    float thickness = 0.002;
    float4 extruded_position = float4(world_position.xyz + (world_normal * thickness), 1.0);

    output.sv_position = mul(ubo.proj, mul(ubo.view, extruded_position));

    return output;
}

[shader("fragment")]
float4 fragment_main(VertexOutput vertex_output): SV_Target {
    return float4(0.0, 0.0, 0.0, 1.0);
}
