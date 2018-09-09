#include "DocumentWindow.h"
#include "TGA.h"
#include "FormatInfo.h"
#include "resource.h"
#include <windowsx.h>
#include <utility>
#include "FileDialogEvents.h"

static const UINT s_CubeFaceCommandIDs[6] = {
	ID_POS_X,
	ID_NEG_X,
	ID_POS_Y,
	ID_NEG_Y,
	ID_POS_Z,
	ID_NEG_Z,
};
static const UINT s_ComponentCommandIDs[6] = {
	ID_RGBA,
	ID_RGB,
	ID_R,
	ID_G,
	ID_B,
	ID_A,
};


struct alignas(16) SShaderData{
	SVec2	ScreenSize;
	SVec2	TextureSize;
	SVec2	Offset;
	float	RangeStart;
	float	RangeEnd;
	SVec2	UVScale;
};
struct alignas(16) SCheckerBoardShaderData{
	SVec2	ScreenSize;
	float	PatternSize;
	float	Pad0;
	SVec2	Offset;
	SVec2	TextureSize;
	CCRGB	ColorA;
	float	Pad1;
	CCRGB	ColorB;
	float	Pad2;
	CCRGB	FillColor;
};



/*
CViewControl

*/

void CViewControl::SaveSurface(){
	TRefCountHandle<IFileSaveDialog> saveFileDialog;
	HRESULT hr;

	hr = CoCreateInstance(CLSID_FileSaveDialog,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(saveFileDialog.GetAddress()));

	if (FAILED(hr)){
		return;
	}

	FILEOPENDIALOGOPTIONS options;
	hr = saveFileDialog->GetOptions(&options);
	if (FAILED(hr)){
		return;
	}
	options |= FOS_FORCEFILESYSTEM
		| FOS_PATHMUSTEXIST;

	hr = saveFileDialog->SetOptions(options);
	if (FAILED(hr)){
		return;
	}
	COMDLG_FILTERSPEC filterSpec = {
		L"Targa image file",
		L"*.tga"
	};
	hr = saveFileDialog->SetFileTypes(1, &filterSpec);
	if (FAILED(hr)){
		return;
	}
	hr = saveFileDialog->SetFileTypeIndex(0);
	if (FAILED(hr)){
		return;
	}
	hr = saveFileDialog->SetDefaultExtension(L"tga");
	if (FAILED(hr)){
		return;
	}

	CStringW outputPath = m_Path;

	size_t filenamePos = outputPath.FindLastOf(L"/\\");
	if (filenamePos == CStringW::npos){
		filenamePos = 0;
	} else {
		++filenamePos;
	}
	size_t extensionPos = outputPath.FindLastOf(L'.');
	if (extensionPos > filenamePos){
		outputPath.Erase(extensionPos);
	}



	wchar_t lBuf[32];
	if (m_CurrentMipLevel > 0){
		swprintf_s(lBuf, L"_Mip_%u", m_CurrentMipLevel);
		outputPath.Append(lBuf);
	}
	if (m_ImageData.ArraySliceCount > 1 ||
		m_ImageData.Depth > 1){
		swprintf_s(lBuf, L"_Slice_%u", m_CurrentSlice);
		outputPath.Append(lBuf);
	}
	const wchar_t* faceNames[] = {
		L"X+",
		L"X-",
		L"Y+",
		L"Y-",
		L"Z+",
		L"Z-"
	};

	if (m_ImageData.TextureType == eTextureType::_Cube){
		swprintf_s(lBuf, L"_%s", faceNames[m_CurrentCubeFace]);
		outputPath.Append(lBuf);
	}
	outputPath.Append(L".tga");

	TRefCountHandle<IShellItem> folderItem;

	CGlobalState &globalState = GetGlobalState();

	if (globalState.SaveFolder.Length() > 0){
		hr = SHCreateItemFromParsingName(globalState.SaveFolder(),
			nullptr, IID_IShellItem, (void**)folderItem.GetAddress());

		if (SUCCEEDED(hr)){
			saveFileDialog->SetFolder(folderItem);
		}
	}

	hr = saveFileDialog->SetFileName(outputPath());
	if (FAILED(hr)){
		return;
	}


	CFileDialogEventsGetLastFolder events;
	DWORD cookie;
	hr = saveFileDialog->Advise(&events, &cookie);
	if (FAILED(hr)){
		return;
	}

	hr = saveFileDialog->Show(m_hWnd);
	if (FAILED(hr)){
		return;
	}

	TRefCountHandle<IShellItem> result;
	hr = saveFileDialog->GetResult(result.GetAddress());
	if (FAILED(hr)){
		return;
	}


	LPWSTR pFileName;
	hr = result->GetDisplayName(SIGDN_FILESYSPATH, &pFileName);
	if (FAILED(hr)){
		return;
	}
	SaveSurfaceToFile(pFileName);
	CoTaskMemFree(pFileName);
	if (events.GetLastFolder().Size() > 0){
		globalState.SaveFolder = events.GetLastFolder();
	}
}
void CViewControl::SaveSurfaceToFile(const wchar_t* pPath){
	UpdateTexture();
	TRefCountHandle<ID3D11RenderTargetView> renderTargetView;
	TRefCountHandle<ID3D11Texture2D> renderTarget;
	TRefCountHandle<ID3D11Texture2D> stagingTexture;

	const SDDSMipInfo &mipInfo = m_ImageData.MipInfos[m_CurrentMipLevel];

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Width = mipInfo.Width;
	textureDesc.Height = mipInfo.Height;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
	textureDesc.ArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;

	HRESULT hr;
	CGlobalState &globalState = GetGlobalState();

	hr = globalState.Device->CreateTexture2D(&textureDesc, nullptr,
		renderTarget.GetAddress());
	if (FAILED(hr)){
		return;
	}

	hr = globalState.Device->CreateRenderTargetView(renderTarget, nullptr,
		renderTargetView.GetAddress());
	if (FAILED(hr)){
		return;
	}

	textureDesc.BindFlags = 0;
	textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	textureDesc.Usage = D3D11_USAGE_STAGING;


	hr = globalState.Device->CreateTexture2D(&textureDesc, nullptr,
		stagingTexture.GetAddress());
	if (FAILED(hr)){
		return;
	}

	const uint32_t width = mipInfo.Width;
	const uint32_t height = mipInfo.Height;

	SShaderData data = {
		{float(width), float(height)},
		{float(width),
		float(height)},
		-float(0), -float(0),
		0.0f, 0.0f,
		m_UVScale
	};
	globalState.Context->UpdateSubresource(m_ShaderParams, 0, nullptr, &data, 0, 0);
	globalState.Context->IASetIndexBuffer(globalState.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	UINT strides[1] = { sizeof(SVertex) };
	UINT offsets[1] = { 0 };
	ID3D11Buffer* buffers[1] = { globalState.VertexBuffer };
	ID3D11Buffer* constantBuffers[1] = { m_ShaderParams };
	globalState.Context->IASetVertexBuffers(0, 1, buffers, strides, offsets);
	globalState.Context->VSSetShader(globalState.VertexShader, nullptr, 0);
	globalState.Context->IASetInputLayout(globalState.InputLayout);
	globalState.Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	globalState.Context->PSSetShader(
		globalState.PixelShaders[Type_UNorm][Swap_None][Component_RGBA], nullptr, 0);
	globalState.Context->PSSetConstantBuffers(0, 1, constantBuffers);
	globalState.Context->VSSetConstantBuffers(0, 1, constantBuffers);

	ID3D11RenderTargetView* renderTargetViews[1] = { renderTargetView };

	ID3D11SamplerState* samplerStates[1] = { globalState.SamplerState };

	globalState.Context->PSSetSamplers(0, 1, samplerStates);
	globalState.Context->OMSetRenderTargets(1, renderTargetViews, nullptr);
	D3D11_VIEWPORT viewPort = {
		0.0f, 0.0f,
		float(width),
		float(height),
		0.0f, 1.0f,
	};
	globalState.Context->RSSetViewports(1, &viewPort);
	ID3D11ShaderResourceView* shaderResourceViews[1] = { m_ShaderResourceView };
	globalState.Context->PSSetShaderResources(0, 1, shaderResourceViews);
	globalState.Context->DrawIndexed(6, 0, 0);

	renderTargetViews[0] = nullptr;
	globalState.Context->OMSetRenderTargets(1, renderTargetViews, nullptr);
	globalState.Context->CopyResource(stagingTexture, renderTarget);

	D3D11_MAPPED_SUBRESOURCE mappedSubresource;
	hr = globalState.Context->Map(stagingTexture, 0, D3D11_MAP_READ, 0,
		&mappedSubresource);
	if (FAILED(hr)){
		return;
	}

	CBlob tgaData;
	tgaData.Resize(width * height * 4 + TGAHeaderGetSize());

	STGAHeader tgaHeader = {};
	tgaHeader.ColorMapType = 0;
	tgaHeader.ImageType = 2;
	tgaHeader.ImageSpec.XOrigin = 0;
	tgaHeader.ImageSpec.YOrigin = 0;
	tgaHeader.ImageSpec.Width = (uint16_t)width;
	tgaHeader.ImageSpec.Height = (uint16_t)height;
	tgaHeader.ImageSpec.BitsPerPixel = 32;

	TGAHeaderEncode(tgaData.GetPointer(), tgaData.Size(), tgaHeader);

	unsigned char* pDest = (unsigned char*)tgaData.GetPointer() + TGAHeaderGetSize();

	const unsigned char* pSrc = (const unsigned char*)mappedSubresource.pData;
	uint32_t y, x;
	for (y = 0; y < height; ++y){
		const unsigned char* pSrcRow = pSrc + y * mappedSubresource.RowPitch;
		unsigned char* pDestRow = pDest + (height - 1 - y) * width * 4;
		for (x = 0; x < width; ++x){
			pDestRow[2 + x * 4] = pSrcRow[0 + x * 4];
			pDestRow[1 + x * 4] = pSrcRow[1 + x * 4];
			pDestRow[0 + x * 4] = pSrcRow[2 + x * 4];
			pDestRow[3 + x * 4] = pSrcRow[3 + x * 4];
		}
	}

	WriteBlobToFile(pPath, tgaData);
	globalState.Context->Unmap(stagingTexture, 0);
}

void CViewControl::Init(){
	uint32_t supportedChannels = Format_GetSupportedChannel(m_ImageData.Format);
	if ((supportedChannels & (CCF_R | CCF_G | CCF_B)) != 0){
		m_CurrentChannel = Component_RGB;
	} else if ((supportedChannels & CCF_A) != 0){
		m_CurrentChannel = Component_A;
	}
	UpdateScrollBars();
}

HMENU CViewControl::GetViewMenu(){
	return GetGlobalState().hViewMenu;
}
HMENU CViewControl::GetFileMenu(){
	return GetGlobalState().hFileMenu;
}
void CViewControl::SetSwapChainDirty(){
	m_SwapChainDirty = true;
	m_TextureDirty = true;
	m_RenderDirty = true;
	InvalidateRect(m_hWnd, nullptr, FALSE);
	UpdateWindow(m_hWnd);
}
void CViewControl::SetRenderDirty(){
	m_RenderDirty = true;
	InvalidateRect(m_hWnd, nullptr, FALSE);
	UpdateWindow(m_hWnd);
}
void CViewControl::SetTextureDirty(){
	m_TextureDirty = true;
	m_RenderDirty = true;
	InvalidateRect(m_hWnd, nullptr, FALSE);
	UpdateWindow(m_hWnd);
}
void CViewControl::SelectRGBA(){
	uint32_t supportedChannels = Format_GetSupportedChannel(m_ImageData.Format);
	if ((supportedChannels & (CCF_R | CCF_G | CCF_B)) != 0 &&
		(supportedChannels & CCF_A) != 0){
		m_CurrentChannel = Component_RGBA;
		SetRenderDirty();
		HMENU hViewMenu = GetViewMenu();
		if (hViewMenu){
			CheckMenuRadioItem(hViewMenu, ID_RGBA, ID_A, s_ComponentCommandIDs[m_CurrentChannel],
				MF_BYCOMMAND);
		}
		HWND hToolbar = GetToolbar();
		if (hToolbar != nullptr){
			ToolbarUpdateChannel(hToolbar);
		}
		UpdateStatusBar();
	}
}
void CViewControl::SelectRGB(){
	uint32_t supportedChannels = Format_GetSupportedChannel(m_ImageData.Format);
	if ((supportedChannels & (CCF_R | CCF_G | CCF_B)) != 0){
		m_CurrentChannel = Component_RGB;
		SetRenderDirty();
		HMENU hViewMenu = GetViewMenu();
		if (hViewMenu){
			CheckMenuRadioItem(hViewMenu, ID_RGBA, ID_A, s_ComponentCommandIDs[m_CurrentChannel],
				MF_BYCOMMAND);
		}
		HWND hToolbar = GetToolbar();
		if (hToolbar != nullptr){
			ToolbarUpdateChannel(hToolbar);
		}
		UpdateStatusBar();
	}
}
void CViewControl::SelectR(){
	uint32_t supportedChannels = Format_GetSupportedChannel(m_ImageData.Format);
	if ((supportedChannels & (CCF_R)) != 0){
		m_CurrentChannel = Component_R;
		SetRenderDirty();
		HMENU hViewMenu = GetViewMenu();
		if (hViewMenu){
			CheckMenuRadioItem(hViewMenu, ID_RGBA, ID_A, s_ComponentCommandIDs[m_CurrentChannel],
				MF_BYCOMMAND);
		}
		HWND hToolbar = GetToolbar();
		if (hToolbar != nullptr){
			ToolbarUpdateChannel(hToolbar);
		}
		UpdateStatusBar();
	}
}
void CViewControl::SelectG(){
	uint32_t supportedChannels = Format_GetSupportedChannel(m_ImageData.Format);
	if ((supportedChannels & (CCF_G)) != 0){
		m_CurrentChannel = Component_G;
		SetRenderDirty();
		HMENU hViewMenu = GetViewMenu();
		if (hViewMenu){
			CheckMenuRadioItem(hViewMenu, ID_RGBA, ID_A, s_ComponentCommandIDs[m_CurrentChannel],
				MF_BYCOMMAND);
		}
		HWND hToolbar = GetToolbar();
		if (hToolbar != nullptr){
			ToolbarUpdateChannel(hToolbar);
		}
		UpdateStatusBar();
	}
}
void CViewControl::SelectB(){
	uint32_t supportedChannels = Format_GetSupportedChannel(m_ImageData.Format);
	if ((supportedChannels & (CCF_B)) != 0){
		m_CurrentChannel = Component_B;
		SetRenderDirty();
		HMENU hViewMenu = GetViewMenu();
		if (hViewMenu){
			CheckMenuRadioItem(hViewMenu, ID_RGBA, ID_A, s_ComponentCommandIDs[m_CurrentChannel],
				MF_BYCOMMAND);
		}
		HWND hToolbar = GetToolbar();
		if (hToolbar != nullptr){
			ToolbarUpdateChannel(hToolbar);
		}
		UpdateStatusBar();
	}
}
void CViewControl::SelectA(){
	uint32_t supportedChannels = Format_GetSupportedChannel(m_ImageData.Format);
	if ((supportedChannels & (CCF_A)) != 0){
		m_CurrentChannel = Component_A;
		SetRenderDirty();
		HMENU hViewMenu = GetViewMenu();
		if (hViewMenu){
			CheckMenuRadioItem(hViewMenu, ID_RGBA, ID_A, s_ComponentCommandIDs[m_CurrentChannel],
				MF_BYCOMMAND);
		}
		HWND hToolbar = GetToolbar();
		if (hToolbar != nullptr){
			ToolbarUpdateChannel(hToolbar);
		}
		UpdateStatusBar();
	}
}

void CViewControl::UpdateStatusBar(){
	wchar_t lBuf[256];
	HWND hStatusBar = (HWND)SendMessageW(GetGlobalState().hFrameWindow, WM_GETSTATUSBAR, 0, 0);

	swprintf_s(lBuf, L"%S", "Format");
	SendMessageW(hStatusBar, SB_SETTEXT, 0, (LPARAM)lBuf);

	const SDDSMipInfo &mipInfo = m_ImageData.MipInfos[m_CurrentMipLevel];

	if (m_ImageData.TextureType == eTextureType::_1D){
		swprintf_s(lBuf, L"1D - %u", mipInfo.Width);
	} else if (m_ImageData.TextureType == eTextureType::_2D){
		swprintf_s(lBuf, L"2D - %ux%u", mipInfo.Width, mipInfo.Height);
	} else if (m_ImageData.TextureType == eTextureType::_3D){
		swprintf_s(lBuf, L"3D - %ux%ux%u", mipInfo.Width, mipInfo.Height, mipInfo.Depth);
	} else if (m_ImageData.TextureType == eTextureType::_Cube){
		swprintf_s(lBuf, L"Cube - %ux%u", mipInfo.Width, mipInfo.Height);
	}
	SendMessageW(hStatusBar, SB_SETTEXT, 1, (LPARAM)lBuf);

	swprintf_s(lBuf, L"Mip %u/%u", m_CurrentMipLevel + 1, m_ImageData.MipCount);
	SendMessageW(hStatusBar, SB_SETTEXT, 2, (LPARAM)lBuf);

	if (m_ImageData.ArraySliceCount != 1){
		swprintf_s(lBuf, L"Slice %u/%u", m_CurrentSlice + 1, m_ImageData.ArraySliceCount);
		SendMessageW(hStatusBar, SB_SETTEXT, 3, (LPARAM)lBuf);
	} else {
		SendMessageW(hStatusBar, SB_SETTEXT, 3, (LPARAM)L"-");
	}

	swprintf_s(lBuf, L"%zu bytes", mipInfo.Size);
	SendMessageW(hStatusBar, SB_SETTEXT, 4, (LPARAM)lBuf);

	const wchar_t* channelNames[] = {
		L"RGBA",
		L"RGB",
		L"R",
		L"G",
		L"B",
		L"A",
	};

	SendMessageW(hStatusBar, SB_SETTEXT, 5, (LPARAM)channelNames[m_CurrentChannel]);



	const wchar_t* faceNames[] = {
		L"X+",
		L"X-",
		L"Y+",
		L"Y-",
		L"Z+",
		L"Z-"
	};

	if (m_ImageData.TextureType == eTextureType::_Cube){
		SendMessageW(hStatusBar, SB_SETTEXT, 6, (LPARAM)faceNames[m_CurrentCubeFace]);
	} else {
		SendMessageW(hStatusBar, SB_SETTEXT, 6, (LPARAM)L"-");
	}

	if (m_ZoomFactor < 0){
		swprintf_s(lBuf, L"%.3f %%", 100.0 / double(1 << (-m_ZoomFactor)));
	} else {
		swprintf_s(lBuf, L"%.0f %%", 100.0 * double(1 << (m_ZoomFactor)));
	}
	SendMessageW(hStatusBar, SB_SETTEXT, 7, (LPARAM)lBuf);



}
HWND CViewControl::GetToolbar(){
	return (HWND)SendMessageW(GetGlobalState().hFrameWindow,
		WM_GETTOOLBAR, 0, 0);
}
HWND CViewControl::GetMipComboBox(){
	return (HWND)SendMessageW(GetGlobalState().hFrameWindow,
		WM_GETMIPCOMBOBOX, 0, 0);
}
HWND CViewControl::GetSliceComboBox(){
	return (HWND)SendMessageW(GetGlobalState().hFrameWindow,
		WM_GETSLICECOMBOBOX, 0, 0);
}
void CViewControl::ToolbarUpdateChannel(HWND hToolbar){
	size_t i;


	uint32_t flags = Format_GetSupportedChannel(m_ImageData.Format);
	SendMessageW(hToolbar, TB_ENABLEBUTTON, ID_R, (flags & CCF_R) ? TRUE : FALSE);
	SendMessageW(hToolbar, TB_ENABLEBUTTON, ID_G, (flags & CCF_G) ? TRUE : FALSE);
	SendMessageW(hToolbar, TB_ENABLEBUTTON, ID_B, (flags & CCF_B) ? TRUE : FALSE);
	SendMessageW(hToolbar, TB_ENABLEBUTTON, ID_A, (flags & CCF_A) ? TRUE : FALSE);
	SendMessageW(hToolbar, TB_ENABLEBUTTON, ID_RGB, (flags & (CCF_R | CCF_G | CCF_B)) ? TRUE : FALSE);
	SendMessageW(hToolbar, TB_ENABLEBUTTON, ID_RGBA, (flags & (CCF_R | CCF_G | CCF_B)) != 0 &&
		(flags & CCF_A) != 0 ? TRUE : FALSE);

	for (i = size_t(eToolbarButton::RGB); i <= size_t(eToolbarButton::A); ++i){
		const size_t channelIndex = i - size_t(eToolbarButton::RGB);
		if (channelIndex == m_CurrentChannel){
			SendMessageW(hToolbar, TB_CHECKBUTTON,
				WPARAM(s_ComponentCommandIDs[channelIndex]), MAKELONG(TRUE, 0));
		} else {
			SendMessageW(hToolbar, TB_CHECKBUTTON,
				WPARAM(s_ComponentCommandIDs[channelIndex]), MAKELONG(FALSE, 0));
		}
	}
}
void CViewControl::ToolbarUpdateCubeFace(HWND hToolbar){
	size_t i;


	uint32_t flags = Format_GetSupportedChannel(m_ImageData.Format);
	BOOL enableCubeButtons = m_ImageData.TextureType == eTextureType::_Cube ?
		TRUE : FALSE;

	SendMessageW(hToolbar, TB_ENABLEBUTTON, ID_POS_X, enableCubeButtons);
	SendMessageW(hToolbar, TB_ENABLEBUTTON, ID_NEG_X, enableCubeButtons);
	SendMessageW(hToolbar, TB_ENABLEBUTTON, ID_POS_Y, enableCubeButtons);
	SendMessageW(hToolbar, TB_ENABLEBUTTON, ID_NEG_Y, enableCubeButtons);
	SendMessageW(hToolbar, TB_ENABLEBUTTON, ID_POS_Z, enableCubeButtons);
	SendMessageW(hToolbar, TB_ENABLEBUTTON, ID_NEG_Z, enableCubeButtons);


	for (i = size_t(eToolbarButton::PositiveX); i <= size_t(eToolbarButton::NegativeZ); ++i){
		const size_t faceIndex = i - size_t(eToolbarButton::PositiveX);
		if (faceIndex == m_CurrentCubeFace){
			SendMessageW(hToolbar, TB_CHECKBUTTON,
				WPARAM(s_CubeFaceCommandIDs[faceIndex]), MAKELONG(TRUE, 0));
		} else {
			SendMessageW(hToolbar, TB_CHECKBUTTON,
				WPARAM(s_CubeFaceCommandIDs[faceIndex]), MAKELONG(FALSE, 0));
		}
	}
}
void CViewControl::UpdateToolbar(){
	HWND hToolbar = GetToolbar();
	if (hToolbar != nullptr){
		ToolbarUpdateChannel(hToolbar);
		ToolbarUpdateCubeFace(hToolbar);
	}
}
void CViewControl::UpdateSliceComboBoxContents(){
	HWND hComboBox = GetSliceComboBox();
	if (hComboBox == nullptr){
		return;
	}
	wchar_t itemBuffer[64];
	SendMessageW(hComboBox, CB_RESETCONTENT, 0, 0);

	if (m_ImageData.TextureType == eTextureType::_3D){
		EnableWindow(hComboBox, TRUE);
		uint32_t depth = m_ImageData.MipInfos[m_CurrentMipLevel].Depth;
		uint32_t i;

		for (i = 0; i < depth; ++i){
			swprintf_s(itemBuffer, L"Depth slice %u", i);
			SendMessageW(hComboBox, CB_ADDSTRING, 0, LPARAM(itemBuffer));
		}
		SendMessageW(hComboBox, CB_SETCURSEL, WPARAM(m_CurrentSlice), 0);
	} else if (m_ImageData.ArraySliceCount > 1){
		EnableWindow(hComboBox, TRUE);
		uint32_t sliceCount = m_ImageData.ArraySliceCount;
		uint32_t i;

		for (i = 0; i < sliceCount; ++i){
			swprintf_s(itemBuffer, L"Array slice %u", i);
			SendMessageW(hComboBox, CB_ADDSTRING, 0, LPARAM(itemBuffer));
		}
		SendMessageW(hComboBox, CB_SETCURSEL, WPARAM(m_CurrentSlice), 0);
	} else {
		EnableWindow(hComboBox, FALSE);
	}
}
void CViewControl::UpdateSliceComboBoxSelection(){
	HWND hComboBox = GetSliceComboBox();
	if (hComboBox == nullptr){
		return;
	}

	if (m_ImageData.TextureType == eTextureType::_3D){
		SendMessageW(hComboBox, CB_SETCURSEL, WPARAM(m_CurrentSlice), 0);
	} else if (m_ImageData.ArraySliceCount > 1){
		SendMessageW(hComboBox, CB_SETCURSEL, WPARAM(m_CurrentSlice), 0);
	}
}
void CViewControl::UpdateMipComboBoxContents(){
	HWND hComboBox = GetMipComboBox();
	if (hComboBox == nullptr){
		return;
	}
	wchar_t itemBuffer[64];
	SendMessageW(hComboBox, CB_RESETCONTENT, 0, 0);

	EnableWindow(hComboBox, m_ImageData.MipCount ? TRUE : FALSE);
	if (m_ImageData.TextureType == eTextureType::_3D){
		uint32_t i;

		for (i = 0; i < m_ImageData.MipCount; ++i){
			const SDDSMipInfo &mipInfo = m_ImageData.MipInfos[i];
			swprintf_s(itemBuffer, L"Mip %u (%ux%ux%u)", i,
				mipInfo.Width, mipInfo.Height, mipInfo.Depth);
			SendMessageW(hComboBox, CB_ADDSTRING, 0, LPARAM(itemBuffer));
		}
		SendMessageW(hComboBox, CB_SETCURSEL, WPARAM(m_CurrentMipLevel), 0);
	} else if (m_ImageData.TextureType == eTextureType::_2D ||
		m_ImageData.TextureType == eTextureType::_Cube){
		uint32_t i;

		for (i = 0; i < m_ImageData.MipCount; ++i){
			const SDDSMipInfo &mipInfo = m_ImageData.MipInfos[i];
			swprintf_s(itemBuffer, L"Mip %u (%ux%u)", i,
				mipInfo.Width, mipInfo.Height);
			SendMessageW(hComboBox, CB_ADDSTRING, 0, LPARAM(itemBuffer));
		}
		SendMessageW(hComboBox, CB_SETCURSEL, WPARAM(m_CurrentMipLevel), 0);
	} else {
		uint32_t i;

		for (i = 0; i < m_ImageData.MipCount; ++i){
			const SDDSMipInfo &mipInfo = m_ImageData.MipInfos[i];
			swprintf_s(itemBuffer, L"Mip %u (%u)", i,
				mipInfo.Width);
			SendMessageW(hComboBox, CB_ADDSTRING, 0, LPARAM(itemBuffer));
		}
		SendMessageW(hComboBox, CB_SETCURSEL, WPARAM(m_CurrentMipLevel), 0);
	}
}

void CViewControl::UpdateMipComboBoxSelection(){
	HWND hComboBox = GetMipComboBox();
	if (hComboBox == nullptr){
		return;
	}

	SendMessageW(hComboBox, CB_SETCURSEL, WPARAM(m_CurrentMipLevel), 0);
	EnableWindow(hComboBox, m_ImageData.MipCount ? TRUE : FALSE);
}
void CViewControl::HandleMipSelectionChange(){
	HWND hComboBox = GetMipComboBox();
	if (hComboBox == nullptr){
		return;
	}

	int32_t selection = (int32_t)SendMessageW(hComboBox, CB_GETCURSEL, 0, 0);
	if (selection == CB_ERR ||
		selection < 0){
		return;
	}

	if (uint32_t(selection) >= m_ImageData.MipCount){
		return;
	}

	uint32_t newMipLevel = uint32_t(selection);
	if (newMipLevel == m_CurrentMipLevel){
		return;
	}

	if (m_ImageData.TextureType == eTextureType::_3D){
		if (newMipLevel < m_CurrentMipLevel){
			if (m_ImageData.MipInfos[m_CurrentMipLevel].Depth - 1 == m_CurrentSlice){
				m_CurrentSlice = m_ImageData.MipInfos[m_CurrentMipLevel - 1].Depth - 1;
			} else {
				m_CurrentSlice <<= m_CurrentMipLevel - newMipLevel;
			}
		} else{
			m_CurrentSlice >>= newMipLevel - m_CurrentMipLevel;
		}
	}

	m_CurrentMipLevel = newMipLevel;

	SetTextureDirty();
	UpdateStatusBar();
	if (m_ImageData.TextureType == eTextureType::_3D){
		UpdateSliceComboBoxContents();
	}

}
void CViewControl::HandleSliceSelectionChange(){
	HWND hComboBox = GetSliceComboBox();
	const SDDSMipInfo &mipInfo = m_ImageData.MipInfos[m_CurrentMipLevel];
	if (hComboBox == nullptr){
		return;
	}

	int32_t selection = (int32_t)SendMessageW(hComboBox, CB_GETCURSEL, 0, 0);
	if (selection == CB_ERR ||
		selection < 0){
		return;
	}

	uint32_t newSlice = uint32_t(selection);

	if (newSlice == m_CurrentSlice){
		return;
	}

	if (m_ImageData.TextureType == eTextureType::_3D){
		if (newSlice >= mipInfo.Depth){
			return;
		}
	} else if (m_ImageData.ArraySliceCount > 1) {
		if (newSlice >= m_ImageData.ArraySliceCount){
			return;
		}
	} else {
		return;
	}

	m_CurrentSlice = newSlice;
	SetTextureDirty();
	UpdateStatusBar();

}
void CViewControl::UpdateMenu(){
	HMENU hViewMenu = GetViewMenu();
	if (hViewMenu == nullptr){
		return;
	}
	HMENU hFileMenu = GetFileMenu();
	if (hFileMenu == nullptr){
		return;
	}
	HMENU hCubeMenu = GetSubMenu(hViewMenu, 8);
	if (hCubeMenu == nullptr){
		return;
	}
	uint32_t i;
	MENUITEMINFOW menuItemInfo = { sizeof(MENUITEMINFOW) };

	if (m_ImageData.MipCount > 1){
		EnableMenuItem(hViewMenu, ID_LARGER_MIP,
			m_CurrentMipLevel > 0 ? MF_ENABLED : MF_GRAYED);
		EnableMenuItem(hViewMenu, ID_SMALLER_MIP,
			m_CurrentMipLevel < m_ImageData.MipCount - 1 ? MF_ENABLED : MF_GRAYED);
	} else {
		EnableMenuItem(hViewMenu, ID_SMALLER_MIP, MF_GRAYED);
		EnableMenuItem(hViewMenu, ID_LARGER_MIP, MF_GRAYED);
	}
	if (m_ImageData.TextureType == eTextureType::_3D &&
		m_ImageData.MipInfos[m_CurrentMipLevel].Depth > 1){
		EnableMenuItem(hViewMenu, ID_PREV_SLICE,
			m_CurrentSlice > 0 ? MF_ENABLED : MF_GRAYED);
		EnableMenuItem(hViewMenu, ID_NEXT_SLICE,
			m_CurrentSlice < m_ImageData.Depth - 1 ? MF_ENABLED : MF_GRAYED);
	} else if (m_ImageData.TextureType != eTextureType::_3D &&
		m_ImageData.ArraySliceCount > 1){
		EnableMenuItem(hViewMenu, ID_PREV_SLICE, MF_ENABLED);
		EnableMenuItem(hViewMenu, ID_NEXT_SLICE, MF_ENABLED);
	} else {
		EnableMenuItem(hViewMenu, ID_PREV_SLICE, MF_GRAYED);
		EnableMenuItem(hViewMenu, ID_NEXT_SLICE, MF_GRAYED);
	}
	EnableMenuItem(hViewMenu, ID_ZOOM_IN, MF_ENABLED); //TODO
	EnableMenuItem(hViewMenu, ID_ZOOM_OUT, MF_ENABLED);
	if (m_ImageData.TextureType == eTextureType::_Cube){
		for (i = 0; i < 6; ++i){
			SetMenuItemInfoW(hCubeMenu, s_CubeFaceCommandIDs[i], FALSE, &menuItemInfo);
			EnableMenuItem(hCubeMenu, s_CubeFaceCommandIDs[i], MF_ENABLED);
		}
		EnableMenuItem(hViewMenu, ID_NEXT_CUBE, m_CurrentCubeFace < 5 ? MF_ENABLED : MF_GRAYED);
		EnableMenuItem(hViewMenu, ID_PREV_CUBE, m_CurrentCubeFace > 0 ? MF_ENABLED : MF_GRAYED);
		CheckMenuRadioItem(hCubeMenu, ID_POS_X, ID_NEG_Z, s_CubeFaceCommandIDs[m_CurrentCubeFace],
			MF_BYCOMMAND);
	} else {
		for (i = 0; i < 6; ++i){
			menuItemInfo.fMask = MIIM_STATE;
			GetMenuItemInfoW(hCubeMenu, s_CubeFaceCommandIDs[i], FALSE, &menuItemInfo);

			menuItemInfo.fState &= ~MFS_CHECKED;
			SetMenuItemInfoW(hCubeMenu, s_CubeFaceCommandIDs[i], FALSE, &menuItemInfo);
			EnableMenuItem(hCubeMenu, s_CubeFaceCommandIDs[i], MF_GRAYED);
		}
		EnableMenuItem(hViewMenu, ID_NEXT_CUBE, MF_GRAYED);
		EnableMenuItem(hViewMenu, ID_PREV_CUBE, MF_GRAYED);
	}
	uint32_t flags = Format_GetSupportedChannel(m_ImageData.Format);
	EnableMenuItem(hViewMenu, ID_R, (flags & CCF_R) ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hViewMenu, ID_G, (flags & CCF_G) ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hViewMenu, ID_B, (flags & CCF_B) ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hViewMenu, ID_A, (flags & CCF_A) ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hViewMenu, ID_RGB, (flags & (CCF_R | CCF_G | CCF_B)) ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hViewMenu, ID_RGBA, (flags & (CCF_R | CCF_G | CCF_B)) != 0 &&
		(flags & CCF_A) != 0 ? MF_ENABLED : MF_GRAYED);

	CheckMenuRadioItem(hViewMenu, ID_RGBA, ID_A, s_ComponentCommandIDs[m_CurrentChannel],
		MF_BYCOMMAND);
	EnableMenuItem(hFileMenu, ID_SAVE_SURFACE, MF_ENABLED);
	UpdateStatusBar();
}

void CViewControl::SelectPrevCubeFace(){
	if (m_ImageData.TextureType != eTextureType::_Cube ||
		m_CurrentCubeFace == 0){
		return;
	}
	HMENU hViewMenu = GetViewMenu();
	if (hViewMenu == nullptr){
		return;
	}
	HMENU hCubeMenu = GetSubMenu(hViewMenu, 8);
	if (hCubeMenu == nullptr){
		return;
	}
	--m_CurrentCubeFace;
	CheckMenuRadioItem(hCubeMenu, ID_POS_X, ID_NEG_Z, s_CubeFaceCommandIDs[m_CurrentCubeFace],
		MF_BYCOMMAND);

	if (m_CurrentCubeFace == 0){
		EnableMenuItem(hViewMenu, ID_PREV_CUBE, MF_GRAYED);
	}
	if (m_CurrentCubeFace < 5){
		EnableMenuItem(hViewMenu, ID_NEXT_CUBE, MF_ENABLED);
	}
	SetTextureDirty();
	UpdateStatusBar();
	HWND hToolBar = GetToolbar();
	if (hToolBar != nullptr){
		ToolbarUpdateCubeFace(hToolBar);
	}
}
void CViewControl::SelectNextCubeFace(){
	if (m_ImageData.TextureType != eTextureType::_Cube ||
		m_CurrentCubeFace == 5){
		return;
	}
	HMENU hViewMenu = GetViewMenu();
	if (hViewMenu == nullptr){
		return;
	}
	HMENU hCubeMenu = GetSubMenu(hViewMenu, 8);
	if (hCubeMenu == nullptr){
		return;
	}
	++m_CurrentCubeFace;
	CheckMenuRadioItem(hCubeMenu, ID_POS_X, ID_NEG_Z, s_CubeFaceCommandIDs[m_CurrentCubeFace],
		MF_BYCOMMAND);

	if (m_CurrentCubeFace > 0){
		EnableMenuItem(hViewMenu, ID_PREV_CUBE, MF_ENABLED);
	}
	if (m_CurrentCubeFace == 5){
		EnableMenuItem(hViewMenu, ID_NEXT_CUBE, MF_GRAYED);
	}
	SetTextureDirty();
	UpdateStatusBar();
	HWND hToolBar = GetToolbar();
	if (hToolBar != nullptr){
		ToolbarUpdateCubeFace(hToolBar);
	}
}
void CViewControl::SelectCubeFace(uint32_t index){
	if (m_ImageData.TextureType != eTextureType::_Cube ||
		index > 5 || m_CurrentCubeFace == index){
		return;
	}
	HMENU hViewMenu = GetViewMenu();
	if (hViewMenu == nullptr){
		return;
	}
	HMENU hCubeMenu = GetSubMenu(hViewMenu, 8);
	if (hCubeMenu == nullptr){
		return;
	}
	m_CurrentCubeFace = index;
	CheckMenuRadioItem(hCubeMenu, ID_POS_X, ID_NEG_Z, s_CubeFaceCommandIDs[m_CurrentCubeFace],
		MF_BYCOMMAND);

	if (m_CurrentCubeFace > 0){
		EnableMenuItem(hViewMenu, ID_PREV_CUBE, MF_ENABLED);
	} else {
		EnableMenuItem(hViewMenu, ID_PREV_CUBE, MF_GRAYED);
	}
	if (m_CurrentCubeFace < 5){
		EnableMenuItem(hViewMenu, ID_NEXT_CUBE, MF_ENABLED);
	} else {
		EnableMenuItem(hViewMenu, ID_NEXT_CUBE, MF_GRAYED);
	}
	SetTextureDirty();
	UpdateStatusBar();
	HWND hToolBar = GetToolbar();
	if (hToolBar != nullptr){
		ToolbarUpdateCubeFace(hToolBar);
	}
}
void CViewControl::SelectPrevSlice(){
	if (m_ImageData.TextureType == eTextureType::_3D){
		HMENU hViewMenu = GetViewMenu();
		if (hViewMenu == nullptr){
			return;
		}

		uint32_t maxDepth = m_ImageData.MipInfos[m_CurrentMipLevel].Depth;
		if (m_CurrentSlice == 0 ||
			maxDepth < 2){
			return;
		}
		--m_CurrentSlice;
		if (m_CurrentSlice == 0){
			EnableMenuItem(hViewMenu, ID_PREV_SLICE, MF_GRAYED);
		}
		if (m_CurrentSlice < maxDepth - 1){
			EnableMenuItem(hViewMenu, ID_NEXT_SLICE, MF_ENABLED);
		}
		SetTextureDirty();
	} else {
		HMENU hViewMenu = GetViewMenu();
		if (hViewMenu == nullptr){
			return;
		}
		if (m_CurrentSlice == 0 ||
			m_ImageData.ArraySliceCount < 2){
			return;
		}
		--m_CurrentSlice;
		if (m_CurrentSlice == 0){
			EnableMenuItem(hViewMenu, ID_PREV_SLICE, MF_GRAYED);
		}
		if (m_CurrentSlice < m_ImageData.ArraySliceCount - 1){
			EnableMenuItem(hViewMenu, ID_NEXT_SLICE, MF_ENABLED);
		}
		SetTextureDirty();
	}
	UpdateStatusBar();
	UpdateSliceComboBoxSelection();
}
void CViewControl::SelectNextSlice(){
	if (m_ImageData.TextureType == eTextureType::_3D){
		HMENU hViewMenu = GetViewMenu();
		if (hViewMenu == nullptr){
			return;
		}

		uint32_t maxDepth = m_ImageData.MipInfos[m_CurrentMipLevel].Depth;
		if (m_CurrentSlice >= maxDepth - 1 ||
			maxDepth < 2){
			return;
		}
		++m_CurrentSlice;
		if (m_CurrentSlice >= maxDepth - 1){
			EnableMenuItem(hViewMenu, ID_NEXT_SLICE, MF_GRAYED);
		}
		if (m_CurrentSlice > 0){
			EnableMenuItem(hViewMenu, ID_PREV_SLICE, MF_ENABLED);
		}
		SetTextureDirty();
	} else {
		HMENU hViewMenu = GetViewMenu();
		if (hViewMenu == nullptr){
			return;
		}
		if (m_CurrentSlice >= m_ImageData.ArraySliceCount - 1 ||
			m_ImageData.ArraySliceCount < 2){
			return;
		}
		++m_CurrentSlice;
		if (m_CurrentSlice >= m_ImageData.ArraySliceCount - 1){
			EnableMenuItem(hViewMenu, ID_NEXT_SLICE, MF_ENABLED);
		}
		if (m_CurrentSlice > 0){
			EnableMenuItem(hViewMenu, ID_PREV_SLICE, MF_GRAYED);
		}
		SetTextureDirty();
	}
	UpdateSliceComboBoxSelection();
	UpdateStatusBar();
}
void CViewControl::SelectSmallerMipLevel(){
	if (m_ImageData.MipCount < 2 ||
		m_CurrentMipLevel >= m_ImageData.MipCount - 1){
		return;
	}
	HMENU hViewMenu = GetViewMenu();
	if (hViewMenu == nullptr){
		return;
	}

	if (m_ImageData.TextureType == eTextureType::_3D){
		m_CurrentSlice /= 2;
	}

	++m_CurrentMipLevel;

	if (m_CurrentMipLevel >= m_ImageData.MipCount - 1){
		EnableMenuItem(hViewMenu, ID_SMALLER_MIP, MF_GRAYED);
	}
	if (m_CurrentMipLevel > 0){
		EnableMenuItem(hViewMenu, ID_LARGER_MIP, MF_ENABLED);
	}
	SetTextureDirty();
	UpdateStatusBar();
	if (m_ImageData.TextureType == eTextureType::_3D){
		UpdateSliceComboBoxContents();
	}
	UpdateMipComboBoxSelection();
}
void CViewControl::SelectLargerMipLevel(){
	if (m_ImageData.MipCount < 2 ||
		m_CurrentMipLevel == 0){
		return;
	}
	HMENU hViewMenu = GetViewMenu();
	if (hViewMenu == nullptr){
		return;
	}

	if (m_ImageData.TextureType == eTextureType::_3D){
		if (m_ImageData.MipInfos[m_CurrentMipLevel].Depth - 1 == m_CurrentSlice){
			m_CurrentSlice = m_ImageData.MipInfos[m_CurrentMipLevel - 1].Depth - 1;
		} else {
			m_CurrentSlice *= 2;
		}
	}

	--m_CurrentMipLevel;

	if (m_CurrentMipLevel < m_ImageData.MipCount - 1){
		EnableMenuItem(hViewMenu, ID_SMALLER_MIP, MF_ENABLED);
	}
	if (m_CurrentMipLevel == 0){
		EnableMenuItem(hViewMenu, ID_LARGER_MIP, MF_GRAYED);
	}
	SetTextureDirty();
	UpdateStatusBar();
	if (m_ImageData.TextureType == eTextureType::_3D){
		UpdateSliceComboBoxContents();
	}
	UpdateMipComboBoxSelection();
}
void CViewControl::ZoomIn(){
	if (m_ZoomFactor < 5){
		++m_ZoomFactor;
		UpdateScrollBars();
		SetRenderDirty();
		UpdateStatusBar();
	}
}
void CViewControl::ZoomOut(){
	if (m_ZoomFactor > -5){
		--m_ZoomFactor;
		UpdateScrollBars();
		SetRenderDirty();
		UpdateStatusBar();
	}
}
void CViewControl::UpdateScrollBars(){
	if (m_InScrollBarUpdate){
		return;
	}
	m_InScrollBarUpdate = true;
	uint32_t imageWidth = m_ImageData.MipInfos[m_CurrentMipLevel].Width;
	uint32_t imageHeight = m_ImageData.MipInfos[m_CurrentMipLevel].Height;




	DWORD style = GetWindowLongW(m_hWnd, GWL_STYLE);



	RECT clientRect;
	GetClientRect(m_hWnd, &clientRect);

	int32_t windowWidth = clientRect.right - clientRect.left;
	int32_t windowHeight = clientRect.bottom - clientRect.top;

	if (m_ZoomFactor < 0){
		imageWidth >>= uint32_t(-m_ZoomFactor);
		imageHeight >>= uint32_t(-m_ZoomFactor);
	} else {
		imageWidth <<= m_ZoomFactor;
		imageHeight <<= m_ZoomFactor;
	}


	bool needVScroll = false, needHScroll = false;

	if (style & WS_HSCROLL){
		windowHeight += GetSystemMetrics(SM_CYHSCROLL);
	}
	if (style & WS_VSCROLL){
		windowWidth += GetSystemMetrics(SM_CXVSCROLL);
	}

	bool hasChanged = true;

	for (; hasChanged;){
		hasChanged = false;

		if (needHScroll == false &&
			int32_t(imageWidth) > windowWidth){
			windowHeight -= GetSystemMetrics(SM_CYHSCROLL);
			needHScroll = true;
			hasChanged = true;
		}
		if (needVScroll == false &&
			int32_t(imageHeight) > windowHeight){
			windowWidth -= GetSystemMetrics(SM_CXVSCROLL);
			needVScroll = true;
			hasChanged = true;
		}
	}
	SCROLLINFO scrollInfo = {
		sizeof(SCROLLINFO),
		SIF_RANGE | SIF_PAGE | SIF_POS,
		0,
	};

	SendMessageA(m_hWnd, WM_SETREDRAW, (WPARAM)FALSE, 0);

	uint32_t scrollWindowWidth = windowWidth;
	uint32_t scrollWindowHeight = windowHeight;

	/*if (m_ZoomFactor > 0){
		scrollWindowWidth >>= m_ZoomFactor;
		if (scrollWindowWidth == 0){
			scrollWindowWidth = 1;
		}
		scrollWindowHeight >>= m_ZoomFactor;
		if (scrollWindowHeight == 0){
			scrollWindowHeight = 0;
		}
	}*/

	scrollInfo.nMax = int(imageHeight) - 1;
	scrollInfo.nPage = UINT(scrollWindowHeight);
	scrollInfo.nPos = m_PositionY;
	SetScrollInfo(m_hWnd, SB_VERT, &scrollInfo, TRUE);
	m_ScrollBoundsY = needVScroll ? int32_t(imageHeight) - scrollWindowHeight : 0;

	scrollInfo.nMax = int(imageWidth) - 1;
	scrollInfo.nPage = UINT(scrollWindowWidth);
	scrollInfo.nPos = m_PositionX;
	SetScrollInfo(m_hWnd, SB_HORZ, &scrollInfo, TRUE);
	SendMessageA(m_hWnd, WM_SETREDRAW, (WPARAM)TRUE, 0);
	m_ScrollBoundsX = needHScroll ? int32_t(imageWidth) - scrollWindowWidth : 0;


	m_InScrollBarUpdate = false;
	SetSwapChainDirty();
}

void CViewControl::Render(){
	if (!m_RenderDirty){
		return;
	}
	UpdateTexture();
	m_RenderDirty = false;
	


	RECT clientRect;
	GetClientRect(m_hWnd, &clientRect);

	CGlobalState &globalState = GetGlobalState();

	globalState.Context->IASetIndexBuffer(globalState.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	globalState.Context->IASetInputLayout(globalState.InputLayout);
	globalState.Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	UINT strides[1] = { sizeof(SVertex) };
	UINT offsets[1] = { 0 };
	ID3D11Buffer* buffers[1] = { globalState.VertexBuffer };
	globalState.Context->IASetVertexBuffers(0, 1, buffers, strides, offsets);
	ID3D11RenderTargetView* renderTargetViews[1] = { m_RenderTargetView };
	globalState.Context->OMSetRenderTargets(1, renderTargetViews, nullptr);
	D3D11_VIEWPORT viewPort = {
		0.0f, 0.0f,
		float(clientRect.right - clientRect.left),
		float(clientRect.bottom - clientRect.top),
		0.0f, 1.0f,
	};
	globalState.Context->RSSetViewports(1, &viewPort);



	float blendFactor[4] = {
		1.0f, 1.0f, 1.0f, 1.0f
	};
	globalState.Context->OMSetBlendState(globalState.OpaqueBlendState,
		blendFactor, 0xffffffff);

	uint32_t width = (uint32_t)m_ImageData.MipInfos[m_CurrentMipLevel].Width;
	uint32_t height = (uint32_t)m_ImageData.MipInfos[m_CurrentMipLevel].Height;

	if (m_ZoomFactor < 0){
		width >>= -m_ZoomFactor;
		height >>= -m_ZoomFactor;
	} else if (m_ZoomFactor > 0){
		width <<= m_ZoomFactor;
		height <<= m_ZoomFactor;
	}

	uint32_t zoomAdjustedPositionX = m_PositionX;
	uint32_t zoomAdjustedPositionY = m_PositionY;

	/*if (m_ZoomFactor > 0){
		zoomAdjustedPositionX <<= m_ZoomFactor;
		zoomAdjustedPositionY <<= m_ZoomFactor;
	}*/

	if (globalState.CheckerBoard){
		SCheckerBoardShaderData checkerBoardData = {
			{float(clientRect.right - clientRect.left), float(clientRect.bottom - clientRect.top)},
			{32.0f},
			0.0f,
			{-float(zoomAdjustedPositionX), -float(zoomAdjustedPositionY)},
			{float(width), float(height)},
			globalState.ColorA, //{0.5f, 0.5f, 0.5f},
			0.0f,
			globalState.ColorB, //{0.75f, 0.75f, 0.75f},
			0.0f,
			globalState.FillColor
		};
		globalState.Context->UpdateSubresource(m_CheckerBoardShaderParams, 0, nullptr,
			&checkerBoardData, 0, 0);

		ID3D11Buffer* constantBuffers[1] = { m_CheckerBoardShaderParams };
		globalState.Context->VSSetShader(
			globalState.CheckerBoardVertexShader, nullptr, 0);
		globalState.Context->PSSetShader(
			globalState.CheckerBoardPixelShader, nullptr, 0);
		globalState.Context->PSSetConstantBuffers(0, 1, constantBuffers);
		globalState.Context->VSSetConstantBuffers(0, 1, constantBuffers);
		globalState.Context->DrawIndexed(6, 0, 0);
	} else {
		CCRGB fillColor = globalState.FillColor;
		float color[4] = {
			fillColor.R, fillColor.G, fillColor.B,
			1.0f
		};
		globalState.Context->ClearRenderTargetView(m_RenderTargetView, color);
	}



	if (m_CurrentChannel == Component_RGBA){
		globalState.Context->OMSetBlendState(globalState.AlphaBlendState,
			blendFactor, 0xffffffff);
	}

	SShaderData data = {
		{ float(clientRect.right - clientRect.left), float(clientRect.bottom - clientRect.top)},
		{ float(width),
		float(height) },
		-float(zoomAdjustedPositionX), -float(zoomAdjustedPositionY),
		0.0f, 0.0f,
		m_UVScale
	};
	globalState.Context->UpdateSubresource(m_ShaderParams, 0, nullptr, &data, 0, 0);
	ID3D11Buffer* constantBuffers[1] = { m_ShaderParams };

	globalState.Context->VSSetShader(globalState.VertexShader, nullptr, 0);
	globalState.Context->PSSetShader(
		globalState.PixelShaders[Type_UNorm][Swap_None][m_CurrentChannel], nullptr, 0);
	globalState.Context->PSSetConstantBuffers(0, 1, constantBuffers);
	globalState.Context->VSSetConstantBuffers(0, 1, constantBuffers);


	ID3D11SamplerState* samplerStates[1] = { globalState.SamplerState };

	globalState.Context->PSSetSamplers(0, 1, samplerStates);
	ID3D11ShaderResourceView* shaderResourceViews[1] = { m_ShaderResourceView };
	globalState.Context->PSSetShaderResources(0, 1, shaderResourceViews);
	globalState.Context->DrawIndexed(6, 0, 0);
}

void CViewControl::UpdateTexture(){
	if (!m_TextureDirty){
		return;
	}
	HRESULT hr;
	ID3D11ShaderResourceView* nullSRV = nullptr;

	CGlobalState &globalState = GetGlobalState();

	globalState.Context->PSSetShaderResources(0, 1, &nullSRV);
	m_ShaderResourceView.Clear();
	m_Texture.Clear();

	D3D11_TEXTURE2D_DESC texture2dDesc = {};
	texture2dDesc.Width = m_ImageData.MipInfos[m_CurrentMipLevel].Width;
	texture2dDesc.Height = m_ImageData.MipInfos[m_CurrentMipLevel].Height;
	texture2dDesc.ArraySize = 1;
	texture2dDesc.MipLevels = 1;
	texture2dDesc.Format = m_ImageData.Format;
	texture2dDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texture2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texture2dDesc.SampleDesc = DXGI_SAMPLE_DESC{ 1, 0 };
	D3D11_SUBRESOURCE_DATA subResourceData;


	UINT originalWidth = texture2dDesc.Width;
	UINT originalHeight = texture2dDesc.Height;

	if (Format_GetElementType(m_ImageData.Format) == eTextureElementType::Block){
		texture2dDesc.Width = (texture2dDesc.Width + 3) / 4 * 4;
		texture2dDesc.Height = (texture2dDesc.Height + 3) / 4 * 4;
	}

	m_UVScale.X = float(originalWidth) / float(texture2dDesc.Width);
	m_UVScale.Y = float(originalHeight) / float(texture2dDesc.Height);

	if (m_ImageData.TextureType == eTextureType::_Cube){
		const char* pData = (const char*)m_ImageData.ImageData.GetPointer();
		pData += ((m_CurrentSlice * 6) + m_CurrentCubeFace) * m_ImageData.SliceStride +
			m_ImageData.MipInfos[m_CurrentMipLevel].Offset;
		subResourceData.pSysMem = pData;
	} else if (m_ImageData.TextureType == eTextureType::_3D) {
		const char* pData = (const char*)m_ImageData.ImageData.GetPointer();
		pData +=
			m_ImageData.MipInfos[m_CurrentMipLevel].Offset +
			m_ImageData.MipInfos[m_CurrentMipLevel].DepthStride * m_CurrentSlice;
		subResourceData.pSysMem = pData;
	} else {
		const char* pData = (const char*)m_ImageData.ImageData.GetPointer();
		pData += m_CurrentSlice * m_ImageData.SliceStride +
			m_ImageData.MipInfos[m_CurrentMipLevel].Offset;
		subResourceData.pSysMem = pData;
	}

	subResourceData.SysMemPitch = (UINT)m_ImageData.MipInfos[m_CurrentMipLevel].RowStride;

	hr = globalState.Device->CreateTexture2D(&texture2dDesc, &subResourceData, m_Texture.GetAddress());

	if (FAILED(hr)){
		return;
	}
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
	shaderResourceViewDesc.Format = m_ImageData.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;

	hr = globalState.Device->CreateShaderResourceView(m_Texture,
		&shaderResourceViewDesc, m_ShaderResourceView.GetAddress());

	if (FAILED(hr)){
		m_Texture.Clear();
		return;
	}

}

void CViewControl::UpdateSwapChain(){
	if (!m_SwapChainDirty){
		return;
	}
	m_SwapChainDirty = false;
	RECT clientRect;
	HRESULT hr;
	GetClientRect(m_hWnd, &clientRect);
	m_RenderTargetView.Clear();
	m_BackBuffer.Clear();

	CGlobalState &globalState = GetGlobalState();

	if (!m_ShaderParams){
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.ByteWidth = sizeof(SShaderData);
		bufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		hr = globalState.Device->CreateBuffer(&bufferDesc, nullptr, m_ShaderParams.GetAddress());
		if (FAILED(hr)){
			return;
		}
	}
	if (!m_CheckerBoardShaderParams){
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.ByteWidth = sizeof(SCheckerBoardShaderData);
		bufferDesc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		hr = globalState.Device->CreateBuffer(&bufferDesc, nullptr, m_CheckerBoardShaderParams.GetAddress());
		if (FAILED(hr)){
			return;
		}
	}
	if (m_SwapChain){
		m_SwapChain->ResizeBuffers(1, clientRect.right - clientRect.left,
			clientRect.bottom - clientRect.top, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	} else {
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.Width = clientRect.right - clientRect.left;
		swapChainDesc.BufferDesc.Height = clientRect.bottom - clientRect.top;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;// 60;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 0;// 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Windowed = TRUE;
		swapChainDesc.OutputWindow = m_hWnd;

		hr = globalState.DXGIFactory->CreateSwapChain(globalState.Device,
			&swapChainDesc, m_SwapChain.GetAddress());
		if (FAILED(hr)){
			return;
		}
	}
	hr = m_SwapChain->GetBuffer(0, IID_ID3D11Texture2D, 
		(void**)m_BackBuffer.GetAddress());
	if (FAILED(hr)){
		return;
	}
	hr = globalState.Device->CreateRenderTargetView(m_BackBuffer, nullptr,
		m_RenderTargetView.GetAddress());
	if (FAILED(hr)){
		return;
	}
	SetRenderDirty();
}

CViewControl::CViewControl(HWND hWnd) : m_hWnd(hWnd){
}

CViewControl::~CViewControl(){
}

void CViewControl::Paint(){
	if (m_SwapChain){
		Render();
		m_SwapChain->Present(0, 0);
	}
}

void CViewControl::VScroll(WORD type){
	if (m_ScrollBoundsY == 0){
		m_PositionY = 0;
		return;
	}
	switch (type){
	case SB_ENDSCROLL:{
		break;
	}
	case SB_LEFT:{
		m_PositionY = 0;
		break;
	}
	case SB_RIGHT: {
		m_PositionY = m_ScrollBoundsY;
		break;
	}
	case SB_LINELEFT:{
		if (m_PositionY > 0){
			--m_PositionY;
		}
		break;
	}
	case SB_LINERIGHT:{
		if (m_PositionY < m_ScrollBoundsY){
			++m_PositionY;
		}
		break;
	}
	case SB_PAGELEFT:{
		if (m_PositionY > 0){
			--m_PositionY;
		}
		break;
	}
	case SB_PAGERIGHT:{
		if (m_PositionY < m_ScrollBoundsY){
			++m_PositionY;
		}
		break;
	}
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:{
		SCROLLINFO scrollInfo = { sizeof(SCROLLINFO), SIF_TRACKPOS };
		GetScrollInfo(m_hWnd, SB_VERT, &scrollInfo);
		m_PositionY = scrollInfo.nTrackPos;
		if (m_PositionY < 0){
			m_PositionY = 0;
		} else if (m_PositionY > m_ScrollBoundsY){
			m_PositionY = m_ScrollBoundsY;
		}
		break;
	}
	}
	SCROLLINFO scrollInfo = { sizeof(SCROLLINFO), SIF_POS };
	scrollInfo.nPos = m_PositionY;
	SetScrollInfo(m_hWnd, SB_VERT, &scrollInfo, TRUE);
	SetRenderDirty();

}
void CViewControl::HScroll(WORD type){
	if (m_ScrollBoundsX == 0){
		m_PositionX = 0;
		return;
	}
	switch (type){
	case SB_ENDSCROLL:{
		break;
	}
	case SB_LEFT:{
		m_PositionX = 0;
		break;
	}
	case SB_RIGHT: {
		m_PositionX = m_ScrollBoundsX;
		break;
	}
	case SB_LINELEFT:{
		if (m_PositionX > 0){
			--m_PositionX;
		}
		break;
	}
	case SB_LINERIGHT:{
		if (m_PositionX < m_ScrollBoundsX){
			++m_PositionX;
		}
		break;
	}
	case SB_PAGELEFT:{
		if (m_PositionX > 0){
			--m_PositionX;
		}
		break;
	}
	case SB_PAGERIGHT:{
		if (m_PositionX < m_ScrollBoundsX){
			++m_PositionX;
		}
		break;
	}
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:{
		SCROLLINFO scrollInfo = { sizeof(SCROLLINFO), SIF_TRACKPOS };
		GetScrollInfo(m_hWnd, SB_HORZ, &scrollInfo);
		m_PositionX = scrollInfo.nTrackPos;
		if (m_PositionX < 0){
			m_PositionX = 0;
		} else if (m_PositionX > m_ScrollBoundsX){
			m_PositionX = m_ScrollBoundsX;
		}
		break;
	}
	}
	SCROLLINFO scrollInfo = { sizeof(SCROLLINFO), SIF_POS };
	scrollInfo.nPos = m_PositionX;
	SetScrollInfo(m_hWnd, SB_HORZ, &scrollInfo, TRUE);
	SetRenderDirty();
}
void CViewControl::LMouseDown(int32_t x, int32_t y){
	if (m_Dragging){
		return;
	}
	m_Dragging = true;
	m_LastX = x;
	m_LastY = y;
	SetCapture(m_hWnd);
}
void CViewControl::LMouseUp(int32_t x, int32_t y){
	m_Dragging = false;
	SetCapture(nullptr);
}
void CViewControl::MouseMove(int32_t x, int32_t y){
	if (!m_Dragging){
		return;
	}
	int32_t deltaX = x - m_LastX;
	int32_t deltaY = y - m_LastY;
	m_LastX = x;
	m_LastY = y;

	m_PositionX -= deltaX;
	m_PositionY -= deltaY;

	if (m_PositionX < 0){
		m_PositionX = 0;
	} else if (m_PositionX > m_ScrollBoundsX){
		m_PositionX = m_ScrollBoundsX;
	}
	if (m_PositionY < 0){
		m_PositionY = 0;
	} else if (m_PositionY > m_ScrollBoundsY){
		m_PositionY = m_ScrollBoundsY;
	}
	SCROLLINFO scrollInfo = { sizeof(SCROLLINFO), SIF_POS };
	scrollInfo.nPos = m_PositionX;
	SetScrollInfo(m_hWnd, SB_HORZ, &scrollInfo, TRUE);

	scrollInfo.nPos = m_PositionY;
	SetScrollInfo(m_hWnd, SB_VERT, &scrollInfo, TRUE);
	SetRenderDirty();
}

LRESULT CViewControl::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch (uMsg){
	case WM_CREATE:{
		break;
	}
	case WM_DESTROY:{
		break;
	}
	case WM_GETVIEWWINDOW:{
		return (LRESULT)this;
	}
	case WM_PAINT:{
		DefMDIChildProcW(hWnd, uMsg, wParam, lParam);
		UpdateSwapChain();
		Paint();
		break;
	}
	case WM_ERASEBKGND:{
		return 1;
	}
	case WM_VSCROLL:{
		VScroll(LOWORD(wParam));
		return 0;
	}
	case WM_HSCROLL:{
		HScroll(LOWORD(wParam));
		return 0;
	}
	case WM_LBUTTONDOWN:{
		LMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}
	case WM_LBUTTONUP:{
		LMouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}
	case WM_MOUSEMOVE:{
		MouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}

	case WM_SIZE:{
		UpdateScrollBars();
		if (!m_InScrollBarUpdate){
			UpdateSwapChain();
			InvalidateRect(m_hWnd, nullptr, TRUE);
		}
		return 0;
	}
	default:{
		return DefMDIChildProcW(hWnd, uMsg, wParam, lParam);
	}

	}
	return 0;
}



/*
CDocumentWindow

*/

CDocumentWindow::CDocumentWindow(HWND hWnd) : m_hWnd(hWnd){
}

bool CDocumentWindow::HandleCommand(WPARAM wParam, LPARAM lParam){

	if (HIWORD(wParam) == CBN_SELCHANGE){
		switch (LOWORD(wParam)){
		case ID_MIP_COMBO_BOX:{
			if (m_pViewControl != nullptr){
				m_pViewControl->HandleMipSelectionChange();
			}
			return true;
		}
		case ID_SLICE_COMBO_BOX:{
			if (m_pViewControl != nullptr){
				m_pViewControl->HandleSliceSelectionChange();
			}
			return true;
		}
		}
	}

	switch (LOWORD(wParam)){
	case ID_POS_X:{
		if (m_pViewControl){
			m_pViewControl->SelectCubeFace(0);
		}
		return true;
	}
	case ID_NEG_X:{
		if (m_pViewControl){
			m_pViewControl->SelectCubeFace(1);
		}
		return true;
	}
	case ID_POS_Y:{
		if (m_pViewControl){
			m_pViewControl->SelectCubeFace(2);
		}
		return true;
	}
	case ID_NEG_Y:{
		if (m_pViewControl){
			m_pViewControl->SelectCubeFace(3);
		}
		return true;
	}
	case ID_POS_Z:{
		if (m_pViewControl){
			m_pViewControl->SelectCubeFace(4);
		}
		return true;
	}
	case ID_NEG_Z:{
		if (m_pViewControl){
			m_pViewControl->SelectCubeFace(5);
		}
		return true;
	}
	case ID_NEXT_CUBE:{
		if (m_pViewControl){
			m_pViewControl->SelectNextCubeFace();
		}
		return true;
	}
	case ID_PREV_CUBE:{
		if (m_pViewControl){
			m_pViewControl->SelectPrevCubeFace();
		}
		return true;
	}
	case ID_NEXT_SLICE:{
		if (m_pViewControl){
			m_pViewControl->SelectNextSlice();
		}
		return true;
	}
	case ID_PREV_SLICE:{
		if (m_pViewControl){
			m_pViewControl->SelectPrevSlice();
		}
		return true;
	}
	case ID_SMALLER_MIP:{
		if (m_pViewControl){
			m_pViewControl->SelectSmallerMipLevel();
		}
		return true;
	}
	case ID_LARGER_MIP:{
		if (m_pViewControl){
			m_pViewControl->SelectLargerMipLevel();
		}
		return true;
	}
	case ID_ZOOM_IN:{
		if (m_pViewControl){
			m_pViewControl->ZoomIn();
		}
		return true;
	}
	case ID_ZOOM_OUT:{
		if (m_pViewControl){
			m_pViewControl->ZoomOut();
		}
		return true;
	}
	case ID_SAVE_SURFACE:{
		if (m_pViewControl){
			m_pViewControl->SaveSurface();
		}
		return true;
	}
	case ID_RGBA:{
		if (m_pViewControl){
			m_pViewControl->SelectRGBA();
		}
		return true;
	}
	case ID_RGB:{
		if (m_pViewControl){
			m_pViewControl->SelectRGB();
		}
		return true;
	}
	case ID_R:{
		if (m_pViewControl){
			m_pViewControl->SelectR();
		}
		return true;
	}
	case ID_G:{
		if (m_pViewControl){
			m_pViewControl->SelectG();
		}
		return true;
	}
	case ID_B:{
		if (m_pViewControl){
			m_pViewControl->SelectB();
		}
		return true;
	}
	case ID_A:{
		if (m_pViewControl){
			m_pViewControl->SelectA();
		}
		return true;
	}
	}
	return false;
}
LRESULT CDocumentWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	switch (uMsg){
	case WM_CREATE:{

		RECT clientRect;
		GetClientRect(hWnd, &clientRect);
		m_hViewControl = CreateWindowExW(0, DDS_VIEW_WINDOW_CLASS, nullptr,
			WS_CHILD | WS_VISIBLE, 0, 0, clientRect.right - clientRect.left,
			clientRect.bottom - clientRect.top, hWnd, nullptr, 
			GetGlobalState().hInstance, &m_pViewControl);

		if (m_hViewControl == nullptr){
			return -1;
		}

		LPCREATESTRUCTW pCreateStruct = (LPCREATESTRUCTW)lParam;
		if (pCreateStruct->lpCreateParams != nullptr){
			LPMDICREATESTRUCTW pMDICreateStruct =
				(LPMDICREATESTRUCTW)pCreateStruct->lpCreateParams;
			m_pViewControl->m_ImageData = std::move(*(CDDSImageData*)pMDICreateStruct->lParam);
			m_pViewControl->m_Path = pMDICreateStruct->szTitle;
			m_pViewControl->Init();
		}
		break;
	}
	case WM_COMMAND:{
		if (HandleCommand(wParam, lParam)){
			return 0;
		}
		return 1;
	}
	case WM_MDIACTIVATE:{
		if (m_pViewControl){
			m_pViewControl->UpdateSliceComboBoxContents();
			m_pViewControl->UpdateMenu();
			m_pViewControl->UpdateMipComboBoxContents();
			m_pViewControl->UpdateToolbar();
		}
		break;
	}
	case WM_RERENDER:{
		if (m_pViewControl){
			m_pViewControl->SetRenderDirty();
		}
		break;
	}
	case WM_SIZE:{
		RECT clientRect;
		GetClientRect(m_hWnd, &clientRect);

		SetWindowPos(m_hViewControl, nullptr, 0, 0,
			clientRect.right - clientRect.left,
			clientRect.bottom - clientRect.top, SWP_NOZORDER);


		return DefMDIChildProcW(hWnd, uMsg, wParam, lParam);
	}
	case WM_DESTROY:{
		SendMessageW(GetGlobalState().hFrameWindow, WM_DOCUMENT_CLOSING, 0, 0);
		break;
	}
	case WM_GETDOCUMENTWINDOW:{
		return (LRESULT)this;
	}
	default:{
		return DefMDIChildProcW(hWnd, uMsg, wParam, lParam);
	}

	}
	return 0;
}


static LRESULT CALLBACK ViewWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	CViewControl* pViewControl;
	if (uMsg == WM_CREATE){
		pViewControl = new CViewControl(hWnd);
		if (!pViewControl){
			return -1;
		}
		LPCREATESTRUCTW pCreateStruct = (LPCREATESTRUCTW)lParam;
		if (pCreateStruct->lpCreateParams != nullptr){
			*(CViewControl**)(pCreateStruct->lpCreateParams) = pViewControl;
		}
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pViewControl);
	} else {
		pViewControl = (CViewControl*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
	}
	LRESULT result = pViewControl ? pViewControl->WindowProc(hWnd, uMsg, wParam, lParam) :
		DefMDIChildProcW(hWnd, uMsg, wParam, lParam);
	if (uMsg == WM_DESTROY){
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)nullptr);
		delete pViewControl;
		return 0;
	}
	return result;
}

static LRESULT CALLBACK DocumentWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	CDocumentWindow* pDocumentWindow;
	if (uMsg == WM_CREATE){
		pDocumentWindow = new CDocumentWindow(hWnd);
		if (!pDocumentWindow){
			return -1;
		}
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pDocumentWindow);
	} else {
		pDocumentWindow = (CDocumentWindow*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
	}
	LRESULT result = pDocumentWindow ? 
		pDocumentWindow->WindowProc(hWnd, uMsg, wParam, lParam) :
		DefMDIChildProcW(hWnd, uMsg, wParam, lParam);

	if (uMsg == WM_DESTROY){
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)nullptr);
		delete pDocumentWindow;
		return 0;
	}
	return result;
}

bool InitializeDocumentWindowClasses(HINSTANCE hInstance){
	   
	WNDCLASSEXW windowClass = {};
	windowClass.cbSize = sizeof(WNDCLASSEXW);
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
	windowClass.lpszMenuName = nullptr;


	windowClass.lpfnWndProc = DocumentWindowProc;
	windowClass.lpszClassName = DDS_DOCUMENT_WINDOW_CLASS;

	if (!RegisterClassExW(&windowClass)){
		return false;
	}

	windowClass.lpfnWndProc = ViewWindowProc;
	windowClass.lpszClassName = DDS_VIEW_WINDOW_CLASS;
	windowClass.style = 0;

	if (!RegisterClassExW(&windowClass)){
		return false;
	}
	return true;
}
