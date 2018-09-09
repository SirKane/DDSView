#ifndef FORMATINFO_H
#define FORMATINFO_H

#include <dxgiformat.h>
#include <stdint.h>

enum class eTextureElementType{
	Pixel,
	PixelPair,
	Block,
	Invalid
};

enum class eFormatType{
	UNorm = 0,
	SNorm,
	Float,
	SInt,
	UInt,
	PerChannel,
	XRBias,
	Void,
	Typeless,
	SharedExp,
	Invalid,
};

enum class eChannelType{
	R = 0, //Red
	G, //Green
	B, //Blue
	A, //Alpha
	G2, //Green 2
	X, //X
	S, //Stencil
	D, //Depth
	C1, //Compressed one component
	C2, //Compressed two component
	C3, //Compressed three component
	C4, //Compressed four component
	E, //Exponent
	N, //None
};


enum eColorChannelFlag : uint32_t{
	CCF_R = 1,
	CCF_G = 1 << 1,
	CCF_B = 1 << 2,
	CCF_A = 1 << 3,
};


size_t Format_GetBitsPerElement(DXGI_FORMAT format);
DXGI_FORMAT Format_GetSRGBFormat(DXGI_FORMAT format);
DXGI_FORMAT Format_GetLinearFormat(DXGI_FORMAT format);
eTextureElementType Format_GetElementType(DXGI_FORMAT format);
eFormatType Format_GetFormatType(DXGI_FORMAT format);

size_t Format_ComputeSubresourceSize(DXGI_FORMAT format, uint32_t width,
	uint32_t height, uint32_t depth = 1);
size_t Format_ComputeImageDataSize(DXGI_FORMAT format, uint32_t width,
	uint32_t height, uint32_t mipLevels, uint32_t depth = 1, uint32_t arraySize = 1);

size_t Format_ComputeRowStride(DXGI_FORMAT format, uint32_t width,
	uint32_t height);
size_t Format_ComputeDepthStride(DXGI_FORMAT format, uint32_t width,
	uint32_t height);


size_t Format_ComputeElementCount(DXGI_FORMAT format, uint32_t width,
	uint32_t height, uint32_t mipLevels, uint32_t depth = 1, uint32_t arraySize = 1);

bool Format_ShrinkMipLevel(uint32_t &width, uint32_t &height, uint32_t &depth);
void Format_ShrinkMipLevelClamp(uint32_t &width, uint32_t &height, uint32_t &depth);

bool Format_GetElementDimensions(DXGI_FORMAT format, uint32_t width, uint32_t height,
	uint32_t &widthInElements, uint32_t &heightInElements);


uint32_t Format_GetSupportedChannel(DXGI_FORMAT format);

#endif //FORMATINFO_H
