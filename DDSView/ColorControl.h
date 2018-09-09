#ifndef _COLORCONTROL_H_
#define _COLORCONTROL_H_
#include <Windows.h>
#include <stdlib.h>
#include <math.h>

#define COLOR_CLASS_NAMEA		("ColorControl")
#define COLOR_CLASS_NAMEW		(L"ColorControl")

#ifdef _UNICODE
#define COLOR_CLASS_NAME		COLOR_CLASS_NAMEW
#else //_UNICODE
#define COLOR_CLASS_NAME		COLOR_CLASS_NAMEA
#endif //!_UNICODE

#define COLOR_ARGB(a,r,g,b) ((DWORD)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#define COLOR_RGBA(r,g,b,a) COLOR_ARGB(a,r,g,b)
#define COLOR_XRGB(r,g,b)   COLOR_ARGB(0xff,r,g,b)



#define CCN_CHANGING	(0U-0U)
#define CCN_CHANGED		(0U-1U)

#define CCM_MSG(_idx)	(WM_USER+(_idx))

enum {
	ColorControl_Version = 1,
	CCM_SETCOLORS		= WM_USER,	//DWORD				Color A, color B
	CCM_GETCOLORS,					//DWORD				Color A, color B
	CCM_SETVALUES,					//Float*			ValueX, ValueY [0..1]
	CCM_GETVALUES,					//Float*			ValueX, ValueY [0..1]
	CCM_SETCONTROLLER,				//HWND				Controller, int slot
	CCM_GETCONTROLLER,				//HWND				:Controller
	CCM_SETORIENTATION,				//eCCOrientation	Orientation
	CCM_GETORIENTATION,				//eCCOrientation	:Orientation
	CCM_SETMODE,					//eCCMode			Mode
	CCM_GETMODE,					//eCCMode			:Mode
	CCM_SETUSEARROWS,				//Int				EnableArrow
	CCM_GETUSEARROWS,				//Int				:EnableArrow
	CCM_SETNOTIFYPARENT,			//Int				EnableNotify
	CCM_GETNOTIFYPARENT,			//Int				:EnableNotify
	CCM_CONTROLLERINITIALIZE,		//
	CCM_PROPAGATECHANGED,
};


struct CCRGB {
	float	R;
	float	G;
	float	B;
	inline DWORD AsRGB() const{
		return COLOR_ARGB(0, (DWORD)(R * 255), (DWORD)(G * 255), (DWORD)(B * 255));
	}
	inline DWORD AsXRGB() const{
		return COLOR_ARGB(255, (DWORD)(R * 255), (DWORD)(G * 255), (DWORD)(B * 255));
	}
	inline DWORD GetR() const{
		return (DWORD)(R*255.0);
	}
	inline DWORD GetG() const{
		return (DWORD)(G*255.0);
	}
	inline DWORD GetB() const{
		return (DWORD)(B*255.0);
	}
	inline float Lum() const{
		return (0.299f*R + 0.587f*G + 0.114f*B);
	}
	void FromDWORD(DWORD d){
		R = 1.0f / 255.0f*(float)((d >> 16) & 0xFF);
		G = 1.0f / 255.0f*(float)((d >> 8) & 0xFF);
		B = 1.0f / 255.0f*(float)((d) & 0xFF);
	}
	//CCRGB &operator=(const CCRGB &
};

struct CCHSV {
	float	H;
	float	S;
	float	V;
};

struct NMCOLORCONTROL {
	NMHDR	Hdr;
	CCRGB	NewColor;
	CCRGB	OldColor;
	CCHSV	NewHSV;
	CCHSV	OldHSV;
	float	NewScalar;
	float	OldScalar;
};


enum class eCCOrientation {
	Horizontal = 0,
	Vertical,
};

enum class eCCSlot {
	Controller = 0,
	Bar,
	Red,
	Green,
	Blue,
	Hue,
	Sat,
	Value,
	New,
	Old,
	Count,
};

/*
Mode of this color control
*/

enum class eCCMode {
	Hue = 0,
	Gradient,
	Rainbow,
	Alpha,
	Swatch,
};


enum {
	CC_ModeMask = 0xFF,
	CC_ModeShift = 0,
	CC_ArrowMask = 1,
	CC_ArrowShift = 8,
	CC_OrientationMask = 1,
	CC_OrientationShift = 9,
	CC_NotificationsMask = 1,
	CC_NotificationsShift = 10,
};

#define CC_COMPONENT(_val, _shift, _mask) (((_val) & (_mask)) << _(shift))
#define CC_GETCOMPONENT(_style, _shift, _mask) (((_style) >> (_shift)) & (_mask))

#define COLOR_CREF(c) (COLOR_XRGB((c) & 0xFF, ((c) >> 8) & 0xFF, ((c) >> 16) & 0xFF))
#define COLOR_ACREF(a, c) (COLOR_ARGB(a, (c) & 0xFF, ((c) >> 8) & 0xFF, ((c) >> 16) & 0xFF)

#define GET_R(c) (((c)) & 0xFF)
#define GET_G(c) (((c) >> 8) & 0xFF)
#define GET_B(c) (((c) >> 16) & 0xFF)
#define COLOR_TO_FLOAT(c) ((float)(1.0f/255.0f*(float)(c)))

#define COLORCONTROL_COMPOSESTYLE(_arrow, _orientation, _type, _notfications) (CC_COMPONENT((_type), CC_TypeShift, CC_TypeMask) | CC_COMPONENT((_arrow), CC_ArrowShift, CC_ArrowMask) | CC_COMPONENT((_orientation), CC_OrientationShift, CC_OrientationMask) | CC_COMPONENT((_notifications), CC_NotificationsShift, CC_NotificationsMask))


struct CCHWB {
	float	H;
	float	W;
	float	B;
};

bool RegisterColorClass(HINSTANCE hInstance);

struct CCSELECTCOLORA {
	const char*		WindowTitle;	//Title for the dialog, optional
	CCRGB			Color;			//Input/output color
};
struct CCSELECTCOLORW {
	const wchar_t*	WindowTitle;	//Title for the dialog, optional
	CCRGB			Color;			//Input/output color
};

bool SelectColorA(CCSELECTCOLORA* pSelectColor, HINSTANCE hInstance, HWND hParent);
bool SelectColorW(CCSELECTCOLORW* pSelectColor, HINSTANCE hInstance, HWND hParent);

#endif //!_COLORCONTROL_H_
