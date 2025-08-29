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

Texture2D g_texture0 : register(t0);
SamplerState g_sampler0 : register(s0);

float4 PS(PSInput input) : SV_TARGET
{
    float4 texColor = g_texture0.Sample(g_sampler0, input.uv);
    float3 glow = texColor.rgb;
    float alpha = texColor.a;
    
    const int samples = 8;
    const float intensity = 2.5;
    
    for (int i = 1; i <= samples; i++)
    {
        float weight = 1.0 / (i * 2.0);
        float2 offsetX = float2(i * g_invTextureSize.x * intensity, 0);
        float2 offsetY = float2(0, i * g_invTextureSize.y * intensity);
        
        glow += g_texture0.Sample(g_sampler0, input.uv + offsetX).rgb * weight;
        glow += g_texture0.Sample(g_sampler0, input.uv - offsetX).rgb * weight;
        glow += g_texture0.Sample(g_sampler0, input.uv + offsetY).rgb * weight;
        glow += g_texture0.Sample(g_sampler0, input.uv - offsetY).rgb * weight;
    }
    
    glow /= (samples * 2.0 + 1.0);
    float3 finalColor = texColor.rgb + glow * 0.5;
    
    return float4(finalColor, alpha) * input.color;
}