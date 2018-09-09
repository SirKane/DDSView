#include "DDSLoader.h"
#include "FormatInfo.h"
#include <xmmintrin.h>
#include <emmintrin.h>
#include <tmmintrin.h>
#include <d3d11.h>

/*
Controls whether DXGI_FORMAT_R8G8B8A8_UNORM gets the red and blue channels swapped to support
poorly written DDS loaders/writers
*/

#define	DDS_SUPPORT_BAD_LOADERS 1

#pragma pack(push, 1)

#define DDS_MAGIC 0x20534444 // "DDS "

struct DDS_PIXELFORMAT{
	uint32_t	Size;
	uint32_t	Flags;
	uint32_t	FourCC;
	uint32_t	RGBBitCount;
	uint32_t	RBitMask;
	uint32_t	GBitMask;
	uint32_t	BBitMask;
	uint32_t	ABitMask;
};

#define DDS_FOURCC      0x00000004  // DDPF_FOURCC
#define DDS_RGB         0x00000040  // DDPF_RGB
#define DDS_RGBA        0x00000041  // DDPF_RGB | DDPF_ALPHAPIXELS
#define DDS_LUMINANCE   0x00020000  // DDPF_LUMINANCE
#define DDS_LUMINANCEA  0x00020001  // DDPF_LUMINANCE | DDPF_ALPHAPIXELS
#define DDS_ALPHA       0x00000002  // DDPF_ALPHA
#define DDS_PAL8        0x00000020  // DDPF_PALETTEINDEXED8

#define DDS_HEADER_FLAGS_TEXTURE        0x00001007  // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT
#define DDS_HEADER_FLAGS_MIPMAP         0x00020000  // DDSD_MIPMAPCOUNT
#define DDS_HEADER_FLAGS_VOLUME         0x00800000  // DDSD_DEPTH
#define DDS_HEADER_FLAGS_PITCH          0x00000008  // DDSD_PITCH
#define DDS_HEADER_FLAGS_LINEARSIZE     0x00080000  // DDSD_LINEARSIZE

#define DDS_HEIGHT 0x00000002 // DDSD_HEIGHT
#define DDS_WIDTH  0x00000004 // DDSD_WIDTH

#define DDS_SURFACE_FLAGS_COMPLEX 0x00000008
#define DDS_SURFACE_FLAGS_TEXTURE 0x00001000 // DDSCAPS_TEXTURE
#define DDS_SURFACE_FLAGS_MIPMAP  0x00400008 // DDSCAPS_COMPLEX | DDSCAPS_MIPMAP
#define DDS_SURFACE_FLAGS_CUBEMAP 0x00000008 // DDSCAPS_COMPLEX

#define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES (DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
                              DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
                              DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ)

#define DDS_CUBEMAP 0x00000200 // DDSCAPS2_CUBEMAP

#define DDS_FLAGS_VOLUME 0x00200000 // DDSCAPS2_VOLUME

#define DDS_RESOURCE_MISC_TEXTURECUBE	0x4

#define DDS_MAX_MIPS	15	//D3D11_REQ_MIP_LEVELS

typedef struct{
	uint32_t		Size; //4
	uint32_t		Flags; //8
	uint32_t		Height; //C
	uint32_t		Width; //10
	uint32_t		PitchOrLinearSize; //14
	uint32_t		Depth; // only if DDS_HEADER_FLAGS_VOLUME is set in flags //18
	uint32_t		MipMapCount; //1C
	uint32_t		Reserved1[11];
	DDS_PIXELFORMAT	PixelFormat;
	uint32_t		Caps;
	uint32_t		Caps2;
	uint32_t		Caps3;
	uint32_t		Caps4;
	uint32_t		Reserved2;
} DDS_HEADER;

typedef struct{
	DXGI_FORMAT	DXGIFormat;
	uint32_t	ResourceDimension;
	uint32_t	MiscFlag; // See D3D11_RESOURCE_MISC_FLAG
	uint32_t	ArraySize;
	uint32_t	Reserved;
} DDS_HEADER_DXT10;

static_assert(sizeof(DDS_HEADER) == 0x7C, "sizeof(DDS_HEADER) == 0x7C");

#define DDS_MAKE_FOURCC(ch0, ch1, ch2, ch3) \
                ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) | \
                ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24))

#pragma pack(pop)

enum DXGI_FORMAT_TRANSCODE{
	DXGI_FORMAT_XC_BASE = 2000,
	DXGI_FORMAT_XC_B8G8R8A8, //D3DFMT_A8R8G8B8
	DXGI_FORMAT_XC_B8G8R8X8, //D3DFMT_X8R8G8B8
	DXGI_FORMAT_XC_R8G8B8X8, //D3DFMT_X8B8G8R8
	DXGI_FORMAT_XC_A2R10G10B10, //D3DFMT_A2R10G10B10
	DXGI_FORMAT_XC_B8G8R8, //D3DFMT_R8G8B8
	DXGI_FORMAT_XC_B5G5R5X1, //D3DFMT_X1R5G5B5
	DXGI_FORMAT_XC_B4G4R4A4, //D3DFMT_A4R4G4B4
	DXGI_FORMAT_XC_B4G4R4X4, //D3DFMT_X4R4G4B4
	DXGI_FORMAT_XC_B2G3R3A8, //D3DFMT_A8R3G3B2
							 //DXGI_FORMAT_XC_A8L8, //D3DFMT_A8L8
							 //DXGI_FORMAT_XC_L16, //D3DFMT_L16
							 //DXGI_FORMAT_XC_L8, //D3DFMT_L8
	DXGI_FORMAT_XC_L4A4, //D3DFMT_A4L4
};

static DXGI_FORMAT FormatToDXGIFormat(uint32_t format){
	switch (format){
	case DXGI_FORMAT_XC_B8G8R8A8:
	case DXGI_FORMAT_XC_B8G8R8X8:
	case DXGI_FORMAT_XC_R8G8B8X8:
	case DXGI_FORMAT_XC_A2R10G10B10:/*{
		return DXGI_FORMAT_R32_TYPELESS;
	}*/
	/*case DXGI_FORMAT_XC_B8G8R8:{
		return DXGI_FORMAT_R8_TYPELESS; //Hack
	}*/
	case DXGI_FORMAT_XC_B8G8R8:
	case DXGI_FORMAT_XC_B5G5R5X1:
	case DXGI_FORMAT_XC_B4G4R4A4:
	case DXGI_FORMAT_XC_B4G4R4X4:
	case DXGI_FORMAT_XC_B2G3R3A8:
	case DXGI_FORMAT_XC_L4A4:{
		return DXGI_FORMAT_R8_TYPELESS;
	}
	default: {
		return (DXGI_FORMAT)format;
	}
	}
}

#define SSE_CONVERT

class CImageDataConverter{
protected:
	CBlob	m_OutputBuffer;
	void Convert_B8G8R8A8(const uint8_t* pSourceData, size_t numTexels){
		size_t i = 0;
		uint8_t* pDest = (uint8_t*)m_OutputBuffer.GetPointer();
#ifdef SSE_CONVERT
		size_t numVectorized = numTexels / 4 * 4;

		__m128i shuffleMask = _mm_set_epi8(
			15, 12, 13, 14,
			11, 8, 9, 10,
			7, 4, 5, 6,
			3, 0, 1, 2);

		for (; i < numVectorized; i += 4){
			_mm_storeu_si128((__m128i*)pDest,
				_mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)pSourceData), shuffleMask));
			pSourceData += 16;
			pDest += 16;
		}
#endif //SSE_CONVERT
		for (; i < numTexels; ++i){
			pDest[0] = pSourceData[2];
			pDest[1] = pSourceData[1];
			pDest[2] = pSourceData[0];
			pDest[3] = pSourceData[3];
			pDest += 4;
			pSourceData += 4;
		}
	}

	void Convert_B8G8R8X8(const uint8_t* pSourceData, size_t numTexels){
		size_t i = 0;
		uint8_t* pDest = (uint8_t*)m_OutputBuffer.GetPointer();
#ifdef SSE_CONVERT
		size_t numVectorized = numTexels / 4 * 4;

		__m128i shuffleMask = _mm_set_epi8(
			15, 12, 13, 14,
			11, 8, 9, 10,
			7, 4, 5, 6,
			3, 0, 1, 2);

		__m128i alphaValues = _mm_set_epi8(
			(char)0xFF, 0, 0, 0,
			(char)0xFF, 0, 0, 0,
			(char)0xFF, 0, 0, 0,
			(char)0xFF, 0, 0, 0);

		for (; i < numVectorized; i += 4){
			_mm_storeu_si128((__m128i*)pDest,
				_mm_or_si128(
					_mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)pSourceData), shuffleMask),
					alphaValues));

			pSourceData += 16;
			pDest += 16;
		}
#endif //SSE_CONVERT
		for (; i < numTexels; ++i){
			pDest[0] = pSourceData[2];
			pDest[1] = pSourceData[1];
			pDest[2] = pSourceData[0];
			pDest[3] = 0xFF;
			pDest += 4;
			pSourceData += 4;
		}
	}
	void Convert_R8G8B8X8(const uint8_t* pSourceData, size_t numTexels){
		size_t i = 0;
		uint8_t* pDest = (uint8_t*)m_OutputBuffer.GetPointer();
#ifdef SSE_CONVERT
		size_t numVectorized = numTexels / 4 * 4;


		__m128i alphaValues = _mm_set_epi8(
			(char)0xFF, 0, 0, 0,
			(char)0xFF, 0, 0, 0,
			(char)0xFF, 0, 0, 0,
			(char)0xFF, 0, 0, 0);

		for (; i < numVectorized; i += 4){
			_mm_storeu_si128((__m128i*)pDest,
				_mm_or_si128(
					_mm_loadu_si128((const __m128i*)pSourceData),
					alphaValues));

			pSourceData += 16;
			pDest += 16;
		}
#endif //SSE_CONVERT
		for (; i < numTexels; ++i){
			pDest[0] = pSourceData[0];
			pDest[1] = pSourceData[1];
			pDest[2] = pSourceData[2];
			pDest[3] = 0xFF;
			pDest += 4;
			pSourceData += 4;
		}
	}

	void Convert_B8G8R8(const uint8_t* pSourceData, size_t numTexels){
		size_t i = 0;
		uint8_t* pDest = (uint8_t*)m_OutputBuffer.GetPointer();
#ifdef SSE_CONVERT
		//There's overlap
		size_t numVectorized = numTexels > 2 ? (numTexels - 2) / 4 * 4 : 0;

		__m128i shuffleMask = _mm_set_epi8(
			(char)0x80, 9, 10, 11,
			(char)0x80, 6, 7, 8,
			(char)0x80, 3, 4, 5,
			(char)0x80, 0, 1, 2);

		__m128i alphaValues = _mm_set_epi8(
			(char)0xFF, 0, 0, 0,
			(char)0xFF, 0, 0, 0,
			(char)0xFF, 0, 0, 0,
			(char)0xFF, 0, 0, 0);

		for (; i < numVectorized; i += 4){
			_mm_storeu_si128((__m128i*)pDest,
				_mm_or_si128(
					_mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)pSourceData), shuffleMask),
					alphaValues));

			pSourceData += 12;
			pDest += 16;
		}
#endif //SSE_CONVERT
		for (; i < numTexels; ++i){
			pDest[0] = pSourceData[2];
			pDest[1] = pSourceData[1];
			pDest[2] = pSourceData[0];
			pDest[3] = 0xFF;
			pDest += 4;
			pSourceData += 3;
		}
	}


	void Convert_B5G6R5(const uint8_t* pSourceData, size_t numTexels){
		size_t i = 0;
		uint8_t* pDest = (uint8_t*)m_OutputBuffer.GetPointer();
		for (; i < numTexels; ++i){
			uint8_t v0 = pSourceData[0];
			uint8_t v1 = pSourceData[1];

			uint8_t r = (v1 >> 5) & 0x1F;
			uint8_t g = ((v1 & 0x07) << 3) | ((v0 >> 5) & 0x07);
			uint8_t b = (v0) & 0x1F;

			r = (uint8_t)(((uint32_t)(r)) * 539087 / 65536);
			g = (uint8_t)(((uint32_t)(g)) * 265265 / 65536);
			b = (uint8_t)(((uint32_t)(b)) * 539087 / 65536);


			pDest[0] = r;
			pDest[1] = g;
			pDest[2] = b;
			pDest[3] = 0xFF;
			pDest += 4;
			pSourceData += 2;
		}
	}
	void Convert_B5G5R5X1(const uint8_t* pSourceData, size_t numTexels){
		size_t i = 0;
		uint8_t* pDest = (uint8_t*)m_OutputBuffer.GetPointer();
		for (; i < numTexels; ++i){
			uint8_t v0 = pSourceData[0];
			uint8_t v1 = pSourceData[1];

			uint8_t r = (v1 >> 2) & 0x1F;
			uint8_t g = ((v1 & 0x03) << 3) | ((v0 >> 5) & 0x07);
			uint8_t b = (v0) & 0x1F;

			r = (uint8_t)(((uint32_t)(r)) * 539087 / 65536);
			g = (uint8_t)(((uint32_t)(g)) * 539087 / 65536);
			b = (uint8_t)(((uint32_t)(b)) * 539087 / 65536);


			pDest[0] = r;
			pDest[1] = g;
			pDest[2] = b;
			pDest[3] = 0xFF;
			pDest += 4;
			pSourceData += 2;
		}
	}
	void Convert_B5G5R5A1(const uint8_t* pSourceData, size_t numTexels){
		size_t i = 0;
		uint8_t* pDest = (uint8_t*)m_OutputBuffer.GetPointer();
		for (; i < numTexels; ++i){
			uint8_t v0 = pSourceData[0];
			uint8_t v1 = pSourceData[1];

			uint8_t r = (v1 >> 2) & 0x1F;
			uint8_t g = ((v1 & 0x03) << 3) | ((v0 >> 5) & 0x07);
			uint8_t b = (v0) & 0x1F;
			uint8_t a = (v1 >> 7) & 0x01;

			r = (uint8_t)(((uint32_t)(r)) * 539087 / 65536);
			g = (uint8_t)(((uint32_t)(g)) * 539087 / 65536);
			b = (uint8_t)(((uint32_t)(b)) * 539087 / 65536);
			a = a ? 0xFF : 0x00;


			pDest[0] = r;
			pDest[1] = g;
			pDest[2] = b;
			pDest[3] = a;
			pDest += 4;
			pSourceData += 2;
		}
	}
	void Convert_B4G4R4A4(const uint8_t* pSourceData, size_t numTexels){
		size_t i = 0;
		uint8_t* pDest = (uint8_t*)m_OutputBuffer.GetPointer();
		for (; i < numTexels; ++i){
			uint8_t v0 = pSourceData[0];
			uint8_t v1 = pSourceData[1];

			uint8_t r = v1 & 0xF;
			uint8_t b = v0 & 0xF;
			uint8_t g = (v0 >> 4) & 0xF;
			uint8_t a = (v1 >> 4) & 0xF;

			r = r * 17;
			g = g * 17;
			b = b * 17;
			a = a * 17;

			pDest[0] = r;
			pDest[1] = g;
			pDest[2] = b;
			pDest[3] = a;
			pDest += 4;
			pSourceData += 2;
		}
	}
	void Convert_B4G4R4X4(const uint8_t* pSourceData, size_t numTexels){
		size_t i = 0;
		uint8_t* pDest = (uint8_t*)m_OutputBuffer.GetPointer();
		for (; i < numTexels; ++i){
			uint8_t v0 = pSourceData[0];
			uint8_t v1 = pSourceData[1];

			uint8_t r = v1 & 0xF;
			uint8_t b = v0 & 0xF;
			uint8_t g = (v0 >> 4) & 0xF;

			r = r * 17;
			g = g * 17;
			b = b * 17;

			pDest[0] = r;
			pDest[1] = g;
			pDest[2] = b;
			pDest[3] = 0xFF;
			pDest += 4;
			pSourceData += 2;
		}
	}
	void Convert_B2G3R3A8(const uint8_t* pSourceData, size_t numTexels){
		size_t i = 0;
		uint8_t* pDest = (uint8_t*)m_OutputBuffer.GetPointer();
		for (; i < numTexels; ++i){
			uint8_t v = pSourceData[0];
			uint8_t r = (v >> 5) & 7;
			uint8_t g = (v >> 2) & 7;
			uint8_t b = (v) & 3;
			uint8_t a = pSourceData[1];

			r = (uint8_t)(((uint32_t)(r)) * 2387383 / 65536);
			g = (uint8_t)(((uint32_t)(g)) * 2387383 / 65536);
			b = b * 85;

			pDest[0] = r;
			pDest[1] = g;
			pDest[2] = b;
			pDest[3] = a;
			pDest += 4;
			pSourceData += 2;
		}
	}
	void Convert_L4A4(const uint8_t* pSourceData, size_t numTexels){
		size_t i = 0;
		uint8_t* pDest = (uint8_t*)m_OutputBuffer.GetPointer();
		for (; i < numTexels; ++i){
			uint8_t v = pSourceData[0];
			uint8_t l = v & 0x0F;
			uint8_t a = (v >> 4) & 0x0F;

			l = l * 17;
			a = a * 17;

			pDest[0] = l;
			pDest[1] = l;
			pDest[2] = l;
			pDest[3] = a;
			pDest += 4;
			++pSourceData;
		}
	}
public:
	DXGI_FORMAT Convert(const void* pSourceData, size_t sourceDataSize, size_t numElements, uint32_t format){
		if (format == DXGI_FORMAT_XC_B8G8R8A8){
			if (sourceDataSize < numElements * 4){
				return DXGI_FORMAT_UNKNOWN;
			}
			m_OutputBuffer.Resize(numElements * 4);
			Convert_B8G8R8A8((const uint8_t*)pSourceData, numElements);
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		} else if (format == DXGI_FORMAT_XC_B8G8R8X8){
			if (sourceDataSize < numElements * 4){
				return DXGI_FORMAT_UNKNOWN;
			}
			m_OutputBuffer.Resize(numElements * 4);
			Convert_B8G8R8X8((const uint8_t*)pSourceData, numElements);
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		} else if (format == DXGI_FORMAT_XC_R8G8B8X8){
			if (sourceDataSize < numElements * 4){
				return DXGI_FORMAT_UNKNOWN;
			}
			m_OutputBuffer.Resize(numElements * 4);
			Convert_R8G8B8X8((const uint8_t*)pSourceData, numElements);
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		} else if (format == DXGI_FORMAT_XC_B8G8R8){
			if (sourceDataSize < numElements * 3){
				return DXGI_FORMAT_UNKNOWN;
			}
			m_OutputBuffer.Resize(numElements * 4);
			Convert_B8G8R8((const uint8_t*)pSourceData, numElements);
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		} else if (format == DXGI_FORMAT_B5G6R5_UNORM){
			if (sourceDataSize < numElements * 2){
				return DXGI_FORMAT_UNKNOWN;
			}
			m_OutputBuffer.Resize(numElements * 4);
			Convert_B5G6R5((const uint8_t*)pSourceData, numElements);
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		} else if (format == DXGI_FORMAT_XC_B5G5R5X1){
			if (sourceDataSize < numElements * 2){
				return DXGI_FORMAT_UNKNOWN;
			}
			m_OutputBuffer.Resize(numElements * 4);
			Convert_B5G5R5X1((const uint8_t*)pSourceData, numElements);
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		} else if (format == DXGI_FORMAT_B5G5R5A1_UNORM){
			if (sourceDataSize < numElements * 2){
				return DXGI_FORMAT_UNKNOWN;
			}
			m_OutputBuffer.Resize(numElements * 4);
			Convert_B5G5R5A1((const uint8_t*)pSourceData, numElements);
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		} else if (format == DXGI_FORMAT_XC_B4G4R4A4){
			if (sourceDataSize < numElements * 2){
				return DXGI_FORMAT_UNKNOWN;
			}
			m_OutputBuffer.Resize(numElements * 4);
			Convert_B4G4R4A4((const uint8_t*)pSourceData, numElements);
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		} else if (format == DXGI_FORMAT_XC_B4G4R4X4){
			if (sourceDataSize < numElements * 2){
				return DXGI_FORMAT_UNKNOWN;
			}
			m_OutputBuffer.Resize(numElements * 4);
			Convert_B4G4R4X4((const uint8_t*)pSourceData, numElements);
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		} else if (format == DXGI_FORMAT_XC_B2G3R3A8){
			if (sourceDataSize < numElements * 2){
				return DXGI_FORMAT_UNKNOWN;
			}
			m_OutputBuffer.Resize(numElements * 4);
			Convert_B2G3R3A8((const uint8_t*)pSourceData, numElements);
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		} else if (format == DXGI_FORMAT_XC_L4A4){
			if (sourceDataSize < numElements * 1){
				return DXGI_FORMAT_UNKNOWN;
			}
			m_OutputBuffer.Resize(numElements * 4);
			Convert_L4A4((const uint8_t*)pSourceData, numElements);
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		return DXGI_FORMAT_UNKNOWN;
	}
	bool Supports(uint32_t Format){
		switch (Format){
		case DXGI_FORMAT_XC_B8G8R8A8:
		case DXGI_FORMAT_XC_B8G8R8X8:
		case DXGI_FORMAT_XC_R8G8B8X8:
		case DXGI_FORMAT_XC_B8G8R8:
		case DXGI_FORMAT_B5G6R5_UNORM:
		case DXGI_FORMAT_XC_B5G5R5X1:
		case DXGI_FORMAT_B5G5R5A1_UNORM:
		case DXGI_FORMAT_XC_B4G4R4A4:
		case DXGI_FORMAT_XC_B4G4R4X4:
		case DXGI_FORMAT_XC_B2G3R3A8:
		case DXGI_FORMAT_XC_L4A4:
			return true;
		}
		return false;
	}
	const void* GetImageData(){
		return m_OutputBuffer.GetPointer();
	}
	size_t GetImageDataSize(){
		return m_OutputBuffer.Size();
	}
};

CImageDataConverter	g_CImageDataConverter;

static uint32_t DDSGetTextureFormat(const DDS_PIXELFORMAT &pf){
	if ((pf.Flags & DDS_RGB) != 0){
		if (pf.RGBBitCount == 32){
			if (pf.RBitMask == 0x000000FF && pf.GBitMask == 0x0000FF00 &&
				pf.BBitMask == 0x00FF0000 && pf.ABitMask == 0xFF000000){
				return DXGI_FORMAT_R8G8B8A8_UNORM;
			} else if (pf.RBitMask == 0x0000FFFF && pf.GBitMask == 0xFFFF0000 &&
				pf.BBitMask == 0x00000000 && pf.ABitMask == 0x00000000){
				return DXGI_FORMAT_R16G16_UNORM;
			} else if (pf.RBitMask == 0x000003FF && pf.GBitMask == 0x000FFC00 &&
				pf.BBitMask == 0x3FF00000 && pf.ABitMask == 0x00000000){
				return DXGI_FORMAT_R10G10B10A2_UNORM;
			} else if (pf.RBitMask == 0x0000FFFF && pf.GBitMask == 0xFFFF0000 &&
				pf.BBitMask == 0x00000000 && pf.ABitMask == 0x00000000){
				return DXGI_FORMAT_R16G16_UNORM;
			} else if (pf.RBitMask == 0x00FF0000 && pf.GBitMask == 0x0000FF00 &&
				pf.BBitMask == 0x000000FF && pf.ABitMask == 0xFF000000){
				return DXGI_FORMAT_XC_B8G8R8A8;
			} else if (pf.RBitMask == 0x00FF0000 && pf.GBitMask == 0x0000FF00 &&
				pf.BBitMask == 0x000000FF && pf.ABitMask == 0x00000000){
				return DXGI_FORMAT_XC_B8G8R8X8;
			} else if (pf.RBitMask == 0x000000FF && pf.GBitMask == 0x0000FF00 &&
				pf.BBitMask == 0x00FF0000 && pf.ABitMask == 0x00000000){
				return DXGI_FORMAT_XC_R8G8B8X8;
			} else if (pf.RBitMask == 0x3FF00000 && pf.GBitMask == 0x000FFC00 &&
				pf.BBitMask == 0x000003FF && pf.ABitMask == 0xC0000000){
				return DXGI_FORMAT_XC_A2R10G10B10;
			}
		} else if (pf.RGBBitCount == 24){
			if (pf.RBitMask == 0x00FF0000 && pf.GBitMask == 0x0000FF00 &&
				pf.BBitMask == 0x000000FF && pf.ABitMask == 0x00000000){
				return DXGI_FORMAT_XC_B8G8R8;
			}
		} else if (pf.RGBBitCount == 16){
			if (pf.RBitMask == 0x00007C00 && pf.GBitMask == 0x000003E0 &&
				pf.BBitMask == 0x0000001F && pf.ABitMask == 0x00008000){
				return DXGI_FORMAT_B5G5R5A1_UNORM;
			} else if (pf.RBitMask == 0x0000F800 && pf.GBitMask == 0x000007E0 &&
				pf.BBitMask == 0x0000001F && pf.ABitMask == 0x00000000){
				return DXGI_FORMAT_B5G6R5_UNORM;
			} else if (pf.RBitMask == 0x00007C00 && pf.GBitMask == 0x000003E0 &&
				pf.BBitMask == 0x0000001F && pf.ABitMask == 0x00000000){
				return DXGI_FORMAT_XC_B5G5R5X1;
			} else if (pf.RBitMask == 0x00000F00 && pf.GBitMask == 0x000000F0 &&
				pf.BBitMask == 0x0000000F && pf.ABitMask == 0x0000F000){
				return DXGI_FORMAT_XC_B4G4R4A4;
			} else if (pf.RBitMask == 0x00000F00 && pf.GBitMask == 0x000000F0 &&
				pf.BBitMask == 0x0000000F && pf.ABitMask == 0x00000000){
				return DXGI_FORMAT_XC_B4G4R4X4;
			} else if (pf.RBitMask == 0x000000E0 && pf.GBitMask == 0x0000001C &&
				pf.BBitMask == 0x00000003 && pf.ABitMask == 0x0000FF00){
				return DXGI_FORMAT_XC_B2G3R3A8;
			}
		} else if (pf.RGBBitCount == 8){
			if (pf.RBitMask == 0xFF && pf.GBitMask == 0 &&
				pf.BBitMask == 0 && pf.ABitMask == 0){
				return DXGI_FORMAT_R8_UNORM;
			}
		}
	}
	if ((pf.Flags & DDS_ALPHA) != 0){
		if (pf.RGBBitCount == 8){
			if (pf.RBitMask == 0x00000000 && pf.GBitMask == 0x00000000 &&
				pf.BBitMask == 0x00000000 && pf.ABitMask == 0x000000FF){
				return DXGI_FORMAT_A8_UNORM;
			}
		}
	}
	if ((pf.Flags & DDS_LUMINANCE) != 0){
		if (pf.RGBBitCount == 16){
			if (pf.RBitMask == 0x000000FF && pf.GBitMask == 0x00000000 &&
				pf.BBitMask == 0x00000000 && pf.ABitMask == 0x0000FF00){
				return DXGI_FORMAT_R8G8_UNORM;
			} else if (pf.RBitMask == 0x0000FFFF && pf.GBitMask == 0x00000000 &&
				pf.BBitMask == 0x00000000 && pf.ABitMask == 0x00000000){
				return DXGI_FORMAT_R16_UNORM;
			}
		} else if (pf.RGBBitCount == 8){
			if (pf.RBitMask == 0x000000FF && pf.GBitMask == 0x00000000 &&
				pf.BBitMask == 0x00000000 && pf.ABitMask == 0x00000000){
				return DXGI_FORMAT_R8_UNORM;
			} else if (pf.RBitMask == 0x0000000F && pf.GBitMask == 0x00000000 &&
				pf.BBitMask == 0x00000000 && pf.ABitMask == 0x000000F0){
				return DXGI_FORMAT_XC_L4A4;
			}
		}
	}
	if ((pf.Flags & DDS_FOURCC) != 0){
		if (pf.FourCC == 0x31545844){
			return DXGI_FORMAT_BC1_UNORM;
		} else if (pf.FourCC == 0x32545844){
			return DXGI_FORMAT_BC2_UNORM;
		} else if (pf.FourCC == 0x33545844){
			return DXGI_FORMAT_BC2_UNORM;
		} else if (pf.FourCC == 0x34545844){
			return DXGI_FORMAT_BC3_UNORM;
		} else if (pf.FourCC == 0x35545844){
			return DXGI_FORMAT_BC3_UNORM;
		} else if (pf.FourCC == 0x55344342){
			return DXGI_FORMAT_BC4_UNORM;
		} else if (pf.FourCC == 0x53344342){
			return DXGI_FORMAT_BC4_SNORM;
		} else if (pf.FourCC == 0x55354342){
			return DXGI_FORMAT_BC5_SNORM;
		} else if (pf.FourCC == 0x53354342){
			return DXGI_FORMAT_BC5_SNORM;
		} else if (pf.FourCC == 0x32495441){
			return DXGI_FORMAT_BC5_UNORM;
		} else if (pf.FourCC == 0x47424752){
			return DXGI_FORMAT_R8G8_B8G8_UNORM;
		} else if (pf.FourCC == 0x42475247){
			return DXGI_FORMAT_G8R8_G8B8_UNORM;
		} else if (pf.FourCC == 0x00000024){
			return DXGI_FORMAT_R16G16B16A16_UNORM;
		} else if (pf.FourCC == 0x0000006E){
			return DXGI_FORMAT_R16G16B16A16_SNORM;
		} else if (pf.FourCC == 0x0000006F){
			return DXGI_FORMAT_R16_FLOAT;
		} else if (pf.FourCC == 0x00000070){
			return DXGI_FORMAT_R16G16_FLOAT;
		} else if (pf.FourCC == 0x00000071){
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		} else if (pf.FourCC == 0x00000072){
			return DXGI_FORMAT_R32_FLOAT;
		} else if (pf.FourCC == 0x00000073){
			return DXGI_FORMAT_R32G32_FLOAT;
		} else if (pf.FourCC == 0x00000074){
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		}
	}
	return DXGI_FORMAT_UNKNOWN;
}

struct SSurfaceData{
	DXGI_FORMAT	Format;
	const void*	ImageData;
	size_t		ImageDataSize;
	uint32_t	Width;
	uint32_t	Height;
	uint32_t	Depth;
	uint32_t	ArraySlice;
	uint32_t	MipLevel;
	uint32_t	CubeFace;
	size_t		DepthStride;
	size_t		RowStride;

};
struct SSurfaceInputData{
	DXGI_FORMAT	Format;
	const void*	ImageData;
	size_t		ImageDataSize;
	uint32_t	Width;
	uint32_t	Height;
	uint32_t	Depth;
	uint32_t	ArraySize;
	uint32_t	MipLevels;
	bool		IsCube;
};

/*
(const SSurfaceData&surfaceData, const SSurfaceInputData &inputData)-> bool
*/
template<typename T> bool IterateDDSSurfaces(T &callback, const SSurfaceInputData &input){
	uint32_t arraySlice, cubeFace, mipLevel, cubeFaceCount;
	
	const unsigned char* pImageData = (const unsigned char*)input.ImageData;
	const unsigned char* pImageDataEnd = pImageData + input.ImageDataSize;
	SSurfaceData data;

	cubeFaceCount = input.IsCube ? 6 : 1;
	data.Format = input.Format;


	for (arraySlice = 0; arraySlice < input.ArraySize; ++arraySlice){
		data.ArraySlice = arraySlice;
		for (cubeFace = 0; cubeFace < cubeFaceCount; ++cubeFace){
			data.CubeFace = cubeFace;	
			data.Width = input.Width;
			data.Height = input.Height;
			data.Depth = input.Depth;

			for (mipLevel = 0; mipLevel < input.MipLevels; ++mipLevel){
				data.MipLevel = mipLevel;

				data.DepthStride = Format_ComputeDepthStride(data.Format, data.Width, data.Height);
				data.RowStride = Format_ComputeRowStride(data.Format, data.Width, data.Height);
				data.ImageDataSize = data.DepthStride * data.Depth;
				data.ImageData = pImageData;
				pImageData += data.ImageDataSize;

				if (pImageData > pImageDataEnd){
					return false;
				}

				if (!callback((const SSurfaceData&)data, (const SSurfaceInputData&)input)){
					return false;
				}

				Format_ShrinkMipLevelClamp(data.Width, data.Height, data.Depth);

			}
		}
	}
	return true;
}



void CDDSImageData::InitializeMipInfos(){
	size_t offset = 0;
	uint32_t mipLevel;

	uint32_t width = Width, height = Height, depth = Depth;

	for (mipLevel = 0; mipLevel < MipCount; ++mipLevel){
		SDDSMipInfo &info = MipInfos[mipLevel];
		info.Width = width;
		info.Height = height;
		info.Depth = depth;
		info.RowStride = Format_ComputeRowStride(Format, width, height);
		info.DepthStride = Format_ComputeDepthStride(Format, width, height);
		info.Offset = offset;
		info.Size = info.DepthStride * depth;

		offset += info.Size;

		Format_ShrinkMipLevelClamp(width, height, depth);
	}
	SliceStride = offset;
}

bool LoadDDSImageData(const CBlob &ddsFileData, CDDSImageData& outputImageData){


	if (ddsFileData.Size() < sizeof(uint32_t) + sizeof(DDS_HEADER)){
		return false;
	}
	const uint8_t* p = (const uint8_t*)ddsFileData.GetPointer();
	if (*(const uint32_t*)p != DDS_MAGIC){
		return false;
	}

	const DDS_HEADER* pHeader = (const DDS_HEADER*)(p + sizeof(uint32_t));
	if (pHeader->Size != sizeof(DDS_HEADER)){
		return false;
	}
	if (pHeader->PixelFormat.Size != sizeof(DDS_PIXELFORMAT)){
		return false;
	}
	//*************
	uint32_t width, height, depth, levels, format, arraySize, mipLevels;

	bool IsSRGB = false;
	width = pHeader->Width;
	height = pHeader->Height;
	mipLevels = pHeader->MipMapCount;
	arraySize = depth = levels = 1;

	if (mipLevels < 1){
		mipLevels = 1;
	} else if (mipLevels > DDS_MAX_MIPS){
		return false;
	}


	format = 0;
	size_t headerSize = sizeof(uint32_t) + sizeof(DDS_HEADER);
	eTextureType textureType = eTextureType::_2D;

	if ((pHeader->PixelFormat.Flags & DDS_FOURCC) != 0 &&
		pHeader->PixelFormat.FourCC == DDS_MAKE_FOURCC('D', 'X', '1', '0')){
		if (ddsFileData.Size() < sizeof(uint32_t) + sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10)){
			return false;
		}
		headerSize += sizeof(DDS_HEADER_DXT10);
		const DDS_HEADER_DXT10* pHeaderDXT10 = (const DDS_HEADER_DXT10*)(p + sizeof(uint32_t) + sizeof(DDS_HEADER));
		switch (pHeaderDXT10->ResourceDimension){
		case D3D10_RESOURCE_DIMENSION_TEXTURE1D:{
			textureType = eTextureType::_1D;
			break;
		}
		case D3D10_RESOURCE_DIMENSION_TEXTURE2D:{
			textureType = eTextureType::_2D;
			break;
		}
		case D3D10_RESOURCE_DIMENSION_TEXTURE3D:{
			textureType = eTextureType::_3D;
			break;
		}
		default:{
			return false;
		}
		}
		if (pHeaderDXT10->MiscFlag & DDS_RESOURCE_MISC_TEXTURECUBE){
			if (textureType != eTextureType::_2D){
				return false;
			}
			textureType = eTextureType::_Cube;
		}
		format = pHeaderDXT10->DXGIFormat;
		depth = textureType == eTextureType::_3D ? pHeader->Depth : 1;
		arraySize = textureType != eTextureType::_3D ? pHeaderDXT10->ArraySize : 1;
	} else {
		textureType = eTextureType::_2D;
		format = DDSGetTextureFormat(pHeader->PixelFormat);
		arraySize = 1;
		depth = (pHeader->Caps2 & DDS_FLAGS_VOLUME) != 0 ? pHeader->Depth : 1;
		if ((pHeader->Caps2 & DDS_FLAGS_VOLUME) != 0){
			if ((pHeader->Caps2 & DDS_CUBEMAP) != 0){
				return false;
			}
			textureType = eTextureType::_3D;
		} else if ((pHeader->Caps2 & DDS_CUBEMAP) != 0){
			//Only accepts cubemap DDS files with all six faces present
			if ((pHeader->Caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES){
				return false;
			}
			textureType = eTextureType::_Cube;
		}
	}
	if (format == 0 || depth < 1 || arraySize < 1){
		return false;
	}


	size_t elementCount = Format_ComputeElementCount(FormatToDXGIFormat(format), width, height, 
		mipLevels, depth, arraySize) * (textureType == eTextureType::_Cube ? 6 : 1);
	size_t imageDataSize;

	const void* pImageData = p + headerSize;
	DXGI_FORMAT targetFormat;
	if (g_CImageDataConverter.Supports(format)){
		targetFormat = g_CImageDataConverter.Convert(pImageData, ddsFileData.Size()- headerSize,
			elementCount, format);
		if (format == 0){
			return nullptr;
		}
		pImageData = g_CImageDataConverter.GetImageData();
		imageDataSize = g_CImageDataConverter.GetImageDataSize();
	} else {
		targetFormat = (DXGI_FORMAT)format;
		imageDataSize = Format_ComputeImageDataSize(targetFormat,
			width, height, mipLevels, depth, arraySize) * (textureType == eTextureType::_Cube ? 6 : 1);
		if (imageDataSize > ddsFileData.Size()- headerSize){
			return nullptr;
		}
	}


	outputImageData.ArraySliceCount = arraySize;
	outputImageData.Depth = depth;
	outputImageData.Format = targetFormat;
	outputImageData.Height = height;
	outputImageData.Width = width;
	outputImageData.MipCount = mipLevels;
	outputImageData.TextureType = textureType;


	if (!outputImageData.ImageData.Resize(imageDataSize)){
		return false;
	}
	/*memcpy(outputImageData.ImageData.GetPointer(), pImageData, imageDataSize);
	SSurfaceInputData inputData;
	inputData.ArraySize = arraySize;
	inputData.Depth = depth;
	inputData.Format = targetFormat;
	inputData.Height = height;
	inputData.Width = width;
	inputData.ImageData = pImageData;
	inputData.ImageDataSize = imageDataSize;
	inputData.IsCube = textureType == eTextureType::_Cube;
	inputData.MipLevels = mipLevels;*/

	outputImageData.InitializeMipInfos();
	memcpy(outputImageData.ImageData.GetPointer(), pImageData, imageDataSize);

	return true;
}