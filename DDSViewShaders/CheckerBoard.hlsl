struct VSOutput{
	float4	Position : SV_POSITION;
	float2	ScreenPos : TEXCOORD0;
};

cbuffer ShaderParams : register(b0){
	float2	ScreenSize;
	float	PatternSize;
	float2	Offset;
	float2	TextureSize;
	float3	ColorA;
	float3	ColorB;
	float3	FillColor;
};


float4 PS_Main(VSOutput input) : SV_TARGET{
	float4 returnValue;
	
	int2 pattern = int2(input.ScreenPos / PatternSize);
	
	float boundsLerp =
		max(
		(input.ScreenPos.x >= TextureSize.x ? 1.0f : 0.0f),
			(input.ScreenPos.y >= TextureSize.y ? 1.0f : 0.0f));

	float s = float((pattern.x ^ pattern.y) & 1);
	float3 color = lerp(ColorA, ColorB, s);
	color = lerp(color, FillColor, boundsLerp);
	returnValue = float4(color, 1.0f);

	return	returnValue;
}



struct VSInput{
	float2	Position : POSITION0;
	float2	TexCoord : TEXCOORD0;
};

VSOutput VS_Main(VSInput input){
	VSOutput output;
	output.ScreenPos = input.TexCoord * ScreenSize - Offset;
	float2 pos = input.Position * 2.0f;
	output.Position = float4(-1.0f + pos.x, 1.0f - pos.y, 0.5f, 1.0f);
	return output;
}