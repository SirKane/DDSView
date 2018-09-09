#ifndef DDSLOADER_H
#define DDSLOADER_H

#include <stdint.h>
#include "Blob.h"
#include <dxgiformat.h>

enum class eTextureType{
	Invalid = 0,
	_1D = 1,
	_2D = 2,
	_3D = 3,
	_Cube = 4,
};


struct SDDSMipInfo{
	uint32_t	Width;
	uint32_t	Height;
	uint32_t	Depth;
	size_t		RowStride;
	size_t		DepthStride;
	size_t		Offset; //Offset from the start of the slice
	size_t		Size;
};

class CDDSImageData{
public:
	DXGI_FORMAT		Format = DXGI_FORMAT_UNKNOWN;
	uint32_t		MipCount = 0;
	uint32_t		ArraySliceCount = 0;
	uint32_t		Width = 0;
	uint32_t		Height = 0;
	uint32_t		Depth = 0;
	eTextureType	TextureType = eTextureType::Invalid;
	CBlob			ImageData;
	CBlob			LayoutStorage;

	SDDSMipInfo		MipInfos[15];
	size_t			SliceStride = 0;

	void InitializeMipInfos();


};

bool LoadDDSImageData(const CBlob &ddsFileData, CDDSImageData& outputImageData);

#endif //!DDSLOADER_H
