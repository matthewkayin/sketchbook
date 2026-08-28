#language slang 2026

struct VertexInput {
    float3 position;
    float3 normal;
    float2 tex_coord;
};

struct UniformBuffer {
    float4x4 view;
    float4x4 proj;
    float4x4 depth_view_projection;
    float4 view_position;
    float4 light_position;
};

struct PushConstants {
    float4x4 model;
    float4x4 normal;
};

[[vk::binding(0, 0)]] ConstantBuffer<UniformBuffer> ubo;

[[vk::binding(0, 1)]] Sampler2D hatch_texture0;
[[vk::binding(1, 1)]] Sampler2D hatch_texture1;
[[vk::binding(2, 1)]] Sampler2D shadow_map;

[[vk::binding(0, 2)]] Sampler2D color_texture;

[[vk::push_constant]] PushConstants push_constants;

struct VertexOutput {
    float4 sv_position: SV_Position;
    float4 depth_position;
    float3 world_position;
    float3 normal;
    float2 tex_coord;
};

static const float4x4 biasMat = float4x4(
    0.5, 0.0, 0.0, 0.5,
    0.0, 0.5, 0.0, 0.5,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0);

[shader("vertex")]
VertexOutput vertex_main(VertexInput input) {
    VertexOutput output;

    output.world_position = mul(push_constants.model, float4(input.position, 1.0)).xyz;
    output.depth_position = mul(biasMat, mul(ubo.depth_view_projection, mul(push_constants.model, float4(input.position, 1.0))));
    output.normal = mul(float3x3(push_constants.normal), input.normal);
    output.sv_position = mul(ubo.proj, mul(ubo.view, float4(output.world_position, 1.0)));
    output.tex_coord = input.tex_coord;

    return output;
}

[shader("fragment")]
float4 fragment_main(VertexOutput vertex_output): SV_Target {
    // Ambient
    float3 light_color = float3(0.5);
    float3 ambient = 0.3 * light_color;

    // Diffuse
    float3 normal = normalize(vertex_output.normal);
    float3 light_direction = normalize(ubo.light_position.xyz - vertex_output.world_position);
    float diffuse_intensity = max(dot(normal, light_direction), 0.0);
    float3 diffuse = float3(diffuse_intensity) * light_color;

    // Shadow
    float4 shadow_coord = vertex_output.depth_position / vertex_output.depth_position.w;
    float shadow = 1.0;
    if (shadow_coord.z > -1.0 && shadow_coord.z < 1.0) {
        float dist = shadow_map.Sample(shadow_coord.xy).r;
        if (shadow_coord.w > 0.0 && dist < shadow_coord.z) {
            shadow = 0.3;
        }
    }
    // depth_position_ndc.z = (depth_position_ndc.z * 0.5) + 0.5;
    // float closest_depth = shadow_map.Sample(depth_position_ndc.xy).r;
    // float current_depth = depth_position_ndc.z;
    // float shadow = current_depth > closest_depth ? 1.0 : 0.0;

    // Intensity
    float3 object_color = color_texture.Sample(vertex_output.tex_coord).xyz;
    float3 lit_color = ambient + (diffuse * shadow);
    float3 lit_object_color = lit_color * object_color;
    float intensity = length(lit_object_color);
    float tone = (1.0 - intensity) * 6.0;

    int level_a = int(floor(tone));
    float blend_weight = frac(tone);

    // Sample
    float3 hatch0 = hatch_texture0.Sample(vertex_output.tex_coord * 16).rgb;
    float3 hatch1 = hatch_texture1.Sample(vertex_output.tex_coord * 16).rgb;

    float hatch_colors[7];
    hatch_colors[0] = 1.0; // No hatch (just white / paper)
    hatch_colors[1] = hatch0.r;
    hatch_colors[2] = hatch0.g;
    hatch_colors[3] = hatch0.b;
    hatch_colors[4] = hatch1.r;
    hatch_colors[5] = hatch1.g;
    hatch_colors[6] = hatch1.b;

    // Hatch shading with blend
    // float hatch_color = lerp(hatch_colors[level_a], hatch_colors[level_a + 1], blend_weight);
    // return float4(hatch_color, hatch_color, hatch_color, 1.0);
    return float4(float3(intensity), 1.0);
}
