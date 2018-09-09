#ifndef MAIN_H
#define MAIN_H

#define _WIN32_WINNT 0x0600
#include <Windows.h>
#include <CommCtrl.h>
#include "RefCount.h"
#include <d3d11.h>
#include <dxgi.h>
#include "ColorControl.h"
#include "String.h"

#define _WIN32_WINNT 0x0600

#define FIRST_MDI_MENU_ID	(0xFF00)

#define ID_MIP_COMBO_BOX	(0xF000)
#define ID_SLICE_COMBO_BOX	(0xF001)

enum : UINT {
	WM_GETFRAMEWINDOW = WM_USER,
	WM_GETDOCUMENTWINDOW,
	WM_GETVIEWWINDOW,
	WM_GETSTATUSBAR,
	WM_RERENDER,
	WM_GETTOOLBAR,
	WM_GETMIPCOMBOBOX,
	WM_GETSLICECOMBOBOX,
	WM_DOCUMENT_CLOSING,
};

#define DDS_FRAME_WINDOW_CLASS		L"DDS_VIEW_FRAME"
#define DDS_DOCUMENT_WINDOW_CLASS	L"DDS_VIEW_DOCUMENT"
#define DDS_VIEW_WINDOW_CLASS		L"DDS_VIEW_VIEW"


template<typename T, size_t TSize>
inline constexpr size_t GetArraySize(const T(&)[TSize]){
	return TSize;
}


enum class eToolbarButton : size_t{
	RGB = 0,
	RGBA,
	R,
	G,
	B,
	A,

	PositiveX,
	NegativeX,
	PositiveY,
	NegativeY,
	PositiveZ,
	NegativeZ,

	Count,
};

enum class eToolbarComboBox : size_t{
	Mip = 0,
	Slice,
	Count,
};

struct SVec2{
	float	X;
	float	Y;
};
struct SVec3{
	float	X;
	float	Y;
	float	Z;
};

struct SVertex{
	float	X, Y;
	float	U, V;
};

enum : size_t{
	Component_RGBA = 0,
	Component_RGB,
	Component_R,
	Component_G,
	Component_B,
	Component_A,
	Component_Count
};

enum : size_t {
	Swap_None = 0,
	Swap_RB,
	Swap_Count,
};

enum : size_t {
	Type_UNorm = 0,
	Type_SNorm,
	Type_Float,
	Type_SInt,
	Type_UInt,
	Type_Count,
};

class CGlobalState{
public:
	HACCEL										Accelerator = nullptr;
	HMENU										hViewMenu = nullptr;
	HMENU										hFileMenu = nullptr;
	HWND										hFrameWindow = nullptr;
	HIMAGELIST									hImageList = nullptr;
	HINSTANCE									hInstance = nullptr;

	TRefCountHandle<ID3D11Device>				Device;
	TRefCountHandle<ID3D11DeviceContext>		Context;
	TRefCountHandle<IDXGIFactory>				DXGIFactory;
	TRefCountHandle<IDXGIAdapter>				DXGIAdapter;
	TRefCountHandle<ID3D11Buffer>				IndexBuffer;
	TRefCountHandle<ID3D11Buffer>				VertexBuffer;
	TRefCountHandle<ID3D11InputLayout>			InputLayout;

	TRefCountHandle<ID3D11VertexShader>			VertexShader;
	TRefCountHandle<ID3D11PixelShader>			PixelShaders
		[Type_Count][Swap_Count][Component_Count];


	TRefCountHandle<ID3D11VertexShader>			CheckerBoardVertexShader;
	TRefCountHandle<ID3D11PixelShader>			CheckerBoardPixelShader;
	//... states
	TRefCountHandle<ID3D11DepthStencilState>	DepthStencilState;
	TRefCountHandle<ID3D11SamplerState>			SamplerState;
	TRefCountHandle<ID3D11BlendState>			OpaqueBlendState;
	TRefCountHandle<ID3D11BlendState>			AlphaBlendState;


	//Settings
	CCRGB										ColorA = { 0.5f, 0.5f, 0.5f };
	CCRGB										ColorB = { 0.75f, 0.75f, 0.75f };
	CCRGB										FillColor = { 0.0f, 0.0f, 0.0f };
	bool										CheckerBoard = true;

	CStringW									SaveFolder;
	CStringW									OpenFolder;
};

CGlobalState &GetGlobalState();
void LogFormatted(const wchar_t* pFmt, ...);

#endif //!MAIN_H
