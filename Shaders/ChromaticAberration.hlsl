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

cbuffer ChromaticParams : register(b1)
{
    float intensity;
    float radialStrength;
    float2 padding;
};

Texture2D g_texture0 : register(t0);
SamplerState g_sampler0 : register(s0);

float4 PS(PSInput input) : SV_TARGET
{
    float2 center = float2(0.5, 0.5);
    float2 dir = input.uv - center;
    float dist = length(dir);
    
    float aberration = intensity + dist * radialStrength;
    
    float2 redOffset = dir * aberration * 0.01;
    float2 greenOffset = dir * aberration * 0.005;
    float2 blueOffset = -dir * aberration * 0.01;
    
    float r = g_texture0.Sample(g_sampler0, input.uv + redOffset).r;
    float g = g_texture0.Sample(g_sampler0, input.uv + greenOffset).g;
    float b = g_texture0.Sample(g_sampler0, input.uv + blueOffset).b;
    float a = g_texture0.Sample(g_sampler0, input.uv).a;
    
    return float4(r, g, b, a) * input.color;
}