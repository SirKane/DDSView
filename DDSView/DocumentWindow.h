#include "Main.h"
#include "DDSLoader.h"

class CViewControl{
public:
	HWND										m_hWnd = nullptr;
	TRefCountHandle<ID3D11Texture2D>			m_Texture;
	TRefCountHandle<ID3D11ShaderResourceView>	m_ShaderResourceView;
	TRefCountHandle<IDXGISwapChain>				m_SwapChain;
	TRefCountHandle<ID3D11Buffer>				m_ShaderParams;
	TRefCountHandle<ID3D11Buffer>				m_CheckerBoardShaderParams;
	TRefCountHandle<ID3D11RenderTargetView>		m_RenderTargetView;
	TRefCountHandle<ID3D11Texture2D>			m_BackBuffer;
	CStringW									m_Path;


	CDDSImageData								m_ImageData;
	bool										m_SwapChainDirty = true;
	bool										m_RenderDirty = true;
	bool										m_TextureDirty = true;

	int32_t										m_ZoomFactor = 0;

	uint32_t									m_CurrentMipLevel = 0;
	uint32_t									m_CurrentSlice = 0;
	uint32_t									m_CurrentCubeFace = 0;
	int32_t										m_PositionX = 0;
	int32_t										m_PositionY = 0;
	int32_t										m_ScrollBoundsX = 0;
	int32_t										m_ScrollBoundsY = 0;
	SVec2										m_UVScale;

	uint32_t									m_CurrentChannel = Component_RGB;

	bool										m_InScrollBarUpdate = false;
	int32_t										m_LastX = 0;
	int32_t										m_LastY = 0;
	bool										m_Dragging = false;

	void SaveSurface();
	void SaveSurfaceToFile(const wchar_t* pPath);

	void Init();

	HMENU GetViewMenu();
	HMENU GetFileMenu();
	void SetSwapChainDirty();
	void SetRenderDirty();
	void SetTextureDirty();
	void SelectRGBA();
	void SelectRGB();
	void SelectR();
	void SelectG();
	void SelectB();
	void SelectA();

	void UpdateStatusBar();
	HWND GetToolbar();
	HWND GetMipComboBox();
	HWND GetSliceComboBox();
	void ToolbarUpdateChannel(HWND hToolbar);
	void ToolbarUpdateCubeFace(HWND hToolbar);
	void UpdateToolbar();
	void UpdateSliceComboBoxContents();
	void UpdateSliceComboBoxSelection();
	void UpdateMipComboBoxContents();

	void UpdateMipComboBoxSelection();
	void HandleMipSelectionChange();
	void HandleSliceSelectionChange();
	void UpdateMenu();

	void SelectPrevCubeFace();
	void SelectNextCubeFace();
	void SelectCubeFace(uint32_t index);
	void SelectPrevSlice();
	void SelectNextSlice();
	void SelectSmallerMipLevel();
	void SelectLargerMipLevel();
	void ZoomIn();
	void ZoomOut();
	void UpdateScrollBars();

	void Render();

	void UpdateTexture();

	void UpdateSwapChain();

	CViewControl(HWND hWnd);

	~CViewControl();
	void Paint();

	void VScroll(WORD type);
	void HScroll(WORD type);
	void LMouseDown(int32_t x, int32_t y);
	void LMouseUp(int32_t x, int32_t y);
	void MouseMove(int32_t x, int32_t y);
	LRESULT WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

class CDocumentWindow{
public:
	HWND			m_hWnd = nullptr;
	HWND			m_hViewControl = nullptr;
	CViewControl*	m_pViewControl = nullptr;
	CDocumentWindow(HWND hWnd);
	bool HandleCommand(WPARAM wParam, LPARAM lParam);
	LRESULT WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};


bool InitializeDocumentWindowClasses(HINSTANCE hInstance);