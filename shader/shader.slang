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
    // Ambient
    float ambient_strength = 0.1;
    float3 ambient = light_data[0].light_color.xyz * ambient_strength;

    // Diffuse
    float3 normal = normalize(vertex_output.normal);
    float3 light_direction = normalize(vertex_output.world_position - light_data[0].light_position.xyz);
    float diffuse_strength = max(dot(normal, light_direction), 0.0);
    float3 diffuse = light_data[0].light_color.xyz * diffuse_strength;

    // Specular
    float3 view_direction = normalize(ubo.view_position.xyz - vertex_output.world_position);
    float3 reflect_direction = reflect(-light_direction, normal);
    float specular_strength = pow(max(dot(view_direction, reflect_direction), 0.0), 32) * 0.5;
    float3 specular = light_data[0].light_color.xyz * specular_strength;

    float3 object_color = float3(1.0, 1.0, 1.0);
    return float4((ambient + diffuse + specular) * object_color, 1.0);
}
