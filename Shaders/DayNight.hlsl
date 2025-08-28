Texture2D g_texture0 : register(t0);
SamplerState g_sampler0 : register(s0);

cbuffer PSConstants2D : register(b0)
{
    float4 g_colorAdd;
    float4 g_sdfParam;
    float4 g_sdfOutlineColor;
    float4 g_sdfShadowColor;
    float4 g_internal;
}

cbuffer DayNightParams : register(b1)
{
    float timeOfDay;
    float moonlightIntensity;
    float phaseBlend;
    float starsBonus;
}

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

float4 PS(PSInput input) : SV_TARGET
{
    float4 texColor = g_texture0.Sample(g_sampler0, input.uv);
    float4 finalColor = texColor * input.color + g_colorAdd;
    
    
    float3 dayColor = finalColor.rgb;
    float3 sunsetColor = lerp(dayColor, dayColor * float3(1.2, 0.8, 0.6), 0.6);
    float3 nightColor = dayColor * float3(0.3, 0.3, 0.5) + float3(0.0, 0.0, moonlightIntensity * 0.2);
    float3 dawnColor = lerp(nightColor, dayColor * float3(0.9, 0.8, 1.0), 0.5);
    
    float3 resultColor;
    
    
    if (timeOfDay < 0.5)
    {
        resultColor = dayColor;
    }
    else if (timeOfDay < 0.65)
    {
        float t = (timeOfDay - 0.5) / 0.15;
        resultColor = lerp(dayColor, sunsetColor, t);
    }
    else if (timeOfDay < 0.85)
    {
        float t = (timeOfDay - 0.65) / 0.2;
        resultColor = lerp(sunsetColor, nightColor, min(t * 2.0, 1.0));
        

        float moonPhase = sin(timeOfDay * 6.28318) * 0.5 + 0.5;
        resultColor += float3(0.05, 0.05, 0.1) * moonlightIntensity * moonPhase;
        

        resultColor += float3(0.1, 0.1, 0.15) * starsBonus;
    }
    else
    {
        float t = (timeOfDay - 0.85) / 0.15;
        resultColor = lerp(nightColor, dawnColor, t);
    }
    

    resultColor = lerp(resultColor, dayColor, phaseBlend * 0.1);
    
 
    float2 vignetteCoord = input.uv * 2.0 - 1.0;
    float vignette = 1.0 - dot(vignetteCoord, vignetteCoord) * 0.3;
    
 
    if (timeOfDay >= 0.65 && timeOfDay < 0.85)
    {
        vignette = 1.0 - dot(vignetteCoord, vignetteCoord) * 0.5;
    }
    
    resultColor *= vignette;

    return float4(resultColor, finalColor.a);
}