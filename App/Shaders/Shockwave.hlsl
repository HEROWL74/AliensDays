struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

cbuffer PSConstants2D : register(b0)
{
    float4 g_colorMul;
    float4 g_sdfParam;
    float2 g_pixelSize;
    float2 g_invTextureSize;
    float4 g_internal;
};

cbuffer ShockwaveParams : register(b1)
{
    float2 center;
    float radius;
    float thickness;
    float force;
    float3 padding;
};

Texture2D g_texture0 : register(t0);
SamplerState g_sampler0 : register(s0);

float4 PS(PSInput input) : SV_TARGET
{
    float2 dir = input.uv - center;
    float dist = length(dir);
    
    float diff = abs(dist - radius);
    float intensity = 0.0;
    
    if (diff < thickness)
    {
        intensity = 1.0 - (diff / thickness);
        intensity = intensity * intensity;
    }
    
    float2 offset = normalize(dir) * intensity * force;
    float2 distortedUV = input.uv + offset;
    
    float4 texColor = g_texture0.Sample(g_sampler0, distortedUV);
    
    float edge = smoothstep(radius - thickness, radius + thickness, dist);
    float3 shockwaveColor = lerp(texColor.rgb, float3(1.0, 0.8, 0.3), intensity * 0.3);
    
    return float4(shockwaveColor, texColor.a) * input.color;
}