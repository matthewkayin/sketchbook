#language slang 2026

struct VertexInput {
    float3 position;
    float3 normal;
    float2 tex_coord;
};

struct UniformBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
    float4x4 normal;
    float4 view_position;
};

struct LightData {
    float4 light_position;
    float4 light_color;
};

[[vk::binding(0, 0)]] StructuredBuffer<LightData> light_data;
[[vk::binding(1, 0)]] ConstantBuffer<UniformBuffer> ubo;
[[vk::binding(2, 0)]] Sampler2D hatch_texture0;
[[vk::binding(3, 0)]] Sampler2D hatch_texture1;

[[vk::binding(0, 1)]] Sampler2D color_texture;

struct VertexOutput {
    float4 sv_position: SV_Position;
    float3 world_position;
    float3 normal;
    float2 tex_coord;
};

[shader("vertex")]
VertexOutput vertex_main(VertexInput input) {
    VertexOutput output;

    output.world_position = mul(ubo.model, float4(input.position, 1.0)).xyz;
    output.normal = mul(float3x3(ubo.normal), input.normal);
    output.sv_position = mul(ubo.proj, mul(ubo.view, float4(output.world_position, 1.0)));
    output.tex_coord = input.tex_coord;

    return output;
}

// Sampler2D texture;

[shader("fragment")]
float4 fragment_main(VertexOutput vertex_output): SV_Target {
    // Object color
    float3 object_color = color_texture.Sample(vertex_output.tex_coord).rgb;

    // Diffuse
    float3 normal = normalize(vertex_output.normal);
    float3 light_direction = normalize(vertex_output.world_position - light_data[0].light_position.xyz);
    float diffuse_intensity = max(dot(normal, light_direction), 0.0);
    float3 diffuse = (diffuse_intensity + object_color) / 2.0;

    // Intensity
    float intensity = length(diffuse);
    float tone = (1.0 - intensity) * 6.0;

    int level_a = int(floor(tone));
    float blend_weight = frac(tone);

    // Sample
    float3 hatch0 = hatch_texture0.Sample(vertex_output.tex_coord).rgb;
    float3 hatch1 = hatch_texture1.Sample(vertex_output.tex_coord).rgb;

    float hatch_colors[7];
    hatch_colors[0] = 1.0; // No hatch (just white / paper)
    hatch_colors[1] = hatch0.r;
    hatch_colors[2] = hatch0.g;
    hatch_colors[3] = hatch0.b;
    hatch_colors[4] = hatch1.r;
    hatch_colors[5] = hatch1.g;
    hatch_colors[6] = hatch1.b;

    float hatch_color = lerp(hatch_colors[level_a], hatch_colors[level_a + 1], blend_weight);
    return float4(hatch_color, hatch_color, hatch_color, 1.0);
}
