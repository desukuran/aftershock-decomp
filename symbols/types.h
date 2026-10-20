/* Data types applied to the Ghidra program by tools/ghidra_scripts/ApplyTypes.java.
 * Plain C only (Ghidra's C parser). Layouts are MSVC 7.1 / XDK 5558, 32-bit. */

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef int BOOL;
typedef void *HANDLE;
typedef long HRESULT;

typedef struct SYSTEMTIME {
	WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds;
} SYSTEMTIME;

typedef struct FILETIME {
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
} FILETIME;

/* std::basic_string<char> (Dinkumware, VC7.1): small-string buffer of 16. 0x1c bytes */
typedef struct std_string {
	BYTE _Alval[4];
	union {
		char _Buf[16];
		char *_Ptr;
	} _Bx;
	DWORD _Mysize;
	DWORD _Myres;   /* < 16 => data is in _Buf */
} std_string;

/* std::basic_string<wchar_t>: small-string buffer of 8 wchar_t. 0x1c bytes */
typedef struct std_wstring {
	BYTE _Alval[4];
	union {
		unsigned short _Buf[8];
		unsigned short *_Ptr;
	} _Bx;
	DWORD _Mysize;
	DWORD _Myres;   /* < 8 => data is in _Buf */
} std_wstring;

/* D3DX math (d3dx8math.h) */
typedef struct D3DXVECTOR2 { float x, y; } D3DXVECTOR2;
typedef struct D3DXVECTOR3 { float x, y, z; } D3DXVECTOR3;
typedef struct D3DXVECTOR4 { float x, y, z, w; } D3DXVECTOR4;
typedef struct D3DXQUATERNION { float x, y, z, w; } D3DXQUATERNION;
typedef struct D3DXMATRIX { float m[4][4]; } D3DXMATRIX;
typedef struct D3DCOLORVALUE { float r, g, b, a; } D3DCOLORVALUE;

/* D3DLIGHT8, 0x68 bytes */
typedef struct D3DLIGHT8 {
	DWORD Type;
	D3DCOLORVALUE Diffuse;
	D3DCOLORVALUE Specular;
	D3DCOLORVALUE Ambient;
	D3DXVECTOR3 Position;
	D3DXVECTOR3 Direction;
	float Range, Falloff, Attenuation0, Attenuation1, Attenuation2, Theta, Phi;
} D3DLIGHT8;

/* XINPUT_GAMEPAD (xbox.h), 0x12 bytes; packed in XINPUT_STATE */
#pragma pack(push, 1)
typedef struct XINPUT_GAMEPAD {
	WORD wButtons;
	BYTE bAnalogButtons[8];
	short sThumbLX, sThumbLY, sThumbRX, sThumbRY;
} XINPUT_GAMEPAD;
typedef struct XINPUT_STATE {
	DWORD dwPacketNumber;
	XINPUT_GAMEPAD Gamepad;
} XINPUT_STATE;
typedef struct XINPUT_CAPABILITIES {
	BYTE SubType;
	WORD Reserved;
	XINPUT_GAMEPAD In;
	WORD OutRumble[2];
} XINPUT_CAPABILITIES;
#pragma pack(pop)

/* The XDK samples' XBGAMEPAD (xbinput.h), 0xdc bytes */
typedef struct XBGAMEPAD {
	XINPUT_GAMEPAD gp;
	BYTE _pad12[2];
	float fX1, fY1, fX2, fY2;
	WORD wLastButtons;
	BYTE _pad26[2];
	BOOL bLastAnalogButtons[8];
	WORD wPressedButtons;
	BYTE _pad4a[2];
	BOOL bPressedAnalogButtons[8];
	WORD Rumble[2];
	BYTE Feedback[0x46];
	XINPUT_CAPABILITIES caps;
	BYTE _padcf[1];
	HANDLE hDevice;
	BOOL bInserted;
	BOOL bRemoved;
} XBGAMEPAD;
