struct Vertex {
    float3 position : POSITION;
    float3 color : COLOR;
};

RWStructuredBuffer<Vertex> vertices : register(u0);

[numthreads(1, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID) {
    float time = DTid.x * 0.1;
    float3 positions[8] = {
        float3(-1, -1, -1), float3(1, -1, -1), float3(1, 1, -1), float3(-1, 1, -1),
        float3(-1, -1, 1), float3(1, -1, 1), float3(1, 1, 1), float3(-1, 1, 1)
    };
    
    // Apply a simple pulsating animation
    float scale = 1.0 + 0.2 * sin(time);
    vertices[DTid.x].position = positions[DTid.x] * scale;
    vertices[DTid.x].color = normalize(positions[DTid.x]) * 0.5 + 0.5;
}