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
ConstantBuffer<UniformBuffer> ubo;

struct LightData {
    float4 light_position;
    float4 light_color;
};
StructuredBuffer<LightData> light_data;

Sampler2D texture0;
Sampler2D texture1;

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
    float3 object_color = float3(1.0, 1.0, 1.0);

    // Ambient
    float ambient_strength = 0.1;
    float3 ambient = light_data[0].light_color.xyz * ambient_strength;

    // Diffuse
    float3 normal = normalize(vertex_output.normal);
    float3 light_direction = normalize(vertex_output.world_position - light_data[0].light_position.xyz);
    float diffuse_intensity = max(dot(normal, light_direction), 0.0);

    // Specular
    float3 view_direction = normalize(ubo.view_position.xyz - vertex_output.world_position);
    float3 reflect_direction = reflect(-light_direction, normal);
    float specular_intensity = pow(max(dot(view_direction, reflect_direction), 0.0), 32) * 0.5;

    // Combine diffuse and specular into intensity
    // float intensity = saturate(diffuse_intensity + specular_intensity);
    float intensity = saturate(diffuse_intensity);
    float tone = (1.0 - intensity) * 6.0;
    int level_a = int(floor(tone));
    float blend_weight = frac(tone);

    // Sample
    float3 hatch0 = texture0.Sample(vertex_output.tex_coord).rgb;
    float3 hatch1 = texture1.Sample(vertex_output.tex_coord).rgb;

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
