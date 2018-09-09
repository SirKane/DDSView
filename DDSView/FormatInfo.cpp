#include "FormatInfo.h"
#include "FormatInfo.inl"


bool Format_ShrinkMipLevel(uint32_t &width, uint32_t &height, uint32_t &depth){

	width >>= 1;
	height >>= 1;
	depth >>= 1;
	if (width == 0 && height == 0 && depth == 0){
		return false;
	}
	if (width == 0){
		width = 1;
	}
	if (height == 0){
		height = 1;
	}
	if (depth == 0){
		depth = 1;
	}
	return true;
}



void Format_ShrinkMipLevelClamp(uint32_t &width, uint32_t &height, uint32_t &depth){

	width >>= 1;
	height >>= 1;
	depth >>= 1;
	if (width == 0){
		width = 1;
	}
	if (height == 0){
		height = 1;
	}
	if (depth == 0){
		depth = 1;
	}
}

bool Format_GetElementDimensions(DXGI_FORMAT format, uint32_t width, uint32_t height,
	uint32_t &widthInElements, uint32_t &heightInElements){
	eTextureElementType elementType = Format_GetElementType(format);
	switch (elementType){
	case eTextureElementType::Block:{
		widthInElements = (width + 3) / 4;
		heightInElements = (height + 3) / 4;
		return true;
	}
	default:
	case eTextureElementType::Invalid:{
		return false;
	}
	case eTextureElementType::Pixel:{
		widthInElements = width;
		heightInElements = height;
		return true; 
	}
	case eTextureElementType::PixelPair:{
		heightInElements = (width + 1) / 2;
		heightInElements = height;
		return true;
	}

	}
}

size_t Format_ComputeSubresourceSize(DXGI_FORMAT format, uint32_t width,
	uint32_t height, uint32_t depth /* = 1*/){


	uint32_t elementWidth;
	uint32_t elementHeight;

	if (!Format_GetElementDimensions(format, width, height, elementWidth, elementHeight)){
		return 0;
	}

	const size_t rowStride = (elementWidth * Format_GetBitsPerElement(format) + 7) / 8;

	return rowStride * elementHeight * depth;
}


size_t Format_ComputeImageDataSize(DXGI_FORMAT format, uint32_t width,
	uint32_t height, uint32_t mipLevels, uint32_t depth /*= 1*/, uint32_t arraySize /*= 1*/){

	size_t imageDataSize = 0;


	uint32_t elementWidth;
	uint32_t elementHeight;

	/*if (!Format_GetElementDimensions(format, elementWidth, elementHeight)){
		return 0;
	}*/
	uint32_t i;
	for (i = 0; i < mipLevels; ++i){

		Format_GetElementDimensions(format, width, height, elementWidth, elementHeight);

		const size_t rowStride = (elementWidth * Format_GetBitsPerElement(format) + 7) / 8;

		imageDataSize += rowStride * elementHeight * depth;

		Format_ShrinkMipLevelClamp(width, height, depth);
	}

	return imageDataSize * arraySize;
}

size_t Format_ComputeElementCount(DXGI_FORMAT format, uint32_t width,
	uint32_t height, uint32_t mipLevels, uint32_t depth /*= 1*/, uint32_t arraySize /*= 1*/){
	size_t elementCount = 0;


	uint32_t elementWidth;
	uint32_t elementHeight;

	uint32_t i;
	for (i = 0; i < mipLevels; ++i){

		if (!Format_GetElementDimensions(format, width, height, elementWidth, elementHeight)){
			return 0;
		}

		const size_t rowStride = (elementWidth * Format_GetBitsPerElement(format) + 
			Format_GetBitsPerElement(format)-1) / Format_GetBitsPerElement(format);

		elementCount += rowStride * elementHeight * depth;

		Format_ShrinkMipLevelClamp(width, height, depth);
	}

	return elementCount * arraySize;
}


size_t Format_ComputeRowStride(DXGI_FORMAT format, uint32_t width,
	uint32_t height){
	uint32_t elementWidth;
	uint32_t elementHeight;

	if (!Format_GetElementDimensions(format, width, height, 
		elementWidth, elementHeight)){
		return 0;
	}
	return (elementWidth * Format_GetBitsPerElement(format) + 7) / 8;

}
size_t Format_ComputeDepthStride(DXGI_FORMAT format, uint32_t width,
	uint32_t height){
	uint32_t elementWidth;
	uint32_t elementHeight;

	if (!Format_GetElementDimensions(format, width, height, 
		elementWidth, elementHeight)){
		return 0;
	}
	return (elementWidth * Format_GetBitsPerElement(format) + 7) / 8 * elementHeight;
}