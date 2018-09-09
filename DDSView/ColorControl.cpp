#include <Windows.h>
#include <stdio.h>
#include <GdiPlus.h>
#include <Windowsx.h>
#include "ColorControl.h"
#include "ColorControlResource.h"
#include "Commctrl.h"

#include <stdint.h>

namespace {

#define LERP_F32(a, b, t) ((a)+((b)-(a))*t)
#define RETURN_RGB(_r, _g, _b) { ret.R = (_r); ret.G = (_g); ret.B = (_b); return ret; }
#define RETURN_HSV(_h, _s, _v) { ret.H = (_h); ret.S = (_s); ret.V = (_v); return ret; }


	class CPoint {
	public:
		int32_t	X;
		int32_t	Y;
		CPoint(){
			X = Y = 0;
		}
		CPoint(int32_t x, int32_t y){
			X = x;
			Y = y;
		}
		CPoint(const CPoint &rhs){
			X = rhs.X;
			Y = rhs.Y;
		}
		CPoint(const POINT &rhs){
			X = rhs.x;
			Y = rhs.y;
		}
		CPoint& operator= (const CPoint rhs){
			X = rhs.X;
			Y = rhs.Y;
			return *this;
		}
		CPoint& operator= (const POINT rhs){
			X = rhs.x;
			Y = rhs.y;
			return *this;
		}
		bool operator == (const CPoint rhs){
			return (X == rhs.X && Y == rhs.Y);
		}
		bool operator != (const CPoint rhs){
			return !(X == rhs.X && Y == rhs.Y);
		}
		CPoint operator +(const CPoint rhs){
			return CPoint(X + rhs.X, Y + rhs.Y);
		}
		CPoint operator -(const CPoint rhs){
			return CPoint(X - rhs.X, Y - rhs.Y);
		}
		CPoint operator *(const CPoint rhs){
			return CPoint(X * rhs.X, Y * rhs.Y);
		}
		CPoint operator /(const CPoint rhs){
			return CPoint(X / rhs.X, Y / rhs.Y);
		}
		CPoint operator %(const CPoint rhs){
			return CPoint(X % rhs.X, Y % rhs.Y);
		}
		CPoint operator <<(const CPoint rhs){
			return CPoint(X << rhs.X, Y << rhs.Y);
		}
		CPoint operator >> (const CPoint rhs){
			return CPoint(X >> rhs.X, Y >> rhs.Y);
		}

		CPoint operator +(const int32_t rhs){
			return CPoint(X + rhs, Y + rhs);
		}
		CPoint operator -(const int32_t rhs){
			return CPoint(X - rhs, Y - rhs);
		}
		CPoint operator *(const int32_t rhs){
			return CPoint(X * rhs, Y * rhs);
		}
		CPoint operator /(const int32_t rhs){
			return CPoint(X / rhs, Y / rhs);
		}
		CPoint operator %(const int32_t rhs){
			return CPoint(X % rhs, Y % rhs);
		}
		CPoint operator <<(const int32_t rhs){
			return CPoint(X << rhs, Y << rhs);
		}
		CPoint operator >> (const int32_t rhs){
			return CPoint(X >> rhs, Y >> rhs);
		}

		CPoint& operator +=(const CPoint&rhs){
			X += rhs.X;
			Y += rhs.Y;
			return *this;
		}
		CPoint& operator -=(const CPoint&rhs){
			X -= rhs.X;
			Y -= rhs.Y;
			return *this;
		}
		CPoint& operator *=(const CPoint&rhs){
			X *= rhs.X;
			Y *= rhs.Y;
			return *this;
		}
		CPoint& operator /=(const CPoint&rhs){
			X /= rhs.X;
			Y /= rhs.Y;
			return *this;
		}
		CPoint& operator %=(const CPoint&rhs){
			X %= rhs.X;
			Y %= rhs.Y;
			return *this;
		}
		CPoint& operator <<=(const CPoint&rhs){
			X <<= rhs.X;
			Y <<= rhs.Y;
			return *this;
		}
		CPoint& operator >>=(const CPoint&rhs){
			X >>= rhs.X;
			Y >>= rhs.Y;
			return *this;
		}

		//=

		CPoint& operator +=(const int32_t&rhs){
			X += rhs;
			Y += rhs;
			return *this;
		}
		CPoint& operator -=(const int32_t&rhs){
			X -= rhs;
			Y -= rhs;
			return *this;
		}
		CPoint& operator *=(const int32_t&rhs){
			X *= rhs;
			Y *= rhs;
			return *this;
		}
		CPoint& operator /=(const int32_t&rhs){
			X /= rhs;
			Y /= rhs;
			return *this;
		}
		CPoint& operator %=(const int32_t&rhs){
			X %= rhs;
			Y %= rhs;
			return *this;
		}
		CPoint& operator <<=(const int32_t&rhs){
			X <<= rhs;
			Y <<= rhs;
			return *this;
		}
		CPoint& operator >>=(const int32_t&rhs){
			X >>= rhs;
			Y >>= rhs;
			return *this;
		}

		CPoint& operator +=(const float&rhs){
			X = (int32_t)((float)X + rhs);
			Y = (int32_t)((float)Y + rhs);
			return *this;
		}
		CPoint& operator -=(const float&rhs){
			X = (int32_t)((float)X - rhs);
			Y = (int32_t)((float)Y - rhs);
			return *this;
		}
		CPoint& operator *=(const float&rhs){
			X = (int32_t)((float)X * rhs);
			Y = (int32_t)((float)Y * rhs);
			return *this;
		}
		CPoint& operator /=(const float&rhs){
			X = (int32_t)((float)X / rhs);
			Y = (int32_t)((float)Y / rhs);
			return *this;
		}

		int32_t Dot(const CPoint &rhs){
			return (X*rhs.X) + (Y*rhs.Y);
		}

		static int32_t Dot(const CPoint &lhs, const CPoint &rhs){
			return (lhs.X*rhs.X) + (lhs.Y*rhs.Y);
		}
	};


	template<typename RT, typename C> 
	LRESULT UserCB0(C* pInstance, RT(C::*pCallback)()){
		return (LRESULT)(pInstance->*pCallback)();
	}

	template<typename RT, typename C, typename T0> 
	LRESULT UserCB1W(C* pInstance, RT(C::*pCallback)(T0), WPARAM wParam){
		return (LRESULT)(pInstance->*pCallback)((T0)wParam);
	}

	template<typename RT, typename C, typename T0> 
	LRESULT UserCB1L(C* pInstance, RT(C::*pCallback)(T0), LPARAM lParam){
		return (LRESULT)(pInstance->*pCallback)((T0)lParam);
	}

	template<typename RT, typename C, typename T0, typename T1> 
	LRESULT UserCB2(C* pInstance, RT(C::*pCallback)(T0, T1), WPARAM wParam, LPARAM lParam){
		return (LRESULT)(pInstance->*pCallback)((T0)wParam, (T1)lParam);
	}



	template<typename C> 
	LRESULT UserCB0v(C* pInstance, void (C::*pCallback)()){
		(pInstance->*pCallback)();
		return 0;
	}

	template<typename C, typename T0> 
	LRESULT UserCB1Wv(C* pInstance, void (C::*pCallback)(T0), WPARAM wParam){
		(pInstance->*pCallback)((T0)wParam);
		return 0;
	}

	template<typename C, typename T0> 
	LRESULT UserCB1Lv(C* pInstance, void (C::*pCallback)(T0), LPARAM lParam){
		(pInstance->*pCallback)((T0)lParam);
		return 0;
	}

	template<typename C, typename T0, typename T1> 
	LRESULT UserCB2v(C* pInstance, void (C::*pCallback)(T0, T1), WPARAM wParam, LPARAM lParam){
		(pInstance->*pCallback)((T0)wParam, (T1)lParam);
		return 0;
	}

	template<typename T>
	T tmax_3(T v0, T v1, T v2){
		if (v1 > v0){
			v0 = v1;
		}
		if (v2 > v0){
			v0 = v2;
		}
		return v0;
	}

	template<typename T> T tmin_3(T v0, T v1, T v2){
		if (v1 < v0){
			v0 = v1;
		}
		if (v2 < v0){
			v0 = v2;
		}
		return v0;
	}

	enum {
		ArrowDirection_Left = 0,
		ArrowDirection_Right,
		ArrowDirection_Top,
		ArrowDirection_Bottom,
	};

	class CArrowPainter {
	private:
		Gdiplus::SolidBrush*	m_pBrush = nullptr;
		Gdiplus::Pen*			m_pPen = nullptr;
	public:
		inline CArrowPainter(){
		}
		inline CArrowPainter(DWORD ColorFill, DWORD ColorOutline){
			m_pBrush = new Gdiplus::SolidBrush(ColorFill);
			m_pPen = new Gdiplus::Pen(ColorOutline);
		}
		inline ~CArrowPainter(){
			if (m_pBrush){
				delete m_pBrush;
			}
			if (m_pPen){
				delete m_pPen;
			}
		}
		inline void SetColors(DWORD ColorFill, DWORD ColorOutline){
			Gdiplus::Color color;
			if (!m_pBrush ||
				(m_pBrush->GetColor(&color) == Gdiplus::Ok && color.GetValue() != ColorFill)){
				if (m_pBrush){
					delete m_pBrush;
				}
				m_pBrush = new Gdiplus::SolidBrush(ColorFill);
			}
			if (!m_pPen ||
				(m_pPen->GetColor(&color) == Gdiplus::Ok && color.GetValue() != ColorOutline)){
				if (m_pPen){
					delete m_pPen;
				}
				m_pPen = new Gdiplus::Pen(ColorOutline);
			}
		}
		inline void DrawArrowDown(float x, float y, float size, Gdiplus::Graphics &graphics){
			Gdiplus::PointF points[3];

			points[0].X = (x - size);
			points[1].X = (x + size);
			points[2].X = (x);
			points[0].Y = points[1].Y = y;
			points[2].Y = y + size;

			if (m_pBrush){
				graphics.FillPolygon(m_pBrush, points, 3, Gdiplus::FillModeAlternate);
			}

			if (m_pPen){
				graphics.DrawPolygon(m_pPen, points, 3);
			}
		}
		void DrawArrowUp(float x, float y, float size, Gdiplus::Graphics &graphics){
			Gdiplus::PointF points[3];

			points[0].X = (x - size);
			points[1].X = (x + size);
			points[2].X = (x);
			points[0].Y = points[1].Y = y + size;
			points[2].Y = y;

			if (m_pBrush){
				graphics.FillPolygon(m_pBrush, points, 3, Gdiplus::FillModeAlternate);
			}

			if (m_pPen){
				graphics.DrawPolygon(m_pPen, points, 3);
			}
		}
		void DrawArrowLeft(float x, float y, float size, Gdiplus::Graphics  &graphics){
			Gdiplus::PointF points[3];

			points[0].Y = (y - size);
			points[1].Y = (y + size);
			points[2].Y = (y);
			points[0].X = points[1].X = x + size;
			points[2].X = x;

			if (m_pBrush){
				graphics.FillPolygon(m_pBrush, points, 3, Gdiplus::FillModeAlternate);
			}

			if (m_pPen){
				graphics.DrawPolygon(m_pPen, points, 3);
			}
		}
		void DrawArrowRight(float x, float y, float size, Gdiplus::Graphics  &graphics){
			Gdiplus::PointF points[3];

			points[0].Y = (y - size);
			points[1].Y = (y + size);
			points[2].Y = (y);
			points[0].X = points[1].Y = x;
			points[2].X = x + size;

			if (m_pBrush){
				graphics.FillPolygon(m_pBrush, points, 3, Gdiplus::FillModeAlternate);
			}

			if (m_pPen){
				graphics.DrawPolygon(m_pPen, points, 3);
			}
		}
	};

	CCRGB HSVToRGB(CCHSV hsv){
		CCRGB ret;
		float h, s, v, m, n, f;
		int32_t i;
		h = hsv.H*6.0f;
		s = hsv.S;
		v = hsv.V;


		i = (int32_t)floor(h);
		f = h - (float)i;

		if ((i & 1) == 0){
			f = 1 - f;
		}
		m = v*(1 - s);
		n = v*(1 - s*f);
		switch (i){
		case 6:
		case 0: RETURN_RGB(v, n, m);
		case 1: RETURN_RGB(n, v, m);
		case 2: RETURN_RGB(m, v, n);
		case 3: RETURN_RGB(m, n, v);
		case 4: RETURN_RGB(n, m, v);
		case 5: RETURN_RGB(v, m, n);
		}
		RETURN_RGB(v, v, v);
	}

	CCHSV RGBToHSV(CCRGB rgb){
		CCHSV ret;
		float r, g, b, v, x, f;
		float i;
		r = rgb.R;
		g = rgb.G;
		b = rgb.B;

		x = tmin_3(r, g, b);
		v = tmax_3(r, g, b);
		if (v == x) {
			RETURN_HSV(0, 0, v);
		}

		if (r == x){
			f = g - b;
		} else if (g == x){
			f = b - r;
		} else {
			f = r - g;
		}

		if (r == x){
			i = 3.0f;
		} else if (g == x){
			i = 5.0f;
		} else {
			i = 1.0f;
		}

		//f = (r == x) ? g - b : ((g == x) ? b - r : r - g);
		//i = (r == x) ? 3 : ((g == x) ? 5 : 1);
		RETURN_HSV((i - f / (v - x)) / 6.0f, (v - x) / v, v);
	}

	//RegisterDragDrop
	class CColorControl 
#if 0
: public IDropTarget {
	private:
		ULONG	m_RefCount = 1;
		~CColorControl(){
		}
	public:

		/* IUnknown */
		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void**ppvObject) override{
			if (ppvObject == nullptr){
				return E_INVALIDARG;
			}
			if (riid == IID_IUnknown){
				AddRef();
				*ppvObject = (void*)(IUnknown*)this;				
				return S_OK;
			}
			if (riid == IID_IDropTarget){
				AddRef();
				*ppvObject = (void*)(IDropTarget*)this;
				return S_OK;
			}
			return E_NOINTERFACE;
			NM_UPDOWN
		}

		virtual ULONG STDMETHODCALLTYPE AddRef(void) override{
			++m_RefCount;
		}

		virtual ULONG STDMETHODCALLTYPE Release(void) override{
			if (--m_RefCount == 0){
				delete this;
				return 0;
			}
			return m_RefCount;
		}
			
		/* IDropTarget */
		virtual HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *pDataObj,
			DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override{
		}

		virtual HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState,
			POINTL pt, DWORD *pdwEffect) override{
		}

		virtual HRESULT STDMETHODCALLTYPE DragLeave(void) override{
		}

		virtual HRESULT STDMETHODCALLTYPE Drop(IDataObject *pDataObj,
			DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override{
		}
#endif //

		{
		public:
		
		enum {
			//Arrow sizes for up/down, switch them for right/left
			Arrow_Size = 5,
			Arrow_SizeL = Arrow_Size * 2 + 1,
			//Spacing from rect to arrow
			Arrow_Spacing = 1,
			//Minimum size for rect
			Rect_MinW = 100,
			Rect_MinH = 10,
			//Handle size
			Handle_Height = 5,
			//Radius for the circle on colormap
			Circle_Radius = 3,
		};
		//Window handle of this control
		HWND				m_hWnd = nullptr;
		//Orientation of this control
		eCCOrientation		m_Orientation = eCCOrientation::Horizontal;
		//Type of this control
		eCCMode				m_Mode = eCCMode::Hue;
		//Cached bitmap for rainbow and hue type controls
		Gdiplus::Bitmap*	m_pBitmap = nullptr;
		Gdiplus::Graphics*	m_pGraphics = nullptr;
		bool				m_CacheValid = false;
		//Color ranges for gradient type controls
		CCRGB				m_ColorA = {};
		CCRGB				m_ColorB = {};
		//Control references for auto-controlling of entire picker dialogs
		CColorControl*		m_pControls[size_t(eCCSlot::Count)] = {};
		//Cached width/height
		int32_t				m_Width;
		int32_t				m_Height;
		//Helper to paint the arrows
		CArrowPainter		m_ArrowPainter;
		//Set when changing colors via mouse(/keyboard?)
		bool				m_Changing = false;
		//Arrow positions, 0 .. 1
		float				m_PosX = 0.0f;
		float				m_PosY = 0.0f;
		float				m_NewX = 0.0f;
		float				m_NewY = 0.0f;
		//Whether to use arrows or box handles
		bool				m_UseArrows = false;
		//Enable WM_NOTIFY
		bool				m_Notify = false;
		//Slot within the controller
		eCCSlot				m_Slot = eCCSlot::Controller;
		//Set when ControllerInitialize is called
		bool				m_IsController = false;
		//Paint state, cached values so they don't need to be recalculated every paint
		struct {
			int32_t		RectWidth, RectHeight;
			int32_t		RectX, RectY;
			int32_t		ArrowWidth, ArrowHeight;
			int32_t		ArrowX, ArrowY;
			int32_t		GradientHeight, GradientWidth;

			/*int32_t RectWidth, RectHeight, RectX, RectY;
			int32_t ArrowWidth, ArrowHeight, ArrowX, ArrowY;
			int32_t GradientHeight, GradientWidth;*/
		}					m_PS;
		CColorControl(HWND hWnd, LONG style) : m_hWnd(hWnd){
			RECT clientRect;
			GetClientRect(hWnd, &clientRect);
			m_Width = clientRect.right - clientRect.left;
			m_Height = clientRect.bottom - clientRect.top;
			m_ArrowPainter.SetColors(COLOR_CREF(GetSysColor(COLOR_WINDOW)),
				COLOR_CREF(GetSysColor(COLOR_WINDOWFRAME)));
			m_Changing = false;
			m_CacheValid = false;
			SetMode(eCCMode::Rainbow);
			if (style){
				m_UseArrows = CC_GETCOMPONENT(style, CC_ArrowShift, CC_ArrowMask);
				m_Notify = CC_GETCOMPONENT(style, CC_NotificationsShift, CC_NotificationsMask);
				m_Orientation = (eCCOrientation)CC_GETCOMPONENT(style, CC_OrientationShift, CC_OrientationMask);
				SetMode((eCCMode)CC_GETCOMPONENT(style, CC_ModeShift, CC_ModeMask));
			}
			InitializePaintState();
		}
		~CColorControl(){

		}
		CColorControl* GetControllerFromHWND(HWND hWnd){
			wchar_t lBuf[32];
			if (GetClassNameW(hWnd, lBuf, 32) == 0){
				return nullptr;
			}
			if (wcscmp(lBuf, COLOR_CLASS_NAME) != 0){
				return nullptr;
			}
			//CWindowStateAccessor<CColorControl> accessor(hWnd);
			return (CColorControl*)GetWindowLongPtrW(hWnd, 0);
		}
		void InitializePaintState(){
			if (m_UseArrows){
				switch (m_Mode){
				case eCCMode::Alpha:
				case eCCMode::Gradient:
				case eCCMode::Hue:
					if (m_Orientation == eCCOrientation::Horizontal){
						m_PS.RectWidth = m_Width - Arrow_Size * 2;
						m_PS.RectHeight = m_Height - Arrow_Size - Arrow_Spacing * 2;
						m_PS.GradientWidth = m_PS.RectWidth - 1;
						m_PS.GradientHeight = 0;
						m_PS.ArrowWidth = Arrow_Size;
						m_PS.ArrowHeight = Arrow_Size;
						m_PS.RectX = Arrow_Size;
						m_PS.RectY = 0;
						m_PS.ArrowX = m_PS.RectX;
						m_PS.ArrowY = m_PS.RectY + m_PS.RectHeight + Arrow_Spacing;

					} else {
						m_PS.RectHeight = m_Height - Arrow_Size * 2;
						m_PS.RectWidth = m_Width - Arrow_Size - Arrow_Spacing * 2;
						m_PS.GradientWidth = 0;
						m_PS.GradientHeight = m_PS.RectHeight - 1;


						m_PS.ArrowHeight = Arrow_Size;
						m_PS.ArrowWidth = Arrow_Size;
						m_PS.RectX = 0;
						m_PS.RectY = Arrow_Size;
						m_PS.ArrowX = m_PS.RectX + m_PS.RectWidth + Arrow_Spacing;
						m_PS.ArrowY = m_PS.RectY;//+(m_PS.RectHeight*(m_Changing ? m_NewX : m_PosX));
					}
					break;
				case eCCMode::Swatch:
					m_PS.RectX = 0;
					m_PS.RectY = 0;
					m_PS.RectWidth = m_Width;
					m_PS.RectHeight = m_Height;
					break;
				case eCCMode::Rainbow:
					m_PS.RectWidth = m_Width - Arrow_Size * 2 - Arrow_Spacing * 2;
					m_PS.RectHeight = m_Height - Arrow_Size * 2 - Arrow_Spacing * 2;
					m_PS.GradientWidth = m_PS.RectWidth - 1;
					m_PS.GradientHeight = 0;
					m_PS.ArrowWidth = Arrow_Size;
					m_PS.ArrowHeight = Arrow_Size;
					m_PS.RectX = Arrow_Size;
					m_PS.RectY = Arrow_Size;
					m_PS.ArrowX = m_PS.RectX;
					m_PS.ArrowY = m_PS.RectY;
					break;
				}
			} else {
				switch (m_Mode){
				case eCCMode::Alpha:
				case eCCMode::Gradient:
				case eCCMode::Hue:
					if (m_Orientation == eCCOrientation::Horizontal){
						m_PS.RectWidth = m_Width;
						m_PS.RectHeight = m_Height;
						m_PS.GradientWidth = m_PS.RectWidth - 1;
						m_PS.GradientHeight = 0;
						m_PS.ArrowWidth = Handle_Height;
						m_PS.ArrowHeight = m_PS.RectHeight;
						m_PS.RectX = 0;
						m_PS.RectY = 0;
						m_PS.ArrowX = 0;
						m_PS.ArrowY = 0;

					} else {
						m_PS.RectWidth = m_Width;
						m_PS.RectHeight = m_Height;
						m_PS.GradientWidth = 0;
						m_PS.GradientHeight = m_PS.RectHeight - 1;
						m_PS.ArrowWidth = m_PS.RectWidth;
						m_PS.ArrowHeight = Handle_Height;
						m_PS.RectX = 0;
						m_PS.RectY = 0;
						m_PS.ArrowX = 0;
						m_PS.ArrowY = 0;
					}
					break;
				case eCCMode::Swatch:
					m_PS.RectX = 0;
					m_PS.RectY = 0;
					m_PS.RectWidth = m_Width;
					m_PS.RectHeight = m_Height;
					break;
				case eCCMode::Rainbow:
					m_PS.RectX = 0;
					m_PS.RectY = 0;
					m_PS.RectWidth = m_Width;
					m_PS.RectHeight = m_Height;
					m_PS.ArrowX = 0;
					m_PS.ArrowY = 0;
					m_PS.GradientWidth = m_PS.RectWidth - 1;
					m_PS.GradientHeight = 0;
					break;
				}
			}
		}
		bool RegisterThis(CColorControl* pControl, eCCSlot slot){
			size_t i;
			if (!pControl ||
				int32_t(slot) < 0 || slot >= eCCSlot::Count){
				return false;
			}
			//Check if the control is already registered with this
			for (i = 0; i < size_t(eCCSlot::Count); i++){
				if (m_pControls[i] == pControl){
					return false;
				}
			}
			pControl->m_Slot = slot;
			m_pControls[size_t(slot)] = pControl;
			return true;
		}
		//Unregister a control from this
		void UnregisterThis(CColorControl* pControl){
			size_t i;
			for (i = 0; i < size_t(eCCSlot::Count); i++){
				if (m_pControls[i] == pControl){
					m_pControls[i] = nullptr;
				}
			}
		}
		void Unregister(){
			size_t i;
			for (i = 0; i < size_t(eCCSlot::Count); i++){
				if (m_pControls[i]){
					m_pControls[i]->UnregisterThis(this);
					m_pControls[i] = nullptr;
				}
			}
		}
		void FreeCache(){
			if (m_pGraphics){
				delete m_pGraphics;
			}
			if (m_pBitmap){
				delete m_pBitmap;
			}
			m_pGraphics = nullptr;
			m_pBitmap = nullptr;
			m_CacheValid = false;
		}
		float SaveGetComponent(eCCSlot slot){
			if (m_pControls[size_t(slot)]){
				return (m_pControls[size_t(slot)]->m_PosX);
			}
			return 0;
		}
		float SaveGetComponentFloat(eCCSlot slot){
			if (m_pControls[size_t(slot)]){
				return m_pControls[size_t(slot)]->m_PosX;
			}
			return 0.0f;
		}
		void GetColorRangeForSlot(CCRGB &colorA, CCRGB &colorB,
			const CCRGB &color, eCCSlot slot){
			switch (slot){
			case eCCSlot::Red:
				colorA = {0.0f, color.G, color.B};
				colorB = {1.0f, color.G, color.B};
				break;
			case eCCSlot::Green:
				colorA = {color.R, 0.0f, color.B};
				colorB = {color.R, 1.0f, color.B};
				break;
			case eCCSlot::Blue:
				colorA = {color.R, color.G, 0.0f};
				colorB = {color.R, color.G, 1.0f};
				break;
			}
		}
		void GetColorRangeForSlotHSV(CCRGB &colorA, CCRGB &colorB,
			float hue, float saturation, float value, eCCSlot slot){
			CCHSV hsv0, hsv1;
			hsv0.H = hsv1.H = hue;
			hsv0.S = hsv1.S = saturation;
			hsv0.V = hsv1.V = value;
			switch (slot){
			case eCCSlot::Sat:{
				hsv0.S = 0;
				hsv1.S = 1.0;
				break;
			}
			case eCCSlot::Value:{
				hsv0.V = 0;
				hsv1.V = 1.0;
				break;
			}
			}
			colorA = HSVToRGB(hsv0);
			colorB = HSVToRGB(hsv1);
		}
		void UpdateSlotColor(eCCSlot slot, const CCRGB &color){
			CCRGB colorA, colorB;
			CCHSV hsv;
			if (!m_pControls[size_t(slot)]){
				return;
			}
			if (slot >= eCCSlot::Red && slot <= eCCSlot::Blue){
				GetColorRangeForSlot(colorA, colorB, color, slot);
			} else if (slot >= eCCSlot::Hue && slot <= eCCSlot::Value){
				hsv = RGBToHSV(color);
				if (hsv.H == 0.0f){
					hsv.H = 1.0f;
				}
				GetColorRangeForSlotHSV(colorA, colorB, hsv.H, hsv.S, hsv.V, slot);
			}
			m_pControls[size_t(slot)]->SetColors(colorA, colorB);
		}
		void UpdateSlotColorHSV(eCCSlot slot, float hue, float saturation,
			float value){
			CCRGB colorA, colorB;
			CCRGB rgb;
			CCHSV hsv;
			if (!m_pControls[size_t(slot)]){
				return;
			}
			if (slot >= eCCSlot::Red && slot <= eCCSlot::Blue){
				hsv.H = hue;
				hsv.S = saturation;
				hsv.V = value;
				rgb = HSVToRGB(hsv);
				GetColorRangeForSlot(colorA, colorB, rgb, slot);
			} else if (slot >= eCCSlot::Hue && slot <= eCCSlot::Value){
				GetColorRangeForSlotHSV(colorA, colorB, hue,
					saturation, value, slot);
			} else if (slot == eCCSlot::Bar){
				GetColorRangeForSlotHSV(colorA, colorB, hue, saturation,
					value, eCCSlot::Sat);
			}
			m_pControls[size_t(slot)]->SetColors(colorA, colorB);
		}
		void UpdateSwatchColor(eCCSlot slot, const CCRGB &color){
			if (!m_pControls[size_t(slot)]){
				return;
			}
			m_pControls[size_t(slot)]->SetColors(color, {0.0f, 0.0f, 0.0f});
		}
		void SaveSetXValue(float val, eCCSlot slot){
			if (!m_pControls[size_t(slot)]){
				return;
			}
			m_pControls[size_t(slot)]->SetValues(&val, nullptr);
		}
		void SaveSetYValue(float val, eCCSlot slot){
			if (!m_pControls[size_t(slot)]){
				return;
			}
			m_pControls[size_t(slot)]->SetValues(nullptr, &val);
		}
		void SaveSetXYValue(float x, float y, eCCSlot slot){
			if (!m_pControls[size_t(slot)])
				return;
			m_pControls[size_t(slot)]->SetValues(&x, &y);
		}
		float SaveGetValueX(eCCSlot slot){
			float val;
			if (!m_pControls[size_t(slot)]){
				return 0.0f;
			}
			m_pControls[size_t(slot)]->GetValues(&val, nullptr);
			return val;
		}
		float SaveGetValueY(eCCSlot slot){
			float val;
			if (!m_pControls[size_t(slot)]){
				return 0.0f;
			}
			m_pControls[size_t(slot)]->GetValues(nullptr, &val);
			return val;
		}
		void ControlChanging(float newval, float oldval, eCCSlot slot,
			bool propagated = false){
			//float r, g, b;
			CCHSV hsv;
			CCRGB rgb = {};

			//r = g = b = 0;

			if (slot >= eCCSlot::Red && slot <= eCCSlot::Blue){
				rgb.R = SaveGetComponent(eCCSlot::Red);
				rgb.G = SaveGetComponent(eCCSlot::Green);
				rgb.B = SaveGetComponent(eCCSlot::Blue);
				switch (slot){
				case eCCSlot::Red:{
					rgb.R = newval;
					break;
				}
				case eCCSlot::Green:{
					rgb.G = newval;
					break;
				}
				case eCCSlot::Blue:{
					rgb.B = newval;
					break;
				}
				}
				hsv = RGBToHSV(rgb);
				SaveSetXValue(hsv.H, eCCSlot::Hue);
				SaveSetXValue(hsv.S, eCCSlot::Sat);
				SaveSetXValue(hsv.S, eCCSlot::Bar);
				SaveSetXValue(hsv.V, eCCSlot::Value);
			} else if ((slot >= eCCSlot::Hue && slot <= eCCSlot::Value) ||
				slot == eCCSlot::Bar){
				hsv.H = SaveGetValueX(eCCSlot::Hue);
				hsv.S = slot == eCCSlot::Bar ?
					SaveGetValueX(eCCSlot::Bar) : SaveGetValueX(eCCSlot::Sat);
				hsv.V = SaveGetValueX(eCCSlot::Value);
				switch (slot){
				case eCCSlot::Hue:{
					hsv.H = newval;
					break;
				}
				case eCCSlot::Bar:
				case eCCSlot::Sat:{
					hsv.S = newval;
					break;
				}
				case eCCSlot::Value:{
					hsv.V = newval;
					break;
				}
				}
				rgb = HSVToRGB(hsv);
				SaveSetXValue(rgb.R, eCCSlot::Red);
				SaveSetXValue(rgb.G, eCCSlot::Green);
				SaveSetXValue(rgb.B, eCCSlot::Blue);
				if (slot == eCCSlot::Bar){
					SaveSetXValue(hsv.S, eCCSlot::Sat);
				} else if (slot == eCCSlot::Sat){
					SaveSetXValue(hsv.S, eCCSlot::Bar);
				}
			} else if (slot == eCCSlot::Controller){
				hsv.H = m_NewX;
				hsv.V = 1.0f - m_NewY;
				hsv.S = SaveGetValueX(eCCSlot::Bar);
				SaveSetXValue(hsv.H, eCCSlot::Hue);
				SaveSetXValue(hsv.V, eCCSlot::Value);

				rgb = HSVToRGB(hsv);
				SaveSetXValue(rgb.R, eCCSlot::Red);
				SaveSetXValue(rgb.G, eCCSlot::Green);
				SaveSetXValue(rgb.B, eCCSlot::Blue);
				if (slot == eCCSlot::Bar){
					SaveSetXValue(hsv.S, eCCSlot::Sat);
				} else if (slot == eCCSlot::Sat){
					SaveSetXValue(hsv.S, eCCSlot::Bar);
				}

			}
			if (slot != eCCSlot::Controller){
				m_PosX = hsv.H;
				m_PosY = 1.0f - hsv.V;
			}
			Invalidate();

			m_ColorB = rgb;

			UpdateSwatchColor(eCCSlot::New, m_ColorB);
		}
		void ControlChanged(float newval, eCCSlot slot, bool propagated = false){
			//DWORD r, g, b;
			CCRGB colorA, colorB;
			size_t i;
			CCHSV hsv;
			CCRGB rgb;
			HWND hParent;
			bool isHsv = false;
			if (slot >= eCCSlot::Red && slot <= eCCSlot::Blue){
				rgb.R = SaveGetComponent(eCCSlot::Red);
				rgb.G = SaveGetComponent(eCCSlot::Green);
				rgb.B = SaveGetComponent(eCCSlot::Blue);
				switch (slot){
				case eCCSlot::Red:
					colorA = {0, rgb.G, rgb.B};
					colorB = {1.0f, rgb.G, rgb.B};
					UpdateSlotColor(eCCSlot::Green, rgb);
					UpdateSlotColor(eCCSlot::Blue, rgb);
					rgb.R = newval;
					break;
				case eCCSlot::Green:
					colorA = {rgb.R, 0.0f, rgb.B};
					colorB = {rgb.R, 1.0f, rgb.B};
					UpdateSlotColor(eCCSlot::Red, rgb);
					UpdateSlotColor(eCCSlot::Blue, rgb);
					rgb.G = newval;
					break;
				case eCCSlot::Blue:
					colorA = {rgb.R, rgb.G, 0.0f};
					colorB = {rgb.R, rgb.G, 1.0f};
					UpdateSlotColor(eCCSlot::Red, rgb);
					UpdateSlotColor(eCCSlot::Green, rgb);
					rgb.B = newval;
					break;
				}
				hsv = RGBToHSV(rgb);
				SaveSetXValue(hsv.H, eCCSlot::Hue);
				SaveSetXValue(hsv.S, eCCSlot::Sat);
				SaveSetXValue(hsv.V, eCCSlot::Value);
				for (i = size_t(eCCSlot::Red); i <= size_t(eCCSlot::Value); ++i){
					UpdateSlotColor(eCCSlot(i), rgb);
					UpdateSlotColorHSV(eCCSlot(i), hsv.H, hsv.S, hsv.V); //TODO: Can merge with above?
				}
				/*for (i = size_t(eCCSlot::Red); i <= size_t(eCCSlot::Value); i++){
				}*/
				UpdateSlotColorHSV(eCCSlot::Bar, hsv.H, hsv.S, hsv.V);

			} else if (slot >= eCCSlot::Hue && slot <= eCCSlot::Value ||
				slot == eCCSlot::Bar){
				hsv.H = SaveGetValueX(eCCSlot::Hue);
				hsv.S = slot == eCCSlot::Bar ? SaveGetValueX(eCCSlot::Bar) :
					SaveGetValueX(eCCSlot::Sat);
				hsv.V = SaveGetValueX(eCCSlot::Value);
				switch (slot){
				case eCCSlot::Hue:
					hsv.H = newval;
					break;
				case eCCSlot::Sat:
					hsv.S = newval;
					break;
				case eCCSlot::Value:
					hsv.V = newval;
					break;
				}
				isHsv = true;
			} else if (slot == eCCSlot::Controller){
				hsv.H = m_NewX;
				hsv.V = 1.0f - m_NewY;
				hsv.S = SaveGetValueX(eCCSlot::Bar);
				SaveSetXValue(hsv.H, eCCSlot::Hue);
				SaveSetXValue(hsv.V, eCCSlot::Value);
				isHsv = true;
			}

			if (isHsv){
				rgb = HSVToRGB(hsv);
				SaveSetXValue(rgb.R, eCCSlot::Red);
				SaveSetXValue(rgb.G, eCCSlot::Green);
				SaveSetXValue(rgb.B, eCCSlot::Blue);
				for (i = size_t(eCCSlot::Red); i <= size_t(eCCSlot::Value); i++){
					UpdateSlotColor(eCCSlot(i), rgb);
					UpdateSlotColorHSV(eCCSlot(i), hsv.H, hsv.S, hsv.V);
				}
				//for (i = size_t(eCCSlot::Red); i <= size_t(eCCSlot::Value); i++){
				//}
				UpdateSlotColorHSV(eCCSlot::Bar, hsv.H, hsv.S, hsv.V);
				if (slot == eCCSlot::Bar){
					SaveSetXValue(hsv.S, eCCSlot::Sat);
				} else if (slot == eCCSlot::Sat){
					SaveSetXValue(hsv.S, eCCSlot::Bar);
				}
			}

			if (slot != eCCSlot::Controller){

				m_PosX = hsv.H;
				m_PosY = 1.0f - hsv.V;
			}
			if (m_Notify && !propagated){
				hParent = GetParent(m_hWnd);
				if (hParent){
					NMCOLORCONTROL notification = {};
					notification.Hdr.code = CCN_CHANGED;
					notification.Hdr.hwndFrom = m_hWnd;
					notification.Hdr.idFrom = GetDlgCtrlID(m_hWnd);
					notification.NewColor = rgb;
					notification.OldColor = m_ColorA;
					notification.NewHSV = hsv;
					notification.OldHSV = RGBToHSV(m_ColorA);
					SendMessageW(hParent, WM_NOTIFY, (WPARAM)notification.Hdr.idFrom,
						(LPARAM)&notification);
				}
			}
			m_ColorB = m_ColorA = rgb;
			UpdateSwatchColor(eCCSlot::New, m_ColorA);
			UpdateSwatchColor(eCCSlot::Old, m_ColorA);
			InvalidateCache();
			Invalidate();
		}
		//Destroys the cache if size changed and type of hue or rainbow, it will be rebuild in the paint callback
		void SizeChanged(int32_t width, int32_t height){
			//Check if anything changed
			if (m_Width == width && m_Height == height){
				return;
			}
			if (m_Mode == eCCMode::Hue ||
				m_Mode == eCCMode::Rainbow){
				FreeCache();
			}
			m_Width = width;
			m_Height = m_Height;
			Invalidate();
			InitializePaintState();
			InvalidateCache();
		}
		void Invalidate(){
			InvalidateRect(m_hWnd, nullptr, FALSE);
			UpdateWindow(m_hWnd);
		}
		void InvalidateCache(){
			m_CacheValid = false;
		}
		bool SetMode(eCCMode mode){
			if (m_Mode == mode){
				return true;
			}
			if (int32_t(mode) < 0 || mode > eCCMode::Swatch)
				return false;
			FreeCache();
			m_Mode = mode;
			Invalidate();
			InitializePaintState();
			return true;
		}
		eCCMode GetMode(){
			return m_Mode;
		}
		/*void SetUseArrows(int32_t useArrows){

			if (m_UseArrows == use){
				return;
			}
			m_UseArrows = use;
			InitializePaintState();
		}*/
		void RepaintCache(){
			DWORD a;
			if (!m_pBitmap ||
				!m_pGraphics ||
				m_CacheValid)
				return; {
			}
			switch (m_Mode){
			case eCCMode::Hue:{
				Gdiplus::Color colors[] = {
					COLOR_XRGB(255, 0, 0),
					COLOR_XRGB(255, 255, 0),
					COLOR_XRGB(0, 255, 0),
					COLOR_XRGB(0, 255, 255),
					COLOR_XRGB(0, 0, 255),
					COLOR_XRGB(255, 0, 255),
					COLOR_XRGB(255, 0, 0),
				};
				float pos[] = {
					(float)(0.0f),
					(float)(1.0f / 360.0f*60.0f),
					(float)(1.0f / 360.0f*120.0f),
					(float)(1.0f / 360.0f*180.0f),
					(float)(1.0f / 360.0f*240.0f),
					(float)(1.0f / 360.0f*300.0f),
					(float)(1.0f / 360.0f*360.0f),
				};

				Gdiplus::LinearGradientBrush brush(Gdiplus::Point(0, 0),
					Gdiplus::Point(m_PS.RectWidth, 0), COLOR_XRGB(255, 255, 255),
					COLOR_XRGB(255, 255, 255));

				Gdiplus::SolidBrush brush2(COLOR_CREF(GetSysColor(COLOR_BTNSHADOW)));
				brush.SetInterpolationColors(colors, pos, 7);
				m_pGraphics->FillRectangle(&brush, 0, 0, m_PS.RectWidth, m_PS.RectHeight);
				break;
			}
			case eCCMode::Gradient:{
				Gdiplus::LinearGradientBrush brush(Gdiplus::Point(0, 0),
					Gdiplus::Point(m_PS.GradientWidth, m_PS.GradientHeight), m_ColorA.AsXRGB(), m_ColorB.AsXRGB());
				m_pGraphics->FillRectangle(&brush, 0, 0, m_PS.RectWidth, m_PS.RectHeight);
				break;
			}
			case eCCMode::Rainbow:{
				Gdiplus::Color colors[] = {
					COLOR_XRGB(255, 0, 0),
					COLOR_XRGB(255, 255, 0),
					COLOR_XRGB(0, 255, 0),
					COLOR_XRGB(0, 255, 255),
					COLOR_XRGB(0, 0, 255),
					COLOR_XRGB(255, 0, 255),
					COLOR_XRGB(255, 0, 0),
				};
				float pos[] = {
					0.0f,
					1.0f / 360.0f*60.0f,
					1.0f / 360.0f*120.0f,
					1.0f / 360.0f*180.0f,
					1.0f / 360.0f*240.0f,
					1.0f / 360.0f*300.0f,
					1.0f / 360.0f*360.0f,
				};

				if (m_pControls[size_t(eCCSlot::Sat)]){
					a = (DWORD)(255.0f * (1.0f - m_pControls[size_t(eCCSlot::Sat)]->m_PosX));
				} else{
					a = 0;
				}

				Gdiplus::LinearGradientBrush brush(Gdiplus::Point(0, 0),
					Gdiplus::Point(m_PS.RectWidth, 0), COLOR_XRGB(255, 255, 255),
					COLOR_XRGB(255, 255, 255));
				Gdiplus::LinearGradientBrush brush2(Gdiplus::Point(0, 0),
					Gdiplus::Point(0, m_PS.RectHeight - 1), COLOR_ARGB(a & 0xFF, 255, 255, 255),
					COLOR_ARGB(255, 0, 0, 0));

				brush.SetInterpolationColors(colors, pos, 7);
				m_pGraphics->FillRectangle(&brush, 0, 0, m_PS.RectWidth, m_PS.RectHeight);
				m_pGraphics->FillRectangle(&brush2, 0, 0, m_PS.RectWidth, m_PS.RectHeight);
				break;
			}
			case eCCMode::Alpha:{
				Gdiplus::LinearGradientBrush brush(Gdiplus::Point(0, 0),
					Gdiplus::Point(m_PS.GradientWidth, m_PS.GradientHeight), COLOR_ARGB(0, 0, 0, 0),
					COLOR_ARGB(255, 255, 255, 255));
				m_pGraphics->FillRectangle(&brush, 0, 0, m_PS.RectWidth, m_PS.RectHeight);
				break;
			}
			case eCCMode::Swatch:{
				Gdiplus::SolidBrush brush(m_ColorA.AsXRGB());
				m_pGraphics->FillRectangle(&brush, 0, 0, m_PS.RectWidth, m_PS.RectHeight);
				break;
			}
			}
		}
		void GenerateCache(){
			if (!m_pBitmap ||
				!m_pGraphics){
				FreeCache();
				m_pBitmap = new Gdiplus::Bitmap(m_PS.RectWidth, m_PS.RectHeight, PixelFormat32bppARGB);
				if (m_pBitmap){
					m_pGraphics = Gdiplus::Graphics::FromImage(m_pBitmap);
					if (!m_pGraphics){
						delete m_pBitmap;
						m_pBitmap = nullptr;
						return;
					}
				}
			}
			RepaintCache();
		}
		void PaintGeneric(Gdiplus::Graphics &graphics, RECT &rect, BOOL erase){
			int32_t arrowX, arrowY;
			/*
			Red
			Red, Green
			Green,
			Green, Blue,
			Blue,
			Red, Blue,
			Red
			*/
			bool error = false;
			if (m_Orientation == eCCOrientation::Horizontal){
				arrowX = m_PS.ArrowX + int32_t((m_PS.RectWidth - 1)*(m_Changing ? m_NewX : m_PosX));
				arrowY = m_PS.ArrowY;
				if (m_PS.RectWidth < Rect_MinW || m_PS.RectHeight < Rect_MinH){
					error = true;
				}

			} else {
				arrowY = m_PS.ArrowY + int32_t((m_PS.RectHeight - 1)*(m_Changing ? m_NewX : m_PosX));
				arrowX = m_PS.ArrowX;

				if (m_PS.RectHeight < Rect_MinW || m_PS.RectWidth < Rect_MinH){
					error = true;
				}
			}
			GenerateCache();
			//File entire thing red if cache still null or error
			if (!m_pBitmap ||
				!m_pGraphics ||
				error){
				Gdiplus::SolidBrush brush(RGB(255, 0, 0));
				graphics.FillRectangle(&brush, 0, 0, m_Width, m_Height);
			}
			//Gdiplus::SolidBrush brush(D3DCOLOR_CREF(GetSysColor(COLOR_BTNFACE)));
			Gdiplus::SolidBrush brush(COLOR_CREF(GetSysColor(COLOR_WINDOWFRAME)));
			if (m_UseArrows){
				graphics.FillRectangle(&brush, 0, 0, m_Width, m_Height);
			}

			graphics.DrawImage(m_pBitmap, m_PS.RectX, m_PS.RectY, m_PS.RectWidth, m_PS.RectHeight);
			if (m_Mode != eCCMode::Swatch){
				if (m_UseArrows){
					if (m_Orientation == eCCOrientation::Horizontal){
						m_ArrowPainter.DrawArrowUp(float(arrowX), float(arrowY), float(m_PS.ArrowWidth), graphics);
					} else{
						m_ArrowPainter.DrawArrowLeft(float(arrowX), float(arrowY), float(m_PS.ArrowWidth), graphics);
					}
				} else {
					if (arrowX < 0){
						arrowX = 0;
					} else if (arrowX + m_PS.ArrowWidth > m_PS.RectWidth + m_PS.RectX){
						arrowX = m_PS.RectWidth + m_PS.RectX - m_PS.ArrowWidth;
					}

					if (arrowY < 0){
						arrowY = 0;
					} else if (arrowY + m_PS.ArrowHeight > m_PS.RectHeight + m_PS.RectY){
						arrowY = m_PS.RectHeight + m_PS.RectY - m_PS.ArrowHeight;
					}
					Gdiplus::SolidBrush brushbg(COLOR_CREF(GetSysColor(COLOR_WINDOW)));
					//Gdiplus::SolidBrush brushbg(GetCurrentColor());
					Gdiplus::Pen pen(COLOR_CREF(GetSysColor(COLOR_WINDOWFRAME)));
					graphics.FillRectangle(&brushbg, arrowX, arrowY, m_PS.ArrowWidth, m_PS.ArrowHeight);
					graphics.DrawRectangle(&pen, arrowX, arrowY, m_PS.ArrowWidth - 1, m_PS.ArrowHeight - 1);
				}
			}
		}
		void PaintSwatch(Gdiplus::Graphics &gr, RECT &r, BOOL erase){
			Gdiplus::SolidBrush brush(m_ColorA.AsXRGB());
			gr.FillRectangle(&brush, 0, 0, m_Width, m_Height);
		}
		void PaintColorMap(Gdiplus::Graphics &graphics, RECT &rect, BOOL erase){
			int32_t arrowX, arrowY, cw;
			/*
			Red
			Red, Green
			Green,
			Green, Blue,
			Blue,
			Red, Blue,
			Red
			*/
			CCRGB rgb;
			CCHSV hsv;
			hsv.H = (m_Changing ? m_NewX : m_PosX);
			hsv.S = SaveGetValueX(eCCSlot::Sat);
			hsv.V = 1.0f - (m_Changing ? m_NewY : m_PosY);
			rgb = HSVToRGB(hsv);
			arrowX = m_PS.ArrowX + int32_t((m_PS.RectWidth)*(m_Changing ? m_NewX : m_PosX));
			arrowY = m_PS.ArrowY + int32_t((m_PS.RectHeight)*(m_Changing ? m_NewY : m_PosY));
			cw = Circle_Radius * 2 + 1;
			GenerateCache();
			//File entire thing red if cache still null
			if (!m_pBitmap || !m_pGraphics){
				Gdiplus::SolidBrush brush(RGB(255, 0, 0));
				graphics.FillRectangle(&brush, 0, 0, m_Width, m_Height);
			}
			Gdiplus::SolidBrush brush(COLOR_CREF(GetSysColor(COLOR_BTNFACE)));
			//Gdiplus::SolidBrush brush(D3DCOLOR_CREF(GetSysColor(COLOR_WINDOWFRAME)));
			if (m_UseArrows){
				graphics.FillRectangle(&brush, 0, 0, m_Width, m_Height);
			}
			graphics.DrawImage(m_pBitmap, m_PS.RectX, m_PS.RectY, m_PS.RectWidth, m_PS.RectHeight);
			if (m_UseArrows){
				m_ArrowPainter.DrawArrowUp(float(arrowX), float(m_PS.RectY + m_PS.RectHeight + Arrow_Spacing), 
					float(m_PS.ArrowWidth), graphics);
				m_ArrowPainter.DrawArrowLeft(float(m_PS.RectX + m_PS.RectWidth + Arrow_Spacing), float(arrowY), 
					float(m_PS.ArrowWidth), graphics);
			}

			Gdiplus::Region reg;

			graphics.GetClip(&reg);
			graphics.SetClip(Gdiplus::Rect(m_PS.RectX, m_PS.RectY, m_PS.RectWidth, m_PS.RectHeight));
			//Gdiplus::SolidBrush brushbg(D3DCOLOR_CREF(GetSysColor(COLOR_WINDOW)));
			//Gdiplus::SolidBrush brushbg(GetCurrentColor());

			Gdiplus::Pen pen(rgb.Lum() >= 0.5 ? COLOR_XRGB(0, 0, 0) : COLOR_XRGB(255, 255, 255));
			arrowX -= Circle_Radius + 1;
			arrowY -= Circle_Radius + 1;
			graphics.DrawEllipse(&pen, arrowX, arrowY, cw, cw);
			graphics.SetClip(&reg);
		}
		void InternalPaint(Gdiplus::Graphics &graphics, RECT &rect, BOOL erase){
			graphics.SetSmoothingMode(Gdiplus::SmoothingModeNone);
			switch (m_Mode){
			case eCCMode::Hue:
			case eCCMode::Gradient:
			case eCCMode::Alpha:{
				PaintGeneric(graphics, rect, erase);
				break;
			}
			case eCCMode::Swatch:{
				PaintSwatch(graphics, rect, erase);
				break;
			}
			case eCCMode::Rainbow:{
				PaintColorMap(graphics, rect, erase);
				break;
			}
			}
		}
		CCRGB LerpColor(const CCRGB &colorA, const CCRGB &colorB, float t){
			
			return{
				LERP_F32(colorA.R, colorB.R, t),
				LERP_F32(colorA.G, colorB.G, t),
				LERP_F32(colorA.B, colorB.B, t)
			};
		}
		CCRGB GetCurrentColor(){
			float xval, yval;

			int32_t i;
			CCRGB colors[] = {
				{1.0f, 0.0f, 0.0f},
				{1.0f, 1.0f, 0.0f},
				{0.0f, 1.0f, 0.0f},
				{0.0f, 1.0f, 1.0f},
				{0.0f, 0.0f, 1.0f},
				{1.0f, 0.0f, 1.0f},
				{1.0f, 0.0f, 0.0f},
			};

			xval = m_Changing ? m_NewX : m_PosX;
			yval = m_Changing ? m_NewY : m_PosY;
			if (xval < 0.0){
				xval = 0.0;
			} else if (xval > 1.0){
				xval = 1.0;
			}

			if (yval < 0.0){
				yval = 0.0;
			} else if (yval > 1.0){
				yval = 1.0;
			}

			if (m_Mode == eCCMode::Hue){
				yval = (xval *= 6.0);
				i = (int32_t)floor(xval);
				xval -= i;
				if (i < 6){
					return LerpColor(colors[i], colors[i + 1], xval);
				} else{
					return colors[6];
				}
			} else if (m_Mode == eCCMode::Alpha){
				return LerpColor(CCRGB{0.0f,0.0f,0.0f}, CCRGB{1.0f, 1.0f, 1.0f}, xval);
			} else if (m_Mode == eCCMode::Gradient){
				return LerpColor(m_ColorA, m_ColorB, xval);
			} else if (m_Mode == eCCMode::Rainbow){
			} else if (m_Mode == eCCMode::Swatch){
				return m_ColorA;
			}
			return {1.0f, 0.0f, 0.0f};
		}
		void Paint(){
			RECT updateRect, clientRect;
			Gdiplus::Graphics *pGraphics;
			Gdiplus::RectF localRect, tempRect;
			PAINTSTRUCT paintStruct;
			if (!GetUpdateRect(m_hWnd, &updateRect, FALSE)) {
				return;
			}
			if (BeginPaint(m_hWnd, &paintStruct) == 0){
				return;
			}
			{
				GetClientRect(m_hWnd, &clientRect);
				Gdiplus::Bitmap bitmap(clientRect.right - clientRect.left,
					clientRect.bottom - clientRect.top, PixelFormat32bppARGB);
				pGraphics = Gdiplus::Graphics::FromImage(&bitmap);
				if (pGraphics){
					InternalPaint(*pGraphics, paintStruct.rcPaint, paintStruct.fErase);

					Gdiplus::Graphics graphics(paintStruct.hdc);

					graphics.DrawImage(&bitmap, clientRect.left, clientRect.top,
						clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
					delete pGraphics;
				}

			}
			EndPaint(m_hWnd, &paintStruct);
		}
		void OnMouseWheel(short flags, CPoint point, int32_t delta){
		}

		void CalculateValues(CPoint point){
			int32_t i, w, h;
			if (m_Mode == eCCMode::Rainbow){
				w = m_PS.RectWidth - 1;
				h = m_PS.RectHeight - 1;
				if (m_UseArrows){
					m_NewX = 1.0f / (float)w*(float)(point.X - (Arrow_Size));
					m_NewY = 1.0f / (float)h*(float)(point.Y - (Arrow_Size));
				} else {
					m_NewX = 1.0f / (float)w*(float)(point.X);
					m_NewY = 1.0f / (float)h*(float)(point.Y);
				}
				Invalidate();

			} else {
				w = m_Width;
				if (m_Orientation == eCCOrientation::Vertical){
					i = point.X;
					point.X = point.Y;
					point.Y = i;
					w = m_Height;
				}
				if (m_UseArrows){
					w -= Arrow_Size * 2 + 1;
					m_NewX = 1.0f / (float)w*(float)(point.X - (Arrow_Size));
					//LogFormated(L"%f -> %d:%d\n", m_NewX, p.X-Arrow_Size, w);
				} else {
					w -= 1;
					m_NewX = 1.0f / (float)w*(float)(point.X);
					//LogFormated(L"%f -> %d:%d\n", m_NewX, p.X, w);
				}

				/*if (p.X < Arrow_Width/2)
					p.X = Arrow_Width/2;
				else if (p.X > w+Arrow_Width/2)
					p.X = w+Arrow_Width/2;*/
				Invalidate();
			}
			if (m_pControls[size_t(eCCSlot::Controller)]){
				m_pControls[size_t(eCCSlot::Controller)]->ControlChanging(m_NewX, m_PosX, m_Slot);
			} else if (m_IsController){
				ControlChanging(m_NewX, m_PosX, eCCSlot::Controller);
			}
		}

		void PropagateValues(){
			bool xChanged, yChanged;
			xChanged = m_PosX != m_NewX;
			yChanged = m_PosY != m_NewY;
			if (xChanged){
				m_PosX = m_NewX;
			}
			if (yChanged){
				m_PosY = m_NewY;
			}
			if (m_pControls[size_t(eCCSlot::Controller)])
				m_pControls[size_t(eCCSlot::Controller)]->ControlChanged(m_NewX, m_Slot);
			else if (m_IsController){
				ControlChanged(m_NewX, eCCSlot::Controller);
			}
		}

		void OnMouseMove(short flags, CPoint point){
			if (m_Mode == eCCMode::Swatch){
				return;
			}
			if (m_Changing){
				CalculateValues(point);
			}
		}

		void OnMouseDownL(short flags, CPoint point){
			RECT clipRect;
			if (m_Mode == eCCMode::Swatch){
				return;
			}
			GetWindowRect(m_hWnd, &clipRect);
			SetCapture(m_hWnd);
			m_Changing = true;
			CalculateValues(point);
			//ShowCursor(FALSE);

			if (m_Mode == eCCMode::Rainbow){
				if (m_UseArrows){
					clipRect.left += Arrow_Size;
					clipRect.right = clipRect.left + m_PS.RectWidth;
					clipRect.top += Arrow_Size;
					clipRect.bottom = clipRect.top + m_PS.RectHeight;
				}
			} else if (m_UseArrows) {
				if (m_Orientation == eCCOrientation::Horizontal){
					clipRect.left += Arrow_Size;
					clipRect.right = clipRect.left + m_PS.RectWidth;
				} else {
					clipRect.top += Arrow_Size;
					clipRect.bottom = clipRect.top + m_PS.RectHeight;
				}
			}

			ClipCursor(&clipRect);
		}

		void OnMouseUpL(short flags, CPoint point){
			if (m_Mode == eCCMode::Swatch){
				return;
			}
			if (m_Changing){
				//ShowCursor(TRUE);
				ReleaseCapture();
				PropagateValues();
				m_Changing = false;
				ClipCursor(nullptr);
			}
		}

		void OnMouseDoubleL(short flags, CPoint point){
			if (m_Mode == eCCMode::Swatch){
				HWND hParent = GetParent(m_hWnd);
				if (IsWindow(hParent)){
					SendMessageW(hParent, WM_COMMAND,
						MAKEWPARAM(GetDlgCtrlID(m_hWnd), 0),
						(LPARAM)m_hWnd);
				}
			}
		}

		void OnMouseDownR(short flags, CPoint point){
		}

		void OnMouseUpR(short flags, CPoint point){
		}

		void OnMouseDoubleR(short flags, CPoint point){
		}

		void OnMouseDownM(short flags, CPoint point){
		}

		void OnMouseUpM(short flags, CPoint point){
		}

		void OnMouseDoubleM(short flags, CPoint point){
		}
		void SetColors(const CCRGB& colorA, const CCRGB& colorB){
			m_ColorA = colorA;
			m_ColorB = colorB;
			Invalidate();
		}
		void SetColorsPtr(const CCRGB* pColorA, const CCRGB *pColorB){
			if (pColorA){
				m_ColorA = *pColorA;
			}
			if (pColorB){
				m_ColorB = *pColorB;
			}
			if (pColorA || pColorB){
				Invalidate();
			}
		}
		void GetColors(CCRGB *pColorA, CCRGB *pColorB){
			if (pColorA){
				*pColorA = m_ColorA;
			}
			if (pColorB){
				*pColorB = m_ColorB;
			}
		}
		void SetValues(float* pX, float *pY){
			float newX, newY;
			bool changed;
			changed = false;
			if (pX){
				newX = *pX;
				if (newX < 0.0f){
					newX = 0.0f;
				} else if (newX > 1.0f){
					newX = 1.0f;
				}
				changed = (m_PosX != newX);
				m_PosX = newX;
			}
			if (pY){
				newY = *pY;
				if (newY < 0.0f){
					newY = 0.0f;
				} else if (newY > 1.0f){
					newY = 1.0f;
				}
				changed |= (m_PosY != newY);
				m_PosY = newY;
			}
			if (changed){
				Invalidate();
			}
		}
		void GetValues(float* pX, float *pY){
			if (pX){
				*pX = m_PosX;
			}
			if (pY){
				*pY = m_PosY;
			}
		}
		void UpdateControls(){
		}
		bool SetController(HWND hController, eCCSlot slot){
			CColorControl* pController;
			if (int32_t(slot) < 0 || slot >= eCCSlot::Count){
				return false;
			}

			if (m_pControls[size_t(eCCSlot::Controller)]){
				m_pControls[size_t(eCCSlot::Controller)]->UnregisterThis(this);
			}
			m_pControls[size_t(eCCSlot::Controller)] = nullptr;

			if (!hController){
				return true;
			}
			pController = GetControllerFromHWND(hController);
			if (pController == nullptr){
				return false;
			}
			/*if (slot == Control_Controller){
				m_pControls[slot] = pCtrl;
				pCtrl->UpdateControls();
			}*/

			if (pController->RegisterThis(this, slot)){
				m_pControls[size_t(eCCSlot::Controller)] = pController;
				return true;
			}
			return false;
		}
		HWND GetController(){
			if (m_pControls[size_t(eCCSlot::Controller)]){
				return m_pControls[size_t(eCCSlot::Controller)]->m_hWnd;
			}
			return nullptr;
		}
		bool SetOrientation(eCCOrientation orientation){
			if (int32_t(orientation) < 0 || orientation > eCCOrientation::Vertical){
				return false;
			}
			if (orientation == m_Orientation){
				return true;
			}
			m_Orientation = orientation;
			InitializePaintState();
			Invalidate();
			return true;
		}
		eCCOrientation GetOrientation(){
			return m_Orientation;
		}
		bool SetUseArrows(int32_t useArrows){
			bool bUseArrows = useArrows != 0;
			if (bUseArrows == m_UseArrows){
				return true;
			}
			m_UseArrows = bUseArrows;
			InitializePaintState();
			Invalidate();
			return true;
		}
		bool GetUseArrows(){
			return m_UseArrows;
		}
		bool SetNotifyParent(int32_t notifyParent){
			m_Notify = notifyParent != 0;
			return true;
		}
		bool GetNotifyParent(){
			return m_Notify;
		}
		void NotifyParent(int32_t code){
			NMCOLORCONTROL notificationMessage;
			notificationMessage.Hdr.code = code;
			notificationMessage.Hdr.hwndFrom = m_hWnd;
			notificationMessage.Hdr.idFrom = GetDlgCtrlID(m_hWnd);
			notificationMessage.NewScalar = m_NewX;
			notificationMessage.OldScalar = m_NewY;
		}
		void ControllerInitialize(){
			CCHSV hsv;
			if (m_Mode != eCCMode::Rainbow){
				return;
			}
			UpdateSlotColor(eCCSlot::Red, m_ColorA);
			UpdateSlotColor(eCCSlot::Green, m_ColorA);
			UpdateSlotColor(eCCSlot::Blue, m_ColorA);

			hsv = RGBToHSV(m_ColorA);
			UpdateSlotColorHSV(eCCSlot::Hue, hsv.H, hsv.S, hsv.V);
			UpdateSlotColorHSV(eCCSlot::Sat, hsv.H, hsv.S, hsv.V);
			UpdateSlotColorHSV(eCCSlot::Value, hsv.H, hsv.S, hsv.V);
			UpdateSlotColorHSV(eCCSlot::Bar, hsv.H, hsv.S, hsv.V);
			m_PosX = hsv.H;
			m_PosY = 1.0f - hsv.V;
			m_ColorB = m_ColorA;
			UpdateSwatchColor(eCCSlot::New, m_ColorA);
			UpdateSwatchColor(eCCSlot::Old, m_ColorA);
			SaveSetXValue(hsv.H, eCCSlot::Hue);
			SaveSetXValue(hsv.S, eCCSlot::Sat);
			SaveSetXValue(hsv.S, eCCSlot::Bar);
			SaveSetXValue(hsv.V, eCCSlot::Value);

			SaveSetXValue(m_ColorA.R, eCCSlot::Red);
			SaveSetXValue(m_ColorA.G, eCCSlot::Green);
			SaveSetXValue(m_ColorA.B, eCCSlot::Blue);
			InvalidateCache();
			Invalidate();
			m_IsController = true;
		}
		void PropagateChanged(){
			if (m_pControls[size_t(eCCSlot::Controller)]){
				m_pControls[size_t(eCCSlot::Controller)]->ControlChanged(m_PosX, m_Slot, true);
			}
		}
#define CC_CB0(_msg, _func) case _msg: result = UserCB0(this, &CColorControl::_func); return true;
#define CC_CB1W(_msg, _func) case _msg: result = UserCB1W(this, &CColorControl::_func, wParam); return true;
#define CC_CB1L(_msg, _func) case _msg: result = UserCB1L(this, &CColorControl::_func, lParam); return true;
#define CC_CB2(_msg, _func) case _msg: result = UserCB2(this, &CColorControl::_func, wParam, lParam); return true;

#define CC_CB0V(_msg, _func) case _msg: result = UserCB0v(this, &CColorControl::_func); return true;
#define CC_CB1WV(_msg, _func) case _msg: result = UserCB1Wv(this, &CColorControl::_func, wParam); return true;
#define CC_CB1LV(_msg, _func) case _msg: result = UserCB1Lv(this, &CColorControl::_func, lParam); return true;
#define CC_CB2V(_msg, _func) case _msg: result = UserCB2v(this, &CColorControl::_func, wParam, lParam); return true;
		/*
			enum {
			ColorControl_Version = 1,
			CCM_SETCOLORS		= WM_USER,	//DWORD		Color A, color B
			CCM_GETCOLORS,					//DWORD		Color A, color B
			CCM_SETVALUES,					//Float*	ValueX, ValueY [0..1]
			CCM_GETVALUES,
			CCM_SETLVALUES,					//Double*	ValueX, ValueY [0..1]
			CCM_GETLVALUES,					//Double*	ValueX, ValueY [0..1]
			CCM_SETCONTROLLER,				//HWND		Controller -> bool
			CCM_GETCONTROLLER,				//			-> HWND Controller
			CCM_SETORIENTATION,				//Int		Orientation
			CCM_GETORIENTATION,				//			-> Int Orientation
			CCM_SETTYPE,					//Int		Type -> Bool success
			CCM_GETTYPE,					//			-> Int Type
			CCM_SETARROW,					//Int		EnableArrow
			CCM_GETARROW,					//			-> Int EnableArrow
			CCM_SETNOTIFY,					//Int		EnableNotify
			CCM_GETNOTIFY,					//			-> Int EnableNotify
		};*/
		bool UserMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &result){
			switch (uMsg){
				CC_CB2V(CCM_SETCOLORS, SetColorsPtr);
				CC_CB2V(CCM_GETCOLORS, GetColors);
				CC_CB2V(CCM_SETVALUES, SetValues);
				CC_CB2V(CCM_GETVALUES, GetValues);

				CC_CB2(CCM_SETCONTROLLER, SetController);
				CC_CB0(CCM_GETCONTROLLER, GetController);

				CC_CB1W(CCM_SETMODE, SetMode);
				CC_CB0(CCM_GETMODE, GetMode);

				CC_CB1W(CCM_SETORIENTATION, SetOrientation);
				CC_CB0(CCM_GETORIENTATION, GetOrientation);

				CC_CB1W(CCM_SETUSEARROWS, SetUseArrows);
				CC_CB0(CCM_GETUSEARROWS, GetUseArrows);

				CC_CB1W(CCM_SETNOTIFYPARENT, SetNotifyParent);
				CC_CB0(CCM_GETNOTIFYPARENT, GetNotifyParent);
				CC_CB0V(CCM_CONTROLLERINITIALIZE, ControllerInitialize);
				CC_CB0V(CCM_PROPAGATECHANGED, PropagateChanged);

			}
			return false;
		}
	};
};

static LRESULT CALLBACK ColorWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	LRESULT lResult;
	LPCREATESTRUCT pCS;
	CColorControl* pControl = (CColorControl*)GetWindowLongPtrW(hWnd, 0);
	CPoint point;
	if (uMsg >= WM_USER && pControl){
		lResult = 0;
		if (pControl->UserMsg(uMsg, wParam, lParam, lResult)){
			return lResult;
		} else{
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
		}
	}
	switch (uMsg){
	case WM_CREATE:{
		pCS = (LPCREATESTRUCT)lParam;

		pControl = new CColorControl(hWnd, pCS->style);
		if (pControl == nullptr){
			return -1;
		}
		SetWindowLongPtrW(hWnd, 0, (LONG_PTR)pControl);
		break;
	}
	case WM_PAINT:{
		if (pControl){
			pControl->Paint();
		}
		return 0;
	}
	case WM_MOUSEWHEEL:{
		if (pControl){
			point.X = GET_X_LPARAM(lParam);
			point.Y = GET_Y_LPARAM(lParam);
			pControl->OnMouseWheel(GET_KEYSTATE_WPARAM(wParam), point,
				(int32_t)GET_WHEEL_DELTA_WPARAM(wParam));
		}
		return 0;
	}					   
#define BUTTONMESSAGEHANDLER(_msg, _func) \
	case _msg: { \
		if (pControl){ \
		point.X = GET_X_LPARAM(lParam); \
		point.Y = GET_Y_LPARAM(lParam); \
		pControl->_func(LOWORD(wParam), point); \
		} \
		} \
		break;
					   BUTTONMESSAGEHANDLER(WM_MOUSEMOVE, OnMouseMove);

					   BUTTONMESSAGEHANDLER(WM_LBUTTONUP, OnMouseUpL);
					   BUTTONMESSAGEHANDLER(WM_LBUTTONDOWN, OnMouseDownL);
					   BUTTONMESSAGEHANDLER(WM_LBUTTONDBLCLK, OnMouseDoubleL);

					   BUTTONMESSAGEHANDLER(WM_RBUTTONUP, OnMouseUpR);
					   BUTTONMESSAGEHANDLER(WM_RBUTTONDOWN, OnMouseDownR);
					   BUTTONMESSAGEHANDLER(WM_RBUTTONDBLCLK, OnMouseDoubleR);

					   BUTTONMESSAGEHANDLER(WM_MBUTTONUP, OnMouseUpM);
					   BUTTONMESSAGEHANDLER(WM_MBUTTONDOWN, OnMouseDownM);
					   BUTTONMESSAGEHANDLER(WM_MBUTTONDBLCLK, OnMouseDoubleM);
	case WM_DESTROY:{
		if (pControl){
			delete pControl;
			SetWindowLongPtrW(hWnd, 0, (LONG_PTR)nullptr);
		}
	}
	default:{
		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
	}
	return 0;
};

bool RegisterColorClass(HINSTANCE hInstance){
	WNDCLASSEXW windowClass = {};

	windowClass.lpszClassName = COLOR_CLASS_NAMEW;
	windowClass.lpszMenuName = NULL;
	windowClass.cbSize = sizeof(WNDCLASSEXW);
	windowClass.style = CS_DBLCLKS;
	windowClass.lpfnWndProc = ColorWndProc;
	windowClass.hIcon = NULL;
	windowClass.hIconSm = NULL;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = sizeof(void*);
	windowClass.hbrBackground = nullptr;
	windowClass.hInstance = hInstance;
	return RegisterClassExW(&windowClass) != FALSE;
}

static void InitSpinner(HWND hDlg, int32_t spinnerId,
	int32_t editId, int32_t lowValue, int32_t highValue, int32_t value){
	HWND hSpinner, hEdit;
	hSpinner = GetDlgItem(hDlg, spinnerId);
	hEdit = GetDlgItem(hDlg, editId);
	if (!hSpinner || !hEdit){
		return;
	}
	SendMessageW(hSpinner, UDM_SETRANGE32, (WPARAM)lowValue, (WPARAM)highValue);
	SendMessageW(hSpinner, UDM_SETBUDDY, (WPARAM)hEdit, 0);
	SendMessageW(hSpinner, UDM_SETPOS32, 0, (LPARAM)value);
}

static void SetSpinner(HWND hDlg, int32_t spinnerId, int32_t value){
	SendMessageW(GetDlgItem(hDlg, spinnerId), UDM_SETPOS32, 0, (LPARAM)value);
}

static void HandleRGBNotification(int32_t type, HWND hDlg, int32_t editId,
	int32_t spinnerId, int32_t colorId){
	int32_t value;
	BOOL wasTranslated;
	float x;
	if (type == EN_KILLFOCUS){
		value = GetDlgItemInt(hDlg, editId, &wasTranslated, FALSE);
		if (!wasTranslated || value < 0){
			value = 0;
		} else if (wasTranslated && value > 255){
			value = 255;
		}
		SetSpinner(hDlg, spinnerId, value);
		x = 1.0f / 255.0f*(float)value;
		SendMessageW(GetDlgItem(hDlg, colorId), CCM_SETVALUES, (WPARAM)&x, 0);
	} else if (type == EN_CHANGE){
		value = GetDlgItemInt(hDlg, editId, &wasTranslated, FALSE);
		if (!wasTranslated || value < 0 || value > 255){
			return;
		}
		SetSpinner(hDlg, spinnerId, value);
		x = 1.0f / 255.0f*(float)value;
		SendMessageW(GetDlgItem(hDlg, colorId), CCM_SETVALUES, (WPARAM)&x, 0);
	}
	SendMessage(GetDlgItem(hDlg, colorId), CCM_PROPAGATECHANGED, 0, 0);
}

static void HandleSVNotification(int32_t type, HWND hDlg, int32_t editId,
	int32_t spinnerId, int32_t colorId){
	int32_t value;
	BOOL wasTranslated;
	float x;
	if (type == EN_KILLFOCUS){
		value = GetDlgItemInt(hDlg, editId, &wasTranslated, FALSE);
		if (!wasTranslated || value < 0){
			value = 0;
		} else if (wasTranslated && value > 100){
			value = 100;
		}
		SetSpinner(hDlg, spinnerId, value);
		x = 1.0f / 100.0f*(float)value;
		SendMessageW(GetDlgItem(hDlg, colorId), CCM_SETVALUES, (WPARAM)&x, 0);
	} else if (type == EN_CHANGE){
		value = GetDlgItemInt(hDlg, editId, &wasTranslated, FALSE);
		if (!wasTranslated || value < 0 || value > 100)
			return;
		SetSpinner(hDlg, spinnerId, value);
		x = 1.0f / 100.0f*(float)value;
		SendMessageW(GetDlgItem(hDlg, colorId), CCM_SETVALUES, (WPARAM)&x, 0);
	}
	SendMessage(GetDlgItem(hDlg, colorId), CCM_PROPAGATECHANGED, 0, 0);
}

void HandleHNotification(int32_t type, HWND hDlg, int32_t edit, int32_t spinner, int32_t color){
	int32_t value;
	BOOL wasTranslated;
	float x;
	if (type == EN_KILLFOCUS){
		value = GetDlgItemInt(hDlg, edit, &wasTranslated, FALSE);
		if (!wasTranslated || value < 0){
			value = 0;
		} else if (wasTranslated && value > 360){
			value = 360;
		}
		SetSpinner(hDlg, spinner, value);
		x = 1.0f / 360.0f*(float)value;
		SendMessageW(GetDlgItem(hDlg, color), CCM_SETVALUES, (WPARAM)&x, (LPARAM)nullptr);
	} else if (type == EN_CHANGE){
		value = GetDlgItemInt(hDlg, edit, &wasTranslated, FALSE);
		if (!wasTranslated || value < 0 || value > 360){
			return;
		}
		SetSpinner(hDlg, spinner, value);
		x = 1.0f / 360.0f*(float)value;
		SendMessageW(GetDlgItem(hDlg, color), CCM_SETVALUES, (WPARAM)&x, (LPARAM)nullptr);
	}
	SendMessage(GetDlgItem(hDlg, color), CCM_PROPAGATECHANGED, 0, 0);
}

void HandleControlChange(HWND hDlg, const NMCOLORCONTROL* pNotification){
	CCRGB rgb;
	CCHSV hsv;
	rgb = pNotification->NewColor;
	hsv = pNotification->NewHSV;
	SetSpinner(hDlg, IDC_R_SPIN, (int32_t)rgb.GetR());
	SetSpinner(hDlg, IDC_G_SPIN, (int32_t)rgb.GetG());
	SetSpinner(hDlg, IDC_B_SPIN, (int32_t)rgb.GetB());

	SetSpinner(hDlg, IDC_H_SPIN, (int32_t)(hsv.H * 360));
	SetSpinner(hDlg, IDC_S_SPIN, (int32_t)(hsv.S * 100));
	SetSpinner(hDlg, IDC_V_SPIN, (int32_t)(hsv.V * 100));
}

struct SColorDlgProcParam {
	bool	IsUnicode;
	union {
		CCSELECTCOLORA* pSelectColorA;
		CCSELECTCOLORW* pSelectColorW;
	};
};

static INT_PTR CALLBACK ColorDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
	LPNMHDR pHdr;
	HWND controller;


	CCRGB rgb;
	CCHSV hsv;
	switch (uMsg) {
	case WM_INITDIALOG:{
		controller = GetDlgItem(hDlg, IDC_C_CONTROL);
		SendMessageW(GetDlgItem(hDlg, IDC_C_BAR), CCM_SETCONTROLLER, (WPARAM)controller, (LPARAM)eCCSlot::Bar);

		SendMessageW(GetDlgItem(hDlg, IDC_C_R), CCM_SETCONTROLLER, (WPARAM)controller, (LPARAM)eCCSlot::Red);
		SendMessageW(GetDlgItem(hDlg, IDC_C_G), CCM_SETCONTROLLER, (WPARAM)controller, (LPARAM)eCCSlot::Green);
		SendMessageW(GetDlgItem(hDlg, IDC_C_B), CCM_SETCONTROLLER, (WPARAM)controller, (LPARAM)eCCSlot::Blue);

		SendMessageW(GetDlgItem(hDlg, IDC_C_H), CCM_SETCONTROLLER, (WPARAM)controller, (LPARAM)eCCSlot::Hue);
		SendMessageW(GetDlgItem(hDlg, IDC_C_S), CCM_SETCONTROLLER, (WPARAM)controller, (LPARAM)eCCSlot::Sat);
		SendMessageW(GetDlgItem(hDlg, IDC_C_V), CCM_SETCONTROLLER, (WPARAM)controller, (LPARAM)eCCSlot::Value);

		SendMessageW(GetDlgItem(hDlg, IDC_C_NEW), CCM_SETCONTROLLER, (WPARAM)controller, (LPARAM)eCCSlot::New);
		SendMessageW(GetDlgItem(hDlg, IDC_C_OLD), CCM_SETCONTROLLER, (WPARAM)controller, (LPARAM)eCCSlot::Old);
		

		SColorDlgProcParam* pParam = (SColorDlgProcParam*)lParam;

		SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);
		SendMessageW(controller, CCM_CONTROLLERINITIALIZE, 0, 0);
		SendMessageW(controller, CCM_SETNOTIFYPARENT, (WPARAM)1, 0);
		if (pParam->IsUnicode){
			SendMessageW(controller, CCM_SETCOLORS, (LPARAM)&pParam->pSelectColorW->Color, (LPARAM)nullptr);
			if (pParam->pSelectColorW->WindowTitle){
				SetWindowTextW(hDlg, pParam->pSelectColorW->WindowTitle);
			}
			rgb = pParam->pSelectColorW->Color;
		} else {
			SendMessageW(controller, CCM_SETCOLORS, (LPARAM)&pParam->pSelectColorA->Color, (LPARAM)nullptr);
			if (pParam->pSelectColorA->WindowTitle){
				SetWindowTextA(hDlg, pParam->pSelectColorA->WindowTitle);
			}
			rgb = pParam->pSelectColorA->Color;
		}
		hsv = RGBToHSV(rgb);
		InitSpinner(hDlg, IDC_H_SPIN, IDC_H_EDIT, 0, 360, (int32_t)(hsv.H * 360));
		InitSpinner(hDlg, IDC_S_SPIN, IDC_S_EDIT, 0, 100, (int32_t)(hsv.S * 100));
		InitSpinner(hDlg, IDC_V_SPIN, IDC_V_EDIT, 0, 100, (int32_t)(hsv.V * 100));
		InitSpinner(hDlg, IDC_R_SPIN, IDC_R_EDIT, 0, 255, (int32_t)(rgb.R * 255));
		InitSpinner(hDlg, IDC_G_SPIN, IDC_G_EDIT, 0, 255, (int32_t)(rgb.G * 255));
		InitSpinner(hDlg, IDC_B_SPIN, IDC_B_EDIT, 0, 255, (int32_t)(rgb.B * 255));
		break;
	}
	case WM_COMMAND:{
		switch (LOWORD(wParam)){
		case IDOK:{
			SendMessageW(GetDlgItem(hDlg, IDC_C_CONTROL), CCM_GETCOLORS, (WPARAM)&rgb, NULL);

			SColorDlgProcParam* pParam = (SColorDlgProcParam*)GetWindowLongPtrW(hDlg, GWLP_USERDATA);
			if (pParam){
				if (pParam->IsUnicode){
					pParam->pSelectColorW->Color = rgb;
				} else {
					pParam->pSelectColorA->Color = rgb;
				}
			}
			EndDialog(hDlg, 0);
			break;
		}
		case IDCANCEL:{
			EndDialog(hDlg, 1);
			break;
		}
		case IDC_R_EDIT:{
			HandleRGBNotification(HIWORD(wParam), hDlg, IDC_R_EDIT, IDC_R_SPIN, IDC_C_R);
			break;
		}
		case IDC_G_EDIT:{
			HandleRGBNotification(HIWORD(wParam), hDlg, IDC_G_EDIT, IDC_G_SPIN, IDC_C_G);
			break;
		}
		case IDC_B_EDIT:{
			HandleRGBNotification(HIWORD(wParam), hDlg, IDC_B_EDIT, IDC_B_SPIN, IDC_C_B);
			break;
		}
		case IDC_H_EDIT:{
			HandleHNotification(HIWORD(wParam), hDlg, IDC_H_EDIT, IDC_H_SPIN, IDC_C_H);
			break;
		}
		case IDC_S_EDIT:{
			HandleSVNotification(HIWORD(wParam), hDlg, IDC_S_EDIT, IDC_S_SPIN, IDC_C_S);
			break;
		}
		case IDC_V_EDIT:{
			HandleSVNotification(HIWORD(wParam), hDlg, IDC_V_EDIT, IDC_V_SPIN, IDC_C_V);
			break;
		}
		default:{
			break;
		}
		}
		break;
	}
	case WM_NOTIFY:{
		pHdr = (LPNMHDR)lParam;
		if (pHdr->idFrom == IDC_C_CONTROL){
			HandleControlChange(hDlg, (const NMCOLORCONTROL*)lParam);
		}
		break;
	}
	default:{
		return FALSE;
		break;
	}
	}
	return TRUE;
}

bool SelectColorA(CCSELECTCOLORA* pSelectColor, HINSTANCE hInstance, HWND hParent){
	INT_PTR returnValue;
	if (!pSelectColor || !hInstance){
		return false;
	}
	SColorDlgProcParam param = {
		false,
		pSelectColor
	};

	returnValue = DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_COLOR_SELECT), hParent,
		ColorDlgProc, (LPARAM)&param);
	return returnValue == 0;
}

bool SelectColorW(CCSELECTCOLORW* pSelectColor, HINSTANCE hInstance, HWND hParent){
	INT_PTR returnValue;
	if (!pSelectColor || !hInstance){
		return false;
	}
	SColorDlgProcParam param = {
		true
	};
	param.pSelectColorW = pSelectColor;

	returnValue = DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_COLOR_SELECT), hParent,
		ColorDlgProc, (LPARAM)&param);
	return returnValue == 0;
}
