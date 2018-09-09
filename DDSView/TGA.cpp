#include "TGA.h"
#include <string.h>

static const size_t g_TGAHeaderSize =
sizeof(uint8_t) + //IDLength @0
sizeof(uint8_t) + //ColorMapType @1
sizeof(uint8_t) + //ImageType @2

sizeof(uint16_t) + //FirstEntryIndex @3
sizeof(uint16_t) + //Length @5
sizeof(uint8_t) + //EntrySize @7

sizeof(uint16_t) + //XOrigin @8
sizeof(uint16_t) + //YOrigin @A
sizeof(uint16_t) + //Width @C
sizeof(uint16_t) + //Height @E
sizeof(uint8_t) + //Bits per pixel @10
sizeof(uint8_t); //ImageDescriptor @11

static inline void PutU8(uint8_t* pDest, uint8_t value){
	pDest[0] = value;
}
static inline void PutU16(uint8_t* pDest, uint16_t value){
	pDest[0] = (value) & 0xFF;
	pDest[1] = (value >> 8) & 0xFF;
}
static inline void PutU24(uint8_t* pDest, uint32_t value){
	pDest[0] = (value) & 0xFF;
	pDest[1] = (value >> 8) & 0xFF;
	pDest[2] = (value >> 16) & 0xFF;
}
static inline void PutU32(uint8_t* pDest, uint32_t value){
	pDest[0] = (value) & 0xFF;
	pDest[1] = (value >> 8) & 0xFF;
	pDest[2] = (value >> 16) & 0xFF;
	pDest[3] = (value >> 24) & 0xFF;
}

static inline uint8_t FetchU8(const uint8_t* pSrc){
	return ((uint8_t)pSrc[0]);
}
static inline uint16_t FetchU16(const uint8_t* pSrc){
	return ((uint16_t)pSrc[0]) |
		((uint16_t)pSrc[1] << 8);
}
static inline uint32_t FetchU24(const uint8_t* pSrc){
	return ((uint32_t)pSrc[0]) |
		((uint32_t)pSrc[1] << 8) |
		((uint32_t)pSrc[2] << 16);
}
static inline uint32_t FetchU32(const uint8_t* pSrc){
	return ((uint32_t)pSrc[0]) |
		((uint32_t)pSrc[1] << 8) |
		((uint32_t)pSrc[2] << 16) |
		((uint32_t)pSrc[3] << 24);
}

static inline uint8_t Expand5Bits(const uint8_t value){
	return (uint8_t)(((uint32_t)(value)) * 539087 / 65536);
}



static bool RLEDecompress(const void* pSrc, size_t sourceLength, CBlob &outputBuffer,
	size_t bitsPerPixel, size_t pixelCount){

	const size_t pixelSize = (bitsPerPixel + 7) / 8;
	outputBuffer.Resize(pixelCount * pixelSize);

	const uint8_t* pCur = (const uint8_t*)pSrc;
	const uint8_t* pEnd = pCur + sourceLength;
	size_t i, j;

	size_t remaining = 0;
	//uint8_t value[4];


	uint8_t* pDest = (uint8_t*)outputBuffer.GetPointer();
	const uint8_t* pDestEnd = pDest + outputBuffer.Size();
	for (i = 0; i < pixelCount;){
		if (pCur + 1 > pEnd){
			return false;
		}
		size_t count = ((*pCur) & 0x7F) + 1;
		bool isRunLength = (*pCur & 0x80) != 0;
		++pCur;

		if (pDest + count * pixelSize > pDestEnd){
			return false;
		}


		i += count;

		if (isRunLength){
			if (pCur + pixelSize > pEnd){
				return false;
			}
			count = (count - 1) * pixelSize;
			memcpy(pDest, pCur, pixelSize);

			uint8_t* pSource = pDest;
			pDest += pixelSize;
			pCur += pixelSize;
			for (j = 0; j < count; ++j){
				*(pDest++) = *(pSource++);
			}
		} else {
			if (pCur + count * pixelSize > pEnd){
				return false;
			}
			memcpy(pDest, pCur, count * pixelSize);
			pDest += count * pixelSize;
			pCur += count * pixelSize;
		}

	}
	return true;
}



bool TGAHeaderDecode(const void* pData, size_t dataSize, STGAHeader &headerOut){
	if (dataSize < g_TGAHeaderSize){
		return false;
	}
	const uint8_t* pSrc = (const uint8_t*)pData;
	headerOut.IDLength = FetchU8(pSrc + 0);
	headerOut.ColorMapType = FetchU8(pSrc + 1);
	headerOut.ImageType = FetchU8(pSrc + 2);

	headerOut.ColorMapSpec.FirstEntryIndex = FetchU16(pSrc + 3);
	headerOut.ColorMapSpec.Length = FetchU16(pSrc + 5);
	headerOut.ColorMapSpec.EntrySize = FetchU8(pSrc + 7);

	headerOut.ImageSpec.XOrigin = FetchU16(pSrc + 8);
	headerOut.ImageSpec.YOrigin = FetchU16(pSrc + 0xA);
	headerOut.ImageSpec.Width = FetchU16(pSrc + 0xC);
	headerOut.ImageSpec.Height = FetchU16(pSrc + 0xE);
	headerOut.ImageSpec.BitsPerPixel = FetchU8(pSrc + 0x10);
	headerOut.ImageSpec.ImageDescriptor = FetchU8(pSrc + 0x11);
	return true;
}

bool TGAHeaderEncode(void* pData, size_t dataSize, const STGAHeader &header){
	if (dataSize < g_TGAHeaderSize){
		return false;
	}
	uint8_t* pDest = (uint8_t*)pData;
	PutU8(pDest + 0, header.IDLength);
	PutU8(pDest + 1, header.ColorMapType);
	PutU8(pDest + 2, header.ImageType);

	PutU16(pDest + 3, header.ColorMapSpec.FirstEntryIndex);
	PutU16(pDest + 5, header.ColorMapSpec.Length);
	PutU8(pDest + 7, header.ColorMapSpec.EntrySize);

	PutU16(pDest + 8, header.ImageSpec.XOrigin);
	PutU16(pDest + 0xA, header.ImageSpec.YOrigin);
	PutU16(pDest + 0xC, header.ImageSpec.Width);
	PutU16(pDest + 0xE, header.ImageSpec.Height);
	PutU8(pDest + 0x10, header.ImageSpec.BitsPerPixel);
	PutU8(pDest + 0x11, header.ImageSpec.ImageDescriptor);
	return true;
}




static inline size_t ComputeOutputIndex(size_t x, size_t y, const STGAHeader &header){

	if ((header.ImageSpec.ImageDescriptor & (1 << 5)) == 0){
		y = header.ImageSpec.Height - y - 1;
	}

	if ((header.ImageSpec.ImageDescriptor & (1 << 4)) != 0){
		x = header.ImageSpec.Width - x - 1;
	}
	return y * header.ImageSpec.Width + x;
}

static bool TGADecodeColorMap(const CBlob &inputBuffer, STGAOutput &output, const STGAHeader &header,
	CBlob &imageDataBuffer, bool isRLE){

	if (header.ColorMapType != 1){
		return false;
	}

	const size_t colorMapEntrySize = (header.ColorMapSpec.EntrySize + 7) / 8;
	const size_t colorMapSize = header.ColorMapSpec.Length * colorMapEntrySize;

	const size_t colorMapOffset = sizeof(STGAHeader) + header.IDLength;
	const size_t imageDataOffset = colorMapOffset + colorMapSize;
	const size_t indexSize = (header.ImageSpec.BitsPerPixel + 7) / 8;


	size_t imageDataEnd = inputBuffer.Size();


	if (isRLE){
		if (!RLEDecompress((uint8_t*)inputBuffer.GetPointer() + imageDataOffset, imageDataEnd - imageDataOffset,
			imageDataBuffer, header.ImageSpec.BitsPerPixel, header.ImageSpec.Width * header.ImageSpec.Height)){
			return false;
		}
	} else {
		imageDataBuffer.Resize(header.ImageSpec.Width * header.ImageSpec.Height * indexSize);
		if (imageDataBuffer.Size() > imageDataEnd - imageDataOffset){
			return false;
		}
		memcpy(imageDataBuffer.GetPointer(), (const uint8_t*)inputBuffer.GetPointer() + imageDataOffset, 
			imageDataBuffer.Size());
	}

	//Probably never hits, since both RLE decompress and copy failure should exit early
	if (imageDataBuffer.Size() < header.ImageSpec.Height * header.ImageSpec.Width * indexSize){
		return false;
	}

	size_t(*GetColorMapIndex)(size_t &offset, size_t size, const uint8_t* pData) = nullptr;



	switch (header.ImageSpec.BitsPerPixel){
	case 8:{
		GetColorMapIndex = [](size_t &offset, size_t size, const uint8_t* pData) -> size_t{
			if (offset + 1 > size){
				return 0;
			}
			const size_t index = pData[offset++];
			return index;
		};
		break;
	}
	case 16:{
		GetColorMapIndex = [](size_t &offset, size_t size, const uint8_t* pData) -> size_t{
			if (offset + 2 > size){
				return 0;
			}
			const size_t index = pData[offset] |
				((size_t)pData[offset + 1] << 8);
			offset += 2;
			return index;
		};
		break;
	}
	case 24:{
		GetColorMapIndex = [](size_t &offset, size_t size, const uint8_t* pData) -> size_t{
			if (offset + 3 > size){
				return 0;
			}
			const size_t index = pData[offset] |
				((size_t)pData[offset + 1] << 8) |
				((size_t)pData[offset + 2] << 16);
			offset += 3;
			return index;
		};
		break;
	}
	case 32:{
		GetColorMapIndex = [](size_t &offset, size_t size, const uint8_t* pData) -> size_t{
			if (offset >= size){
				return 0;
			}
			const size_t index = pData[offset] |
				((size_t)pData[offset + 1] << 8) |
				((size_t)pData[offset + 2] << 16) |
				((size_t)pData[offset + 3] << 24);
			offset += 4;
			return index;
		};
		break;
	}
	default:{
		return false;
	}
	}


	const uint8_t* pColorMap = (const uint8_t*)inputBuffer.GetPointer() + colorMapOffset;
	switch (header.ColorMapSpec.EntrySize){
	case 32:{
		size_t x, y;
		size_t offset = 0;
		const size_t imageSize = imageDataBuffer.Size();
		uint8_t* pOutputImageData = (uint8_t*)output.ImageData.GetPointer();
		const uint8_t* pImageData = (const uint8_t*)imageDataBuffer.GetPointer();
		for (y = 0; y < header.ImageSpec.Height; ++y){
			for (x = 0; x < header.ImageSpec.Width; ++x){
				const size_t index = GetColorMapIndex(offset, imageSize, pImageData);
				if (index < header.ColorMapSpec.FirstEntryIndex ||
					index >= header.ColorMapSpec.FirstEntryIndex + header.ColorMapSpec.Length){
					return false;
				}
				const size_t baseOffset = ComputeOutputIndex(x, y, header) * 4;
				const uint32_t px = FetchU32(pColorMap + index * 4);
				const uint8_t r = (px >> 16) & 0xFF;
				const uint8_t g = (px >> 8) & 0xFF;
				const uint8_t b = (px) & 0xFF;
				const uint8_t a = (px >> 24) & 0xFF;
				pOutputImageData[baseOffset + 0] = r;
				pOutputImageData[baseOffset + 1] = g;
				pOutputImageData[baseOffset + 2] = b;
				pOutputImageData[baseOffset + 3] = a;
			}
		}
		break;
	}
	case 24:{
		size_t x, y;
		size_t offset = 0;
		const size_t imageSize = imageDataBuffer.Size();
		uint8_t* pOutputImageData = (uint8_t*)output.ImageData.GetPointer();
		const uint8_t* pImageData = (const uint8_t*)imageDataBuffer.GetPointer();
		for (y = 0; y < header.ImageSpec.Height; ++y){
			for (x = 0; x < header.ImageSpec.Width; ++x){
				const size_t index = GetColorMapIndex(offset, imageSize, pImageData);
				if (index < header.ColorMapSpec.FirstEntryIndex ||
					index >= header.ColorMapSpec.FirstEntryIndex + header.ColorMapSpec.Length){
					return false;
				}
				const size_t baseOffset = ComputeOutputIndex(x, y, header) * 4;
				const uint32_t px = FetchU24(pColorMap + index * 3);
				const uint8_t r = (px >> 16) & 0xFF;
				const uint8_t g = (px >> 8) & 0xFF;
				const uint8_t b = (px) & 0xFF;
				const uint8_t a = 0xFF;
				pOutputImageData[baseOffset + 0] = r;
				pOutputImageData[baseOffset + 1] = g;
				pOutputImageData[baseOffset + 2] = b;
				pOutputImageData[baseOffset + 3] = a;
			}
		}
		break;
	}
	case 16:{
		size_t x, y;
		size_t offset = 0;
		const size_t imageSize = imageDataBuffer.Size();
		uint8_t* pOutputImageData = (uint8_t*)output.ImageData.GetPointer();
		const uint8_t* pImageData = (const uint8_t*)imageDataBuffer.GetPointer();
		for (y = 0; y < header.ImageSpec.Height; ++y){
			for (x = 0; x < header.ImageSpec.Width; ++x){
				const size_t index = GetColorMapIndex(offset, imageSize, pImageData);
				if (index < header.ColorMapSpec.FirstEntryIndex ||
					index >= header.ColorMapSpec.FirstEntryIndex + header.ColorMapSpec.Length){
					return false;
				}
				const size_t baseOffset = ComputeOutputIndex(x, y, header) * 4;
				const uint32_t px = FetchU16(pColorMap + index * 2);
				const uint8_t r = Expand5Bits((px >> 10) & 0x1F);
				const uint8_t g = Expand5Bits((px >> 5) & 0x1F);
				const uint8_t b = Expand5Bits((px) & 0x1F);
				const uint8_t a = ((px >> 15) & 1) * 0xFF;
				pOutputImageData[baseOffset + 0] = r;
				pOutputImageData[baseOffset + 1] = g;
				pOutputImageData[baseOffset + 2] = b;
				pOutputImageData[baseOffset + 3] = a;
			}
		}
		break;
	}
	case 15:{
		size_t x, y;
		size_t offset = 0;
		const size_t imageSize = imageDataBuffer.Size();
		uint8_t* pOutputImageData = (uint8_t*)output.ImageData.GetPointer();
		const uint8_t* pImageData = (const uint8_t*)imageDataBuffer.GetPointer();
		for (y = 0; y < header.ImageSpec.Height; ++y){
			for (x = 0; x < header.ImageSpec.Width; ++x){
				const size_t index = GetColorMapIndex(offset, imageSize, pImageData);
				if (index < header.ColorMapSpec.FirstEntryIndex ||
					index >= header.ColorMapSpec.FirstEntryIndex + header.ColorMapSpec.Length){
					return false;
				}
				const size_t baseOffset = ComputeOutputIndex(x, y, header) * 4;
				const uint32_t px = FetchU16(pColorMap + index * 2);
				const uint8_t r = Expand5Bits((px >> 10) & 0x1F);
				const uint8_t g = Expand5Bits((px >> 5) & 0x1F);
				const uint8_t b = Expand5Bits((px) & 0x1F);
				const uint8_t a = 0xFF;
				pOutputImageData[baseOffset + 0] = r;
				pOutputImageData[baseOffset + 1] = g;
				pOutputImageData[baseOffset + 2] = b;
				pOutputImageData[baseOffset + 3] = a;
			}
		}
		break;
	}
	}
	return true;

}
static bool TGADecodeGrayScale(const CBlob &inputBuffer, STGAOutput &output, const STGAHeader &header,
	CBlob &imageDataBuffer, bool isRLE){

	const size_t colorMapEntrySize = header.ColorMapType == 1 ?
		(header.ColorMapSpec.EntrySize + 7) / 8 : 0;
	const size_t colorMapSize = header.ColorMapType == 1 ?
		header.ColorMapSpec.Length * colorMapEntrySize : 0;

	const size_t colorMapOffset = sizeof(STGAHeader) + header.IDLength;
	const size_t imageDataOffset = colorMapOffset + colorMapSize;
	const size_t colorSize = (header.ImageSpec.BitsPerPixel + 7) / 8;


	size_t imageDataEnd = inputBuffer.Size();


	if (isRLE){
		if (!RLEDecompress((uint8_t*)inputBuffer.GetPointer() + imageDataOffset, imageDataEnd - imageDataOffset,
			imageDataBuffer, header.ImageSpec.BitsPerPixel, header.ImageSpec.Width * header.ImageSpec.Height)){
			return false;
		}
	} else {
		imageDataBuffer.Resize(header.ImageSpec.Width * header.ImageSpec.Height * colorSize);
		if (imageDataBuffer.Size() > imageDataEnd - imageDataOffset){
			return false;
		}
		memcpy(imageDataBuffer.GetPointer(), (const uint8_t*)inputBuffer.GetPointer() + imageDataOffset
			, imageDataBuffer.Size());
	}

	//Probably never hits, since both RLE decompress and copy failure should exit early
	if (imageDataBuffer.Size() < header.ImageSpec.Height * header.ImageSpec.Width * colorSize){
		return false;
	}


	switch (header.ImageSpec.BitsPerPixel){
	case 8:{
		size_t x, y;
		size_t offset = 0;
		const size_t imageSize = imageDataBuffer.Size();
		uint8_t* pOutputImageData = (uint8_t*)output.ImageData.GetPointer();
		const uint8_t* pImageData = (const uint8_t*)imageDataBuffer.GetPointer();
		for (y = 0; y < header.ImageSpec.Height; ++y){
			for (x = 0; x < header.ImageSpec.Width; ++x){
				const size_t index = y * header.ImageSpec.Width + x;
				const size_t baseOffset = ComputeOutputIndex(x, y, header) * 4;
				const uint32_t px = FetchU8(pImageData + index);
				const uint8_t g = (px) & 0xFF;
				const uint8_t a = 0xFF;
				pOutputImageData[baseOffset + 0] = g;
				pOutputImageData[baseOffset + 1] = g;
				pOutputImageData[baseOffset + 2] = g;
				pOutputImageData[baseOffset + 3] = a;
			}
		}
		break;
	}
	default:{
		return false;
	}
	}
	return true;
}
static bool TGADecodeRGB(const CBlob &inputBuffer, STGAOutput &output, const STGAHeader &header,
	CBlob&imageDataBuffer, bool isRLE){

	const size_t colorMapEntrySize = header.ColorMapType == 1 ?
		(header.ColorMapSpec.EntrySize + 7) / 8 : 0;
	const size_t colorMapSize = header.ColorMapType == 1 ?
		header.ColorMapSpec.Length * colorMapEntrySize : 0;

	const size_t colorMapOffset = sizeof(STGAHeader) + header.IDLength;
	const size_t imageDataOffset = colorMapOffset + colorMapSize;
	const size_t colorSize = (header.ImageSpec.BitsPerPixel + 7) / 8;


	size_t imageDataEnd = inputBuffer.Size();


	if (isRLE){
		if (!RLEDecompress((const uint8_t*)inputBuffer.GetPointer() + imageDataOffset, imageDataEnd - imageDataOffset,
			imageDataBuffer, header.ImageSpec.BitsPerPixel, header.ImageSpec.Width * header.ImageSpec.Height)){
			return false;
		}
	} else {
		imageDataBuffer.Resize(header.ImageSpec.Width * header.ImageSpec.Height * colorSize);
		if (imageDataBuffer.Size() > imageDataEnd - imageDataOffset){
			return false;
		}
		memcpy(imageDataBuffer.GetPointer(), (const uint8_t*)inputBuffer.GetPointer() + imageDataOffset, 
			imageDataBuffer.Size());
	}

	//Probably never hits, since both RLE decompress and copy failure should exit early
	if (imageDataBuffer.Size() < header.ImageSpec.Height * header.ImageSpec.Width * colorSize){
		return false;
	}


	switch (header.ImageSpec.BitsPerPixel){
	case 32:{
		size_t x, y;
		size_t offset = 0;
		const size_t imageSize = imageDataBuffer.Size();
		uint8_t* pOutputImageData = (uint8_t*)output.ImageData.GetPointer();
		const uint8_t* pImageData = (const uint8_t*)imageDataBuffer.GetPointer();
		for (y = 0; y < header.ImageSpec.Height; ++y){
			for (x = 0; x < header.ImageSpec.Width; ++x){
				const size_t index = y * header.ImageSpec.Width + x;

				const size_t baseOffset = ComputeOutputIndex(x, y, header) * 4;
				const uint32_t px = FetchU32(pImageData + index * 4);
				const uint8_t r = (px >> 16) & 0xFF;
				const uint8_t g = (px >> 8) & 0xFF;
				const uint8_t b = (px) & 0xFF;
				const uint8_t a = (px >> 24) & 0xFF;
				pOutputImageData[baseOffset + 0] = r;
				pOutputImageData[baseOffset + 1] = g;
				pOutputImageData[baseOffset + 2] = b;
				pOutputImageData[baseOffset + 3] = a;
			}
		}
		break;
	}
	case 24:{
		size_t x, y;
		size_t offset = 0;
		const size_t imageSize = imageDataBuffer.Size();
		uint8_t* pOutputImageData = (uint8_t*)output.ImageData.GetPointer();
		const uint8_t* pImageData = (const uint8_t*)imageDataBuffer.GetPointer();
		for (y = 0; y < header.ImageSpec.Height; ++y){
			for (x = 0; x < header.ImageSpec.Width; ++x){
				const size_t index = y * header.ImageSpec.Width + x;
				const size_t baseOffset = ComputeOutputIndex(x, y, header) * 4;
				const uint32_t px = FetchU24(pImageData + index * 3);
				const uint8_t r = (px >> 16) & 0xFF;
				const uint8_t g = (px >> 8) & 0xFF;
				const uint8_t b = (px) & 0xFF;
				const uint8_t a = 0xFF;
				pOutputImageData[baseOffset + 0] = r;
				pOutputImageData[baseOffset + 1] = g;
				pOutputImageData[baseOffset + 2] = b;
				pOutputImageData[baseOffset + 3] = a;
			}
		}
		break;
	}
	case 16:{
		size_t x, y;
		size_t offset = 0;
		const size_t imageSize = imageDataBuffer.Size();
		uint8_t* pOutputImageData = (uint8_t*)output.ImageData.GetPointer();
		const uint8_t* pImageData = (const uint8_t*)imageDataBuffer.GetPointer();
		for (y = 0; y < header.ImageSpec.Height; ++y){
			for (x = 0; x < header.ImageSpec.Width; ++x){
				const size_t index = y * header.ImageSpec.Width + x;
				const size_t baseOffset = ComputeOutputIndex(x, y, header) * 4;
				const uint32_t px = FetchU16(pImageData + index * 2);
				const uint8_t r = Expand5Bits((px >> 10) & 0x1F);
				const uint8_t g = Expand5Bits((px >> 5) & 0x1F);
				const uint8_t b = Expand5Bits((px) & 0x1F);
				const uint8_t a = ((px >> 15) & 1) * 0xFF;
				pOutputImageData[baseOffset + 0] = r;
				pOutputImageData[baseOffset + 1] = g;
				pOutputImageData[baseOffset + 2] = b;
				pOutputImageData[baseOffset + 3] = a;
			}
		}
		break;
	}
	case 15:{
		size_t x, y;
		size_t offset = 0;
		const size_t imageSize = imageDataBuffer.Size();
		uint8_t* pOutputImageData = (uint8_t*)output.ImageData.GetPointer();
		const uint8_t* pImageData = (const uint8_t*)imageDataBuffer.GetPointer();
		for (y = 0; y < header.ImageSpec.Height; ++y){
			for (x = 0; x < header.ImageSpec.Width; ++x){
				const size_t index = y * header.ImageSpec.Width + x;
				const size_t baseOffset = ComputeOutputIndex(x, y, header) * 4;
				const uint32_t px = FetchU16(pImageData + index * 2);
				const uint8_t r = Expand5Bits((px >> 10) & 0x1F);
				const uint8_t g = Expand5Bits((px >> 5) & 0x1F);
				const uint8_t b = Expand5Bits((px) & 0x1F);
				const uint8_t a = 0xFF;
				pOutputImageData[baseOffset + 0] = r;
				pOutputImageData[baseOffset + 1] = g;
				pOutputImageData[baseOffset + 2] = b;
				pOutputImageData[baseOffset + 3] = a;
			}
		}
		break;
	}
	default:{
		return false;
	}
	}
	return true;
}


bool TGADecodeToRGBA(const CBlob &inputBuffer, STGAOutput &output, STGADecodeCache &cache){
	

	if (inputBuffer.Size() < g_TGAHeaderSize){
		return false;
	}



	STGAHeader header;

	if (!TGAHeaderDecode(inputBuffer.GetPointer(), inputBuffer.Size(), header)){
		return false;
	}

	if (header.ColorMapType != 0 && header.ColorMapType != 1){
		return false;
	}
	if (header.ImageType != 0 &&
		header.ImageType != 1 &&
		header.ImageType != 2 &&
		header.ImageType != 3 &&
		header.ImageType != 9 &&
		header.ImageType != 10 &&
		header.ImageType != 11){
		return false;
	}
	if (header.ColorMapType == 1 &&
		header.ColorMapSpec.EntrySize != 15 &&
		header.ColorMapSpec.EntrySize != 16 &&
		header.ColorMapSpec.EntrySize != 24 &&
		header.ColorMapSpec.EntrySize != 32){
		return false;
	}
	if (header.ImageSpec.BitsPerPixel != 8 &&
		header.ImageSpec.BitsPerPixel != 16 &&
		header.ImageSpec.BitsPerPixel != 24 &&
		header.ImageSpec.BitsPerPixel != 32){
		return false;
	}
	if (header.ImageType == 0){
		output.Height = output.Width = 0;
		output.ImageData.Resize(0);
		return true;
	}
	output.ImageData.Resize(header.ImageSpec.Width * header.ImageSpec.Height * 4);
	output.HasAlpha = false;

	switch (header.ImageType){
	case 1:{
		if (!TGADecodeColorMap(inputBuffer, output, header, cache.ImageDataBuffer, false)){
			return false;
		}
		output.HasAlpha = header.ColorMapSpec.EntrySize == 32 || header.ColorMapSpec.EntrySize == 16;
		break;
	}
	case 2:{
		if (!TGADecodeRGB(inputBuffer, output, header, cache.ImageDataBuffer, false)){
			return false;
		}
		output.HasAlpha = header.ImageSpec.BitsPerPixel == 32 || header.ImageSpec.BitsPerPixel == 16;
		break;
	}
	case 3:{
		if (!TGADecodeGrayScale(inputBuffer, output, header, cache.ImageDataBuffer, false)){
			return false;
		}
		break;
	}
	case 9:{
		if (!TGADecodeColorMap(inputBuffer, output, header, cache.ImageDataBuffer, true)){
			return false;
		}
		output.HasAlpha = header.ColorMapSpec.EntrySize == 32 || header.ColorMapSpec.EntrySize == 16;
		break;
	}
	case 10:{
		if (!TGADecodeRGB(inputBuffer, output, header, cache.ImageDataBuffer, true)){
			return false;
		}
		output.HasAlpha = header.ImageSpec.BitsPerPixel == 32 || header.ImageSpec.BitsPerPixel == 16;
		break;
	}
	case 11:{
		if (!TGADecodeGrayScale(inputBuffer, output, header, cache.ImageDataBuffer, true)){
			return false;
		}
		break;
	}
	}
	output.Width = header.ImageSpec.Width;
	output.Height = header.ImageSpec.Height;
	return true;
}

size_t TGAHeaderGetSize(){
	return g_TGAHeaderSize;
}