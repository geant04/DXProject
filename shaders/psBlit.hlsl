Texture2D inputTexture : register(t0);
SamplerState texSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

float4 PSMain(PSInput input) : SV_TARGET
{
    return inputTexture.Sample(texSampler, input.texcoord);
}