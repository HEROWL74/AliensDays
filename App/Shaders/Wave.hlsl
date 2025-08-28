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

cbuffer WaveParams : register(b1)
{
    float time;
    float amplitude;
    float frequency;
    float speed;
};

Texture2D g_texture0 : register(t0);
SamplerState g_sampler0 : register(s0);

float4 PS(PSInput input) : SV_TARGET
{
    float2 uv = input.uv;
    
    float wave = sin(uv.y * frequency + time * speed) * amplitude;
    uv.x += wave * g_invTextureSize.x;
    
    float wave2 = cos(uv.x * frequency * 0.7 + time * speed * 1.3) * amplitude * 0.5;
    uv.y += wave2 * g_invTextureSize.y;
    
    float4 texColor = g_texture0.Sample(g_sampler0, uv);
    
    return texColor * input.color;
}