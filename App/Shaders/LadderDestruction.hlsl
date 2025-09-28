// Shaders/LadderDestruction.hlsl
cbuffer DestructionCB : register(b0)
{
    float4 gTime;
    float2 gCenter;
    float2 _pad;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0; 
};


float4 PS(PSInput input) : SV_Target
{

    float2 diff = (input.uv - gCenter);
    float dist = length(diff);

    float t = gTime.x;
    float ringCenter = t * 0.5;
    float ringWidth = lerp(0.10, 0.02, saturate(t)); 
    float ring = 1.0 - smoothstep(ringCenter - ringWidth, ringCenter + ringWidth, dist);

    float wave = 0.5 + 0.5 * sin(dist * 40.0 - t * 12.0);

    float flash = (1.0 - smoothstep(0.0, 0.12, dist)) * saturate(1.0 - t);

    float alpha = saturate(ring * wave) * saturate(1.0 - t * 0.6) + flash * 0.6;

    float3 glowColor = float3(1.0, 0.8, 0.3);

    return float4(glowColor, alpha) * input.color;
}
