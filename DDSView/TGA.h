#ifndef SHARED_TGA_H
#define SHARED_TGA_H

#include <stdint.h>
#include "Blob.h"

struct STGAHeader{
	uint8_t	IDLength;
	uint8_t	ColorMapType;
	uint8_t	ImageType;
	struct {
		uint16_t	FirstEntryIndex;
		uint16_t	Length;
		uint8_t		EntrySize;
	}		ColorMapSpec;
	struct {
		uint16_t	XOrigin;
		uint16_t	YOrigin;
		uint16_t	Width;
		uint16_t	Height;
		uint8_t		BitsPerPixel;
		uint8_t		ImageDescriptor;
	}	ImageSpec;
};


struct STGAOutput{
	CBlob				ImageData;
	uint16_t			Width;
	uint16_t			Height;
	bool				HasAlpha;
};

struct STGADecodeCache{
	CBlob	ImageDataBuffer;
};

bool TGAHeaderDecode(const void* pData, size_t dataSize, STGAHeader &headerOut);
bool TGAHeaderEncode(void* pData, size_t dataSize, const STGAHeader &header);
bool TGADecodeToRGBA(const CBlob &inputBuffer, STGAOutput &output, STGADecodeCache &cache);
size_t TGAHeaderGetSize();


#endif //!SHARED_TGA_H