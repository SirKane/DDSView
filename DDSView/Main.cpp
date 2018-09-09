#define _WIN32_WINNT 0x0600

#include <Windows.h>
#include <CommCtrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <d3d11.h>
#include <dxgi.h>

#include "RefCount.h"
#include "Shaders.inl"
#include "resource.h"

#include <utility>
#include "String.h"

#include <gdiplus.h>

#include "ColorControl.h"

#include "Main.h"
#include "FileDialogEvents.h"
#include "DocumentWindow.h"



/*#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")*/



static const int s_ToolbarCommands[size_t(eToolbarButton::Count)] = {
	ID_RGB,
	ID_RGBA,
	ID_R,
	ID_G,
	ID_B,
	ID_A,

	ID_POS_X,
	ID_NEG_X,
	ID_POS_Y,
	ID_NEG_Y,
	ID_POS_Z,
	ID_NEG_Z,
};


static CGlobalState	s_GlobalState;

CGlobalState &GetGlobalState() {
	return s_GlobalState;
}

template<typename T>
void EnumAllChildWindowsT(HWND hParent, T functor){
	EnumChildWindows(hParent, [](HWND hWnd, LPARAM lParam) -> BOOL{
		return (*(T*)lParam)(hWnd);
	}, (LPARAM)&functor);
}
template<typename T>
void EnumChildWindowsT(HWND hParent, T functor){
	struct SParams{
		T* pFunctor;
		HWND hParent;
	};
	SParams params = {
		&functor,
		hParent,
	};
	EnumChildWindows(hParent, [](HWND hWnd, LPARAM lParam) -> BOOL{
		SParams *pParams = (SParams*)lParam;
		if (GetParent(hWnd) != pParams->hParent){
			return true;
		}
		return (*(pParams->pFunctor))(hWnd);
	}, (LPARAM)&params);
}


static TRefCountHandle<IDXGIFactory> GetFactoryAndAdapterFromDevice(
	TRefCountHandle<ID3D11Device> device, TRefCountHandle<IDXGIAdapter> &adapterOut){

	TRefCountHandle<IDXGIDevice> dxgiDevice;
	TRefCountHandle<IDXGIAdapter> dxgiAdapater;
	TRefCountHandle<IDXGIFactory> dxgiFactory;
	if (FAILED(device->QueryInterface(IID_IDXGIDevice, (void **)dxgiDevice.GetAddress()))){
		return TRefCountHandle<IDXGIFactory>();
	}

	if (FAILED(dxgiDevice->GetParent(IID_IDXGIAdapter, (void **)dxgiAdapater.GetAddress()))){
		return TRefCountHandle<IDXGIFactory>();
	}

	if (FAILED(dxgiAdapater->GetParent(IID_IDXGIFactory, (void **)dxgiFactory.GetAddress()))){
		return TRefCountHandle<IDXGIFactory>();
	}
	if (dxgiFactory){
		adapterOut = dxgiAdapater;
	}
	return dxgiFactory;
}


void ClearGlobals(){
	size_t i, j, k;


	if (s_GlobalState.Context){
		s_GlobalState.Context->ClearState();
	}

	for (i = 0; i < Type_Count; ++i){
		for (j = 0; j < Swap_Count; ++j){
			for (k = 0; k < Component_Count; ++k){
				s_GlobalState.PixelShaders[i][j][k].Clear();
			}
		}
	}
	s_GlobalState.SamplerState.Clear();
	s_GlobalState.OpaqueBlendState.Clear();
	s_GlobalState.AlphaBlendState.Clear();
	s_GlobalState.CheckerBoardPixelShader.Clear();
	s_GlobalState.CheckerBoardVertexShader.Clear();
	s_GlobalState.DepthStencilState.Clear();
	s_GlobalState.InputLayout.Clear();
	s_GlobalState.VertexBuffer.Clear();
	s_GlobalState.IndexBuffer.Clear();
	s_GlobalState.DXGIAdapter.Clear();
	s_GlobalState.DXGIFactory.Clear();
	s_GlobalState.Context.Clear();
	s_GlobalState.Device.Clear();
}

void LogFormatted(const wchar_t* pFmt, ...){
	wchar_t lBuf[1024];
	va_list va;
	va_start(va, pFmt);
	vswprintf_s(lBuf, pFmt, va);
	va_end(va);
	OutputDebugStringW(lBuf);
}

bool InitializeGlobals(){
	HRESULT hr = S_OK;
	size_t i, j, k;



	s_GlobalState.Accelerator = LoadAcceleratorsW(
		s_GlobalState.hInstance, MAKEINTRESOURCEW(IDR_ACCELERATOR));
	if (s_GlobalState.Accelerator == nullptr){
		DWORD err = GetLastError();
		return false;
	}

	UINT flags = D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0,
	};
	hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
		nullptr, flags, featureLevels, (UINT)GetArraySize(featureLevels),
		D3D11_SDK_VERSION, s_GlobalState.Device.GetAddress(), nullptr,
		s_GlobalState.Context.GetAddress());
	if (FAILED(hr)){
		return false;
	}
	s_GlobalState.DXGIFactory = GetFactoryAndAdapterFromDevice(
		s_GlobalState.Device, s_GlobalState.DXGIAdapter);

	if (!s_GlobalState.DXGIFactory || !s_GlobalState.DXGIAdapter){
		ClearGlobals();
		return false;
	}


	hr = s_GlobalState.Device->CreateVertexShader(VertexShader.Code,
		VertexShader.Size, nullptr, s_GlobalState.VertexShader.GetAddress());
	if (FAILED(hr)){
		ClearGlobals();
		return false;
	}


	hr = s_GlobalState.Device->CreateVertexShader(VS_Main_CheckerBoard,
		sizeof(VS_Main_CheckerBoard), nullptr,
		s_GlobalState.CheckerBoardVertexShader.GetAddress());
	if (FAILED(hr)){
		ClearGlobals();
		return false;
	}
	hr = s_GlobalState.Device->CreatePixelShader(PS_Main_CheckerBoard,
		sizeof(PS_Main_CheckerBoard), nullptr,
		s_GlobalState.CheckerBoardPixelShader.GetAddress());
	if (FAILED(hr)){
		ClearGlobals();
		return false;
	}

	for (i = 0; i < Type_Count; ++i){
		for (j = 0; j < Swap_Count; ++j){
			for (k = 0; k < Component_Count; ++k){
				const SShader &shader = PixelShaders[i][j][k];

				hr = s_GlobalState.Device->CreatePixelShader(shader.Code,
					shader.Size, nullptr,
					s_GlobalState.PixelShaders[i][j][k].GetAddress());
				if (FAILED(hr)){
					ClearGlobals();
					return false;
				}
			}
		}
	}

	D3D11_INPUT_ELEMENT_DESC inputElements[] = {
		{ "POSITION", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,
			0, 0, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT, 0,
		offsetof(SVertex, U),
		D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	hr = s_GlobalState.Device->CreateInputLayout(inputElements,
		(UINT)GetArraySize(inputElements), VertexShader.Code,
		VertexShader.Size, s_GlobalState.InputLayout.GetAddress());

	if (FAILED(hr)){
		ClearGlobals();
		return false;
	}
	const SVertex verts[4] = {
		{0.0f, 0.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f, 1.0f},
		{1.0f, 1.0f, 1.0f, 1.0f},
	};
	const uint16_t indices[6] = {
		0, 1, 2, 1, 3, 2,
	};

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = 4 * sizeof(SVertex);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA subResourceData = {
		verts,
		0,
		0,
	};

	hr = s_GlobalState.Device->CreateBuffer(&bufferDesc, &subResourceData, s_GlobalState.VertexBuffer.GetAddress());
	if (FAILED(hr)){
		ClearGlobals();
		return false;
	}
	bufferDesc.ByteWidth = 6 * sizeof(uint16_t);
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	subResourceData.pSysMem = indices;

	hr = s_GlobalState.Device->CreateBuffer(&bufferDesc, &subResourceData, s_GlobalState.IndexBuffer.GetAddress());
	if (FAILED(hr)){
		ClearGlobals();
		return false;
	}

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {
		FALSE,
		D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ZERO,
		D3D11_COMPARISON_ALWAYS,
		FALSE,
		D3D11_DEFAULT_STENCIL_READ_MASK,
		D3D11_DEFAULT_STENCIL_WRITE_MASK,
		{D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS },
		{D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS }
	};

	hr = s_GlobalState.Device->CreateDepthStencilState(&depthStencilDesc,
		s_GlobalState.DepthStencilState.GetAddress());

	if (FAILED(hr)){
		ClearGlobals();
		return false;
	}

	D3D11_SAMPLER_DESC samplerDesc = {
		D3D11_FILTER_MIN_MAG_MIP_POINT,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		0.0f,
		0,
		D3D11_COMPARISON_NEVER,
		{0.0f, 0.0f, 0.0f, 0.0f},
		0.0f,
		D3D11_FLOAT32_MAX
	};

	hr = s_GlobalState.Device->CreateSamplerState(&samplerDesc,
		s_GlobalState.SamplerState.GetAddress());
	if (FAILED(hr)){
		ClearGlobals();
		return false;
	}

	D3D11_BLEND_DESC alphaBlendDesc = {
		FALSE, FALSE,
		{
			TRUE,
			D3D11_BLEND_SRC_ALPHA,
			D3D11_BLEND_INV_SRC_ALPHA,
			D3D11_BLEND_OP_ADD,
			D3D11_BLEND_ONE,
			D3D11_BLEND_ZERO,
			D3D11_BLEND_OP_ADD,
			0xF
		}
	};

	D3D11_BLEND_DESC opaqueBlendDesc = {
		FALSE, FALSE,
		{
			TRUE,
			D3D11_BLEND_ONE,
			D3D11_BLEND_ZERO,
			D3D11_BLEND_OP_ADD,
			D3D11_BLEND_ONE,
			D3D11_BLEND_ZERO,
			D3D11_BLEND_OP_ADD,
			0xF
		}
	};
	hr = s_GlobalState.Device->CreateBlendState(&alphaBlendDesc,
		s_GlobalState.AlphaBlendState.GetAddress());
	if (FAILED(hr)){
		ClearGlobals();
		return false;
	}
	hr = s_GlobalState.Device->CreateBlendState(&opaqueBlendDesc,
		s_GlobalState.OpaqueBlendState.GetAddress());
	if (FAILED(hr)){
		ClearGlobals();
		return false;
	}

	return true;
}




static INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT uMsg,
	WPARAM wParam, LPARAM lParam){

	switch (uMsg){
	case WM_INITDIALOG:{

		SendDlgItemMessageW(hDlg, IDC_PATTERN_COLOR_A, CCM_SETCOLORS,
			(WPARAM)&GetGlobalState().ColorA, 0);
		SendDlgItemMessageW(hDlg, IDC_PATTERN_COLOR_B, CCM_SETCOLORS,
			(WPARAM)&GetGlobalState().ColorB, 0);
		SendDlgItemMessageW(hDlg, IDC_FILL_COLOR, CCM_SETCOLORS,
			(WPARAM)&GetGlobalState().FillColor, 0);
		return TRUE;
	}
	case WM_COMMAND:{
		switch (LOWORD(wParam)){
		case IDC_PATTERN_COLOR_A:{
			CCSELECTCOLORW selectColor;
			CCRGB color;
			SendDlgItemMessageW(hDlg, IDC_PATTERN_COLOR_A, CCM_GETCOLORS,
				(WPARAM)&color, 0);

			selectColor.Color = color;
			selectColor.WindowTitle = L"Select pattern color A";

			SelectColorW(&selectColor, GetGlobalState().hInstance, hDlg);
			SendDlgItemMessageW(hDlg, IDC_PATTERN_COLOR_A, CCM_SETCOLORS,
				(WPARAM)&selectColor.Color, 0);

			return TRUE;
		}
		case IDC_PATTERN_COLOR_B:{
			CCSELECTCOLORW selectColor;
			CCRGB color;
			SendDlgItemMessageW(hDlg, IDC_PATTERN_COLOR_B, CCM_GETCOLORS,
				(WPARAM)&color, 0);

			selectColor.Color = color;
			selectColor.WindowTitle = L"Select pattern color B";

			SelectColorW(&selectColor, GetGlobalState().hInstance, hDlg);
			SendDlgItemMessageW(hDlg, IDC_PATTERN_COLOR_B, CCM_SETCOLORS,
				(WPARAM)&selectColor.Color, 0);
			return TRUE;
		}
		case IDC_FILL_COLOR:{
			CCSELECTCOLORW selectColor;
			CCRGB color;
			SendDlgItemMessageW(hDlg, IDC_FILL_COLOR, CCM_GETCOLORS,
				(WPARAM)&color, 0);

			selectColor.Color = color;
			selectColor.WindowTitle = L"Select fill color";

			SelectColorW(&selectColor, GetGlobalState().hInstance, hDlg);
			SendDlgItemMessageW(hDlg, IDC_FILL_COLOR, CCM_SETCOLORS,
				(WPARAM)&selectColor.Color, 0);
			return TRUE;
		}
		case IDOK:{
			SendDlgItemMessageW(hDlg, IDC_PATTERN_COLOR_A, CCM_GETCOLORS,
				(WPARAM)&GetGlobalState().ColorA, 0);
			SendDlgItemMessageW(hDlg, IDC_PATTERN_COLOR_B, CCM_GETCOLORS,
				(WPARAM)&GetGlobalState().ColorB, 0);
			SendDlgItemMessageW(hDlg, IDC_FILL_COLOR, CCM_GETCOLORS,
				(WPARAM)&GetGlobalState().FillColor, 0);

			EndDialog(hDlg, 1);
			return TRUE;
		}
		case IDCANCEL:{
			EndDialog(hDlg, 0);
			return TRUE;
		}
		}
		return FALSE;
	}
	case WM_CLOSE:{
		EndDialog(hDlg, 0);
		return TRUE;
	}
	default:{
		return FALSE;
	}
	};
}

class CFrameWindow{
public:
	HWND	m_hWnd = nullptr;
	HWND	m_hClientWindow = nullptr;
	HWND	m_hToolBar = nullptr;
	HWND	m_hStatusBar = nullptr;
	HWND	m_hMipComboBox = nullptr;
	HWND	m_hSliceComboBox = nullptr;
	bool	m_SettingsLoaded = false;
	CFrameWindow(HWND hWnd);
	~CFrameWindow();
	void DoSettings();
	void OpenFile(const wchar_t *pPath);
	void BroadcastClientMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void RestoreDefaultUIState();
	void HandleDocumentClosing();
	void LoadSettings();
	void SaveSettings();
	void Open();
	bool HandleCommand(WPARAM wParam, LPARAM lParam);
	void OnDropFiles(HDROP hDrop);
	void SetupToolbar(HWND hToolbar);
	LRESULT WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

};


CFrameWindow::CFrameWindow(HWND hWnd) : m_hWnd(hWnd){
}

CFrameWindow::~CFrameWindow(){
}


void CFrameWindow::DoSettings(){
	INT_PTR returnValue = DialogBoxParamW(GetGlobalState().hInstance,
		MAKEINTRESOURCEW(IDD_SETTINGS), m_hWnd, SettingsDlgProc, 0);
	if (returnValue == 1){
		BroadcastClientMessage(WM_RERENDER, 0, 0);
	}
}

void CFrameWindow::OpenFile(const wchar_t *pPath){
	CBlob ddsFileData;
	if (!LoadFileToBlob(pPath, ddsFileData)){
		CStringW messageString;
		messageString.Print(L"Failed to read \"%s\".", pPath);
		MessageBoxW(m_hWnd, messageString(), L"Error", MB_ICONERROR);
		return;
	}

	CDDSImageData imageData;
	if (!LoadDDSImageData(ddsFileData, imageData)){
		CStringW messageString;
		messageString.Print(L"Failed to parse \"%s\".", pPath);
		MessageBoxW(m_hWnd, messageString(), L"Error", MB_ICONERROR);
		return;
	}

	BOOL isMaximized = FALSE;
	SendMessageW(m_hClientWindow, WM_MDIGETACTIVE, 0, (LPARAM)&isMaximized);

	if (isMaximized != FALSE){
		SendMessageW(m_hClientWindow, WM_SETREDRAW, FALSE, 0);
	}


	MDICREATESTRUCTW mdiCreateStruct = {};
	mdiCreateStruct.hOwner = GetGlobalState().hInstance;
	mdiCreateStruct.szClass = DDS_DOCUMENT_WINDOW_CLASS;
	mdiCreateStruct.szTitle = pPath;
	mdiCreateStruct.cx = mdiCreateStruct.cy =
		mdiCreateStruct.x = mdiCreateStruct.y = CW_USEDEFAULT;
	mdiCreateStruct.lParam = (LPARAM)&imageData;


	HWND hWnd = (HWND)SendMessageW(m_hClientWindow, WM_MDICREATE, 0,
		(LPARAM)&mdiCreateStruct);


	if (isMaximized != FALSE){
		if (hWnd != nullptr){
			ShowWindow(hWnd, SW_SHOWMAXIMIZED);
		}
		SendMessageW(m_hClientWindow, WM_SETREDRAW, TRUE, 0);
		RedrawWindow(m_hClientWindow, nullptr, nullptr, RDW_ALLCHILDREN | RDW_INVALIDATE);
	}
}

void CFrameWindow::BroadcastClientMessage(UINT uMsg, WPARAM wParam, LPARAM lParam){
	EnumChildWindowsT(m_hWnd, [&](HWND hChild) -> bool{
		SendMessageW(hChild, uMsg, wParam, lParam);
		return true;
	});
}

void CFrameWindow::RestoreDefaultUIState(){
	HMENU hViewMenu = GetGlobalState().hViewMenu;
	if (hViewMenu != nullptr){
		EnableMenuItem(hViewMenu, ID_SMALLER_MIP, MF_GRAYED);
		EnableMenuItem(hViewMenu, ID_LARGER_MIP, MF_GRAYED);
		EnableMenuItem(hViewMenu, ID_PREV_SLICE, MF_GRAYED);
		EnableMenuItem(hViewMenu, ID_NEXT_SLICE, MF_GRAYED);
		EnableMenuItem(hViewMenu, ID_ZOOM_IN, MF_GRAYED);
		EnableMenuItem(hViewMenu, ID_ZOOM_OUT, MF_GRAYED);
		EnableMenuItem(hViewMenu, ID_RESET_ZOOM, MF_GRAYED);
		EnableMenuItem(hViewMenu, ID_NEXT_CUBE, MF_GRAYED);
		EnableMenuItem(hViewMenu, ID_PREV_CUBE, MF_GRAYED);


		HMENU hCubeMenu = GetSubMenu(hViewMenu, 8);
		if (hCubeMenu != nullptr){
			EnableMenuItem(hViewMenu, ID_POS_X, MF_GRAYED);
			EnableMenuItem(hViewMenu, ID_NEG_X, MF_GRAYED);
			EnableMenuItem(hViewMenu, ID_POS_Y, MF_GRAYED);
			EnableMenuItem(hViewMenu, ID_NEG_Y, MF_GRAYED);
			EnableMenuItem(hViewMenu, ID_POS_Z, MF_GRAYED);
			EnableMenuItem(hViewMenu, ID_NEG_Z, MF_GRAYED);
		}


		EnableMenuItem(hViewMenu, ID_RGB, MF_GRAYED);
		EnableMenuItem(hViewMenu, ID_RGBA, MF_GRAYED);
		EnableMenuItem(hViewMenu, ID_R, MF_GRAYED);
		EnableMenuItem(hViewMenu, ID_G, MF_GRAYED);
		EnableMenuItem(hViewMenu, ID_B, MF_GRAYED);
		EnableMenuItem(hViewMenu, ID_A, MF_GRAYED);
	}
	HMENU hFileMenu = GetGlobalState().hFileMenu;

	if (hFileMenu != nullptr){
		EnableMenuItem(hFileMenu, ID_SAVE_SURFACE, MF_GRAYED);
	}
	SendMessageW(m_hMipComboBox, CB_RESETCONTENT, 0, 0);
	EnableWindow(m_hMipComboBox, FALSE);
	SendMessageW(m_hSliceComboBox, CB_RESETCONTENT, 0, 0);
	EnableWindow(m_hSliceComboBox, FALSE);

	size_t i;

	for (i = 0; i < GetArraySize(s_ToolbarCommands); ++i){
		SendMessageW(m_hToolBar, TB_ENABLEBUTTON,
			WPARAM(s_ToolbarCommands[i]), FALSE);
		SendMessageW(m_hToolBar, TB_CHECKBUTTON,
			WPARAM(s_ToolbarCommands[i]), MAKELONG(FALSE, 0));
	}
}

void CFrameWindow::HandleDocumentClosing(){
	size_t documentCount = 0;

	EnumChildWindowsT(m_hClientWindow, [&](HWND hChild) -> bool{
		++documentCount;
		return true;
	});

	if (documentCount <= 1){
		RestoreDefaultUIState();
	}
}

void CFrameWindow::LoadSettings(){

	if (!m_SettingsLoaded){
		m_SettingsLoaded = true;
	} else {
		return;
	}

	WINDOWPLACEMENT placement = {
		sizeof(WINDOWPLACEMENT)
	};
	WINDOWPLACEMENT newPlacement = {
		sizeof(WINDOWPLACEMENT)
	};
	GetWindowPlacement(m_hWnd, &placement);
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\DDSView\\", 0, KEY_READ, &hKey)
		!= ERROR_SUCCESS){
		return;
	}

	DWORD type, size;
	size = 0;
	int showCmd = placement.showCmd;

	if (RegQueryValueExW(hKey, L"PositionAndSize", 0, &type, nullptr, &size) == ERROR_SUCCESS &&
		type == REG_BINARY && size == sizeof(RECT)){
		if (RegQueryValueExW(hKey, L"PositionAndSize", 0, &type,
			(BYTE*)&newPlacement.rcNormalPosition, &size) != ERROR_SUCCESS){
			newPlacement.rcNormalPosition = placement.rcNormalPosition;
		}
	} else  {
		newPlacement.rcNormalPosition = placement.rcNormalPosition;
	}
	if (RegQueryValueExW(hKey, L"Maximized", 0, &type, nullptr, &size) == ERROR_SUCCESS &&
		type == REG_DWORD && size == sizeof(DWORD)){
		DWORD isMaximized;
		if (RegQueryValueExW(hKey, L"Maximized", 0, &type,
			(BYTE*)&isMaximized, &size) != ERROR_SUCCESS){
			newPlacement.showCmd = placement.showCmd;
		} else {
			newPlacement.showCmd = isMaximized ? SW_MAXIMIZE :
				SW_NORMAL;
		}
	}
	if (RegQueryValueExW(hKey, L"MaximizedPoint", 0, &type, nullptr, &size) == ERROR_SUCCESS &&
		type == REG_BINARY && size == sizeof(POINT)){
		if (RegQueryValueExW(hKey, L"MaximizedPoint", 0, &type,
			(BYTE*)&newPlacement.ptMaxPosition, &size) != ERROR_SUCCESS){
			newPlacement.ptMaxPosition = placement.ptMaxPosition;
		}
	} else {
		newPlacement.ptMaxPosition = placement.ptMaxPosition;
	}
	SetWindowPlacement(m_hWnd, &newPlacement);

	if (RegQueryValueExW(hKey, L"SaveFolder", 0, &type, nullptr, &size) == ERROR_SUCCESS &&
		type == REG_SZ && size > 1){

		GetGlobalState().SaveFolder.Resize((size / sizeof(wchar_t)) - 1);

		if (RegQueryValueExW(hKey, L"SaveFolder", 0, &type,
			(BYTE*)GetGlobalState().SaveFolder.GetBuffer(), &size) != ERROR_SUCCESS){
			GetGlobalState().SaveFolder = L"";
		}
	}
	if (RegQueryValueExW(hKey, L"OpenFolder", 0, &type, nullptr, &size) == ERROR_SUCCESS &&
		type == REG_SZ && size > 1){

		GetGlobalState().OpenFolder.Resize((size / sizeof(wchar_t)) - 1);

		if (RegQueryValueExW(hKey, L"OpenFolder", 0, &type,
			(BYTE*)GetGlobalState().OpenFolder.GetBuffer(), &size) != ERROR_SUCCESS){
			GetGlobalState().OpenFolder = L"";
		}
	}
	RegCloseKey(hKey);

}
void CFrameWindow::SaveSettings(){
	WINDOWPLACEMENT placement = {
		sizeof(WINDOWPLACEMENT)
	};

	if (!GetWindowPlacement(m_hWnd, &placement)){
		return;
	}
	HKEY hKey;
	if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\DDSView\\", 0, nullptr, 0,
		KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS){
		return;
	}
	LSTATUS status = RegSetValueExW(hKey, L"PositionAndSize", 0, REG_BINARY, (const BYTE*)&placement.rcNormalPosition,
		sizeof(RECT));


	DWORD isMaximized = placement.showCmd == SW_SHOWMAXIMIZED;

	RegSetValueExW(hKey, L"Maximized", 0, REG_DWORD, (const BYTE*)&isMaximized,
		sizeof(DWORD));
	RegSetValueExW(hKey, L"MaximizedPoint", 0, REG_BINARY, (const BYTE*)&placement.ptMaxPosition,
		sizeof(POINT));

	RegSetValueExW(hKey, L"SaveFolder", 0, REG_SZ, (const BYTE*)GetGlobalState().SaveFolder(),
		DWORD(GetGlobalState().SaveFolder.Size() + 1) * sizeof(wchar_t));

	RegSetValueExW(hKey, L"OpenFolder", 0, REG_SZ, (const BYTE*)GetGlobalState().OpenFolder(),
		DWORD(GetGlobalState().OpenFolder.Size() + 1) * sizeof(wchar_t));




	RegCloseKey(hKey);
}
void CFrameWindow::Open(){
	TRefCountHandle<IFileOpenDialog> openFileDialog;
	HRESULT hr;

	hr = CoCreateInstance(CLSID_FileOpenDialog,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(openFileDialog.GetAddress()));

	if (FAILED(hr)){
		return;
	}

	FILEOPENDIALOGOPTIONS options;
	hr = openFileDialog->GetOptions(&options);
	if (FAILED(hr)){
		return;
	}
	options |= FOS_FORCEFILESYSTEM
		//| FOS_ALLOWMULTISELECT
		| FOS_FILEMUSTEXIST
		| FOS_PATHMUSTEXIST;

	hr = openFileDialog->SetOptions(options);
	if (FAILED(hr)){
		return;
	}
	COMDLG_FILTERSPEC filterSpec = {
		L"DDS image file",
		L"*.dds"
	};
	hr = openFileDialog->SetFileTypes(1, &filterSpec);
	if (FAILED(hr)){
		return;
	}
	hr = openFileDialog->SetFileTypeIndex(0);
	if (FAILED(hr)){
		return;
	}
	hr = openFileDialog->SetDefaultExtension(L"dds");
	if (FAILED(hr)){
		return;
	}

	TRefCountHandle<IShellItem> folderItem;
	if (GetGlobalState().OpenFolder.Length() > 0){
		hr = SHCreateItemFromParsingName(GetGlobalState().OpenFolder(),
			nullptr, IID_IShellItem, (void**)folderItem.GetAddress());

		if (SUCCEEDED(hr)){
			openFileDialog->SetFolder(folderItem);
		}
	}

	CFileDialogEventsGetLastFolder events;
	DWORD cookie;
	hr = openFileDialog->Advise(&events, &cookie);
	if (FAILED(hr)){
		return;
	}


	hr = openFileDialog->Show(m_hWnd);
	if (FAILED(hr)){
		return;
	}
	TRefCountHandle<IShellItemArray> results;
	hr = openFileDialog->GetResults(results.GetAddress());
	if (FAILED(hr)){
		return;
	}
	DWORD itemCount, i;
	hr = results->GetCount(&itemCount);
	if (FAILED(hr)){
		return;
	}
	for (i = 0; i < itemCount; ++i){
		TRefCountHandle<IShellItem> item;
		hr = results->GetItemAt(i, item.GetAddress());
		if (SUCCEEDED(hr)){
			LPWSTR pFileName;
			hr = item->GetDisplayName(SIGDN_FILESYSPATH, &pFileName);
			if (SUCCEEDED(hr)){
				OpenFile(pFileName);
				CoTaskMemFree(pFileName);
			}
		}
	}
	if (events.GetLastFolder().Size() > 0){
		GetGlobalState().OpenFolder = events.GetLastFolder();
	}

}
bool CFrameWindow::HandleCommand(WPARAM wParam, LPARAM lParam){

	if (m_hClientWindow != nullptr){
		HWND hActiveClient = (HWND)SendMessageW(m_hClientWindow, WM_MDIGETACTIVE, 0, 0);
		if (hActiveClient != nullptr){
			LRESULT result = SendMessageW(hActiveClient, WM_COMMAND, wParam, lParam);
			if (result == 0){
				return true;
			}
		}
	}

	switch (LOWORD(wParam)){
	case ID_OPEN:{
		Open();
		return true;
	}
	case ID_SETTINGS:{
		DoSettings();
		return true;;
	}
	case ID_CHECKERBOARD_PATTERN:{
		GetGlobalState().CheckerBoard = !GetGlobalState().CheckerBoard;
		CheckMenuItem(GetGlobalState().hViewMenu, ID_CHECKERBOARD_PATTERN,
			(GetGlobalState().CheckerBoard ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
		BroadcastClientMessage(WM_RERENDER, 0, 0);
		return true;
	}
	}
	return false;
}

void CFrameWindow::OnDropFiles(HDROP hDrop){
	UINT fileCount, i;
	fileCount = DragQueryFileW(hDrop, 0xFFFF'FFFF, nullptr, 0);
	wchar_t localBuf[MAX_PATH + 1];
	wchar_t* pBuffer = localBuf;

	UINT bufferSize = MAX_PATH + 1;

	for (i = 0; i < fileCount; ++i){
		UINT sizeRequired = DragQueryFileW(hDrop, i, nullptr, 0) + 1;
		if (sizeRequired > 1){
			if (sizeRequired > bufferSize){
				if (pBuffer != localBuf){
					free(pBuffer);
				}
				pBuffer = (wchar_t*)malloc(sizeof(wchar_t) * sizeRequired);
				if (pBuffer == nullptr){
					break;
				}
				bufferSize = sizeRequired;
			}
			if (DragQueryFileW(hDrop, i, pBuffer, bufferSize) != 0){
				OpenFile(pBuffer);
			}
		}
	}
	if (pBuffer != localBuf){
		free(pBuffer);
	}
	DragFinish(hDrop);
}

void CFrameWindow::SetupToolbar(HWND hToolbar){

	const size_t buttonCount = size_t(eToolbarButton::Count) +
		size_t(eToolbarComboBox::Count);
	const size_t firstComboBoxIndex = size_t(eToolbarButton::Count);

	TBBUTTON buttons[buttonCount] = {};


	size_t i;
	for (i = 0; i < size_t(eToolbarButton::Count); ++i){
		TBBUTTON &button = buttons[i];
		button.iBitmap = int32_t(i);
		button.idCommand = s_ToolbarCommands[i];
		button.fsState = 0;
		button.fsStyle = BTNS_CHECK;
	}
	for (i = 0; i < size_t(eToolbarComboBox::Count); ++i){
		TBBUTTON &button = buttons[i + firstComboBoxIndex];
		button.iBitmap = 150;
		button.idCommand = 0;
		button.fsState = 0;
		button.fsStyle = BTNS_SEP;
	}

	SendMessageW(hToolbar, TB_SETIMAGELIST,
		0, (LPARAM)GetGlobalState().hImageList);

	SendMessageW(hToolbar, TB_BUTTONSTRUCTSIZE,
		(WPARAM)sizeof(TBBUTTON), 0);

	SendMessageW(hToolbar, TB_ADDBUTTONSW,
		WPARAM(buttonCount), (LPARAM)&buttons[0]);

	RECT itemRect;

	const int32_t dropHeight = 200;

	SendMessageW(hToolbar, TB_GETITEMRECT, WPARAM(firstComboBoxIndex),
		(LPARAM)&itemRect);

	itemRect.top = 1;
	itemRect.bottom = itemRect.top + dropHeight;

	HFONT hStockFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

	m_hMipComboBox = CreateWindowExW(0, WC_COMBOBOXW, nullptr,
		WS_VISIBLE | CBS_DROPDOWNLIST | WS_CHILD | WS_DISABLED | WS_VSCROLL,
		itemRect.left + 2, itemRect.top,
		itemRect.right - itemRect.left - 2, itemRect.bottom - itemRect.top,
		m_hWnd, HMENU(ID_MIP_COMBO_BOX), nullptr, nullptr);

	if (m_hMipComboBox != nullptr){
		SetParent(m_hMipComboBox, hToolbar);
		SendMessageW(m_hMipComboBox, WM_SETFONT, WPARAM(hStockFont),
			MAKELPARAM(FALSE, 0));
	}


	SendMessageW(hToolbar, TB_GETITEMRECT, WPARAM(firstComboBoxIndex + 1),
		(LPARAM)&itemRect);

	itemRect.top = 1;
	itemRect.bottom = itemRect.top + dropHeight;

	m_hSliceComboBox = CreateWindowExW(0, WC_COMBOBOXW, nullptr,
		WS_VISIBLE | CBS_DROPDOWNLIST | WS_CHILD | WS_DISABLED | WS_VSCROLL,
		itemRect.left + 2, itemRect.top,
		itemRect.right - itemRect.left - 2, itemRect.bottom - itemRect.top,
		m_hWnd, HMENU(ID_SLICE_COMBO_BOX), nullptr, nullptr);

	if (m_hSliceComboBox != nullptr){
		SetParent(m_hSliceComboBox, hToolbar);
		SendMessageW(m_hSliceComboBox, WM_SETFONT, WPARAM(hStockFont),
			MAKELPARAM(FALSE, 0));
	}
}

LRESULT CFrameWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch (uMsg){
	case WM_CREATE:{
		CLIENTCREATESTRUCT clientCreateStruct = {};

		HMENU hMenu = GetMenu(hWnd);
		clientCreateStruct.hWindowMenu = GetSubMenu(GetMenu(hWnd), 2);
		clientCreateStruct.idFirstChild = FIRST_MDI_MENU_ID;
		m_hClientWindow = CreateWindowExW(0, L"mdiclient", L"", WS_CHILD | WS_CLIPCHILDREN |
			WS_VSCROLL | WS_HSCROLL | WS_VISIBLE, 0, 0, 0, 0, hWnd, nullptr,
			GetGlobalState().hInstance, &clientCreateStruct);

		m_hToolBar = CreateWindowExW(0, TOOLBARCLASSNAMEW, nullptr,
			WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS, 0, 0, 0, 0, hWnd,
			nullptr, nullptr, nullptr);

		SetupToolbar(m_hToolBar);

		m_hStatusBar = CreateWindowExW(0, STATUSCLASSNAMEW, nullptr,
			WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, nullptr, nullptr, nullptr);

		if (m_hStatusBar){
			int widths[8] = {
				250, //Format
				120, //Dimension
				60, //Mip
				70, //Slice
				150, //Size
				40, //Channel
				40, //Face
				-1, //Zoom
			};


			size_t i;
			for (i = 1; i < 8; ++i){
				if (widths[i] != -1){
					widths[i] += widths[i - 1];
				}
			}
			SendMessageW(m_hStatusBar, SB_SETPARTS, 8, (LPARAM)&widths[0]);

		}


		if (hMenu == nullptr) {
			return -1;
		}
		GetGlobalState().hFileMenu = GetSubMenu(hMenu, 0);
		GetGlobalState().hViewMenu = GetSubMenu(hMenu, 1);

		DragAcceptFiles(hWnd, TRUE);
		bool success = (GetGlobalState().hFileMenu != nullptr &&
			GetGlobalState().hViewMenu != nullptr &&
			m_hClientWindow != nullptr &&
			m_hToolBar != nullptr &&
			m_hStatusBar != nullptr &&
			m_hSliceComboBox != nullptr &&
			m_hMipComboBox != nullptr);

		if (success){
			//LoadSettings();
			return 0;
		} else {
			return -1;
		}

		break;
	}
	case WM_SHOWWINDOW:{
		LoadSettings();
		return 0;
	}
	case WM_DROPFILES:{
		OnDropFiles((HDROP)wParam);
		return 0;
		break;
	}
	case WM_COMMAND:{
		if (HandleCommand(wParam, lParam)){
			return 0;
		}
		return DefFrameProcW(hWnd, m_hClientWindow, uMsg, wParam, lParam);
	}
	case WM_CLOSE:{
		DestroyWindow(hWnd);
		break;
	}
	case WM_DESTROY:{
		SaveSettings();
		PostQuitMessage(0);
		break;
	}
	case WM_SIZE:{
		RECT r = {};
		GetWindowRgnBox(hWnd, &r);
		RECT clientRect;
		GetClientRect(m_hWnd, &clientRect);
		RECT toolbarRect;
		RECT statusBarRect;
		GetWindowRect(m_hToolBar, &toolbarRect);
		GetWindowRect(m_hStatusBar, &statusBarRect);

		SetWindowPos(m_hClientWindow, nullptr, 0, toolbarRect.bottom - toolbarRect.top,
			clientRect.right - clientRect.left,
			(clientRect.bottom - clientRect.top) - (statusBarRect.bottom - statusBarRect.top) -
			(toolbarRect.bottom - toolbarRect.top), SWP_NOZORDER);

		SetWindowPos(m_hToolBar, nullptr, 0, 0,
			clientRect.right - clientRect.left,
			toolbarRect.bottom - toolbarRect.top, SWP_NOZORDER);

		SetWindowPos(m_hStatusBar, nullptr, 0, (clientRect.bottom - clientRect.top) -
			(statusBarRect.bottom - statusBarRect.top),
			clientRect.right - clientRect.left,
			statusBarRect.bottom - statusBarRect.top, SWP_NOZORDER);


		break;
	}
	case WM_DOCUMENT_CLOSING:{
		HandleDocumentClosing();
		break;
	}
	case WM_GETFRAMEWINDOW:{
		return (LRESULT)this;
	}
	case WM_GETSTATUSBAR:{
		return (LRESULT)m_hStatusBar;
	}
	case WM_GETTOOLBAR:{
		return (LRESULT)m_hToolBar;
	}
	case WM_GETMIPCOMBOBOX:{
		return (LRESULT)m_hMipComboBox;
	}
	case WM_GETSLICECOMBOBOX:{
		return (LRESULT)m_hSliceComboBox;
	}
	default:{
		return DefFrameProcW(hWnd, m_hClientWindow, uMsg, wParam, lParam);
	}

	}
	return 0;
}

LRESULT CALLBACK FrameWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	CFrameWindow* pFrameWindow;
	if (uMsg == WM_CREATE){
		pFrameWindow = new CFrameWindow(hWnd);
		if (!pFrameWindow){
			return -1;
		}
		LPCREATESTRUCTW pCreateStruct = (LPCREATESTRUCTW)lParam;
		if (pCreateStruct->lpCreateParams != nullptr){
			*(CFrameWindow**)(pCreateStruct->lpCreateParams) = pFrameWindow;
		}
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pFrameWindow);
	} else {
		pFrameWindow = (CFrameWindow*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
	}
	LRESULT result = pFrameWindow ? pFrameWindow->WindowProc(hWnd, uMsg, wParam, lParam) :
		DefFrameProcW(hWnd, nullptr, uMsg, wParam, lParam);
	if (uMsg == WM_DESTROY){
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)nullptr);
		//delete pFrameWindow;
		return 0;
	}
	return result;
}



bool InitializeWindowClasses(HINSTANCE hInstance){

	INITCOMMONCONTROLSEX commonControls;

	commonControls.dwSize = sizeof(INITCOMMONCONTROLSEX);

	commonControls.dwICC = ICC_BAR_CLASSES;

	if (InitCommonControlsEx(&commonControls) == FALSE){
		return false;
	}


	WNDCLASSEXW windowClass = {};
	windowClass.cbSize = sizeof(WNDCLASSEXW);
	windowClass.lpfnWndProc = FrameWindowProc;
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
	windowClass.lpszMenuName = MAKEINTRESOURCEW(IDR_MAIN_MENU);
	windowClass.lpszClassName = DDS_FRAME_WINDOW_CLASS;
	if (!RegisterClassExW(&windowClass)){
		return false;
	}
	return true;
}

bool SetupImageList(HINSTANCE hInstance){
	HIMAGELIST hImageList = ImageList_Create(16, 16, ILC_COLOR24, 0, 1);
	if (hImageList == nullptr){
		return false;
	}
	HBITMAP hBitMap = LoadBitmapW(hInstance, MAKEINTRESOURCEW(IDB_ICONS));
	if (!hBitMap){
		ImageList_Destroy(hImageList);
		return false;
	}
	ImageList_Add(hImageList, hBitMap, nullptr);
	DeleteObject(hBitMap);
	GetGlobalState().hImageList = hImageList;
	return true;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd){


	s_GlobalState.hInstance = hInstance;

	if (!InitializeWindowClasses(hInstance) ||
		!InitializeDocumentWindowClasses(hInstance) ||
		!RegisterColorClass(hInstance) ||
		!SetupImageList(hInstance)){
		MessageBoxW(GetDesktopWindow(), L"Failed to initialize.", L"Error",
			MB_ICONERROR);
		return 0;
	}



	Gdiplus::GdiplusStartupInput input;
	ULONG_PTR token;


	if (Gdiplus::GdiplusStartup(&token, &input, NULL) != Gdiplus::Ok){
		return 0;
	}


	if (!InitializeGlobals()){
		MessageBoxW(GetDesktopWindow(), L"Failed to initialize D3D11.", L"Error",
			MB_ICONERROR);
		Gdiplus::GdiplusShutdown(token);
		return 0;
	}

	CFrameWindow* pFrameWindow = nullptr;
	DWORD myStyle = WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_THICKFRAME;
	HWND hFrameWindow = CreateWindowExW(0, DDS_FRAME_WINDOW_CLASS, L"DDS View",
		WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hInstance, &pFrameWindow);



	GetGlobalState().hFrameWindow = hFrameWindow;
	BOOL returnValue;
	MSG msg;
	for (;;){
		returnValue = GetMessageW(&msg, nullptr, 0, 0);
		if (returnValue == 0 || returnValue == -1){
			break;
		}
		if (!TranslateMDISysAccel(pFrameWindow->m_hClientWindow, &msg) &&
			!TranslateAcceleratorW(pFrameWindow->m_hWnd, s_GlobalState.Accelerator, &msg)){
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
	if (pFrameWindow != nullptr){
		delete pFrameWindow;
	}

	Gdiplus::GdiplusShutdown(token);
	ClearGlobals();
	return 0;

}