struct VSOutput{
	float4	Position : SV_POSITION;
	float2	TexCoord : TEXCOORD0;
};



#define Component_RGBA 0
#define	Component_RGB 1
#define Component_R 2
#define Component_G 3
#define Component_B 4
#define Component_A 5

#define Swap_None 0
#define Swap_RB 1

#define Type_UNorm 0
#define Type_SNorm 1
#define Type_Float 2
#define Type_SInt 3
#define Type_UInt 4


cbuffer ShaderParams : register(b0){
	float2	ScreenSize;
	float2	TextureSize;
	float2	Position;
	float	RangeStart;
	float	RangeEnd;
	float	UVScale;

};
#ifdef PIXEL_SHADER
SamplerState	Sampler : register(s0);

#if TYPE == Type_UNorm
Texture2D<float4>		Texture : register(t0);
float4 Sample_UNorm(float2 texCoord){
	float4 color = Texture.Sample(Sampler, texCoord);
	return color;
}
#elif TYPE == Type_SNorm
Texture2D<float4>		Texture : register(t0);
float4 Sample_SNorm(float2 texCoord){
	float4 color = Texture.Sample(Sampler, texCoord);
	return (color * 0.5f) + 0.5f;
}
#elif TYPE == Type_Float
Texture2D<float4>		Texture : register(t0);
float4 Sample_Float(float2 texCoord){
	float4 color = Texture.Sample(Sampler, texCoord);

	return (color - RangeStart) / (RangeEnd - RangeStart);
}
#elif TYPE == Type_SInt
Texture2D<int4>		Texture : register(t0);
float4 Sample_SInt(float2 texCoord){
	int4 color = Texture.Load(int3((int2)(texCoord * TextureSize), 0));
	return (float4(color) - RangeStart) / (RangeEnd - RangeStart);
}
#elif TYPE == Type_UInt
Texture2D<uint4>		Texture : register(t0);
float4 Sample_UInt(float2 texCoord){
	uint4 color = Texture.Load(int3((int2)(texCoord * TextureSize), 0));
	return (float4(color)-RangeStart) / (RangeEnd - RangeStart);
}
#endif //Type_SInt


float4 PS_Main(VSOutput input) : SV_TARGET{
	float4 returnValue;
	float4 value;

#if TYPE == Type_UNorm
	value = Sample_UNorm(input.TexCoord);
#elif TYPE == Type_SNorm
	value = Sample_SNorm(input.TexCoord);
#elif TYPE == Type_Float
	value = Sample_Float(input.TexCoord);
#elif TYPE == Type_UInt
	value = Sample_UInt(input.TexCoord);
#elif TYPE == Type_SInt
	value = Sample_SInt(input.TexCoord);
#endif

#if SWAP == Swap_None
	//value = value.rgba;
#elif SWAP == Swap_RB
	value = value.bgra;
#endif //SWAP_RB

	//return float4(0.0f, 1.0f, 0.0f, 1.0f);
#if COMPONENT == Component_RGBA
	return value;
#elif COMPONENT == Component_RGB
	return float4(value.rgb, 1.0f);
#elif COMPONENT == Component_R
	return float4(value.rrr, 1.0f);
#elif COMPONENT == Component_G
	return float4(value.ggg, 1.0f);
#elif COMPONENT == Component_B
	return float4(value.bbb, 1.0f);
#elif COMPONENT == Component_A
	return float4(value.aaa, 1.0f);
#endif
}

#endif //PIXEL_SHADER


struct VSInput{
	float2	Position : POSITION0;
	float2	TexCoord : TEXCOORD0;
};

VSOutput VS_Main(VSInput input){
	VSOutput output;
	output.TexCoord = input.TexCoord * UVScale;
	float2 pos = input.Position / ScreenSize * TextureSize * 2;
	pos += (Position / ScreenSize) * 2;
	output.Position = float4(-1.0f + pos.x, 1.0f - pos.y, 0.5f, 1.0f);
	return output;
}