#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>

#include "colors.h"
#include "draw.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

DWORD dwEndscene_hook;
DWORD dwEndscene_ret;
DWORD*vTable;
LPD3DXFONT pFont;

int text;

CDraw Draw;

VOID WriteText(LPDIRECT3DDEVICE9 pDevice, INT x, INT y, DWORD color, CHAR*text)
{
	RECT rect;
	SetRect(&rect, x, y, x, y);
	pFont->DrawText(NULL, text, -1, &rect, DT_NOCLIP|DT_LEFT, color);
}

void DrawFillRect(IDirect3DDevice9*dev, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b)
{
	D3DCOLOR rectColor = D3DCOLOR_XRGB(r, g, b);
	D3DRECT BarRect = { x, y, x + w, y + h }; 
	dev->Clear(1,&BarRect,  D3DCLEAR_TARGET | D3DCLEAR_TARGET , rectColor, 0, 0);
}

void WriteMemory(void *address, void *bytes, int byteSize)
{
	DWORD NewProtection;
	VirtualProtect(address, byteSize, PAGE_EXECUTE_READWRITE, &NewProtection);
	memcpy(address, bytes, byteSize);
	VirtualProtect(address, byteSize, NewProtection, &NewProtection);
}

VOID WINAPI EndScene(LPDIRECT3DDEVICE9 pDevice)
{
	Draw.GetDevice(pDevice);
	Draw.Reset();

	if(pFont)
	{
		pFont->Release();
		pFont = NULL;
	}
	if(!pFont)
	{
		D3DXCreateFont(pDevice, 14, 0, FW_BOLD, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, "Arial", &pFont);
	}
	WriteText(pDevice, 15, 80, D3DCOLOR_RGBA(255, 000, 000, 255), "My Private Hook");
	//DrawFillRect(pDevice, 100, 100, 100, 100, 0, 255, 0); //Alpha don't work

	if(Draw.Font()) Draw.OnLostDevice();
	//Draw.CircleFilled(Draw.Screen.x_center, Draw.Screen.y_center, 100, 0, full, 32, LAWNGREEN(255));
	//Draw.Circle(Draw.Screen.x_center, Draw.Screen.y_center, 100, 0, full, true, 32, BLACK(255));
	//
	Draw.BoxRounded(Draw.Screen.x_center, Draw.Screen.y_center, 200, 200, 9, 1, PINK(123), ORANGE(255));
	Draw.Box(Draw.Screen.x_center, Draw.Screen.y_center, 200, 200, 0, 0);
}
__declspec(naked) void MyEndscene()
{
	static LPDIRECT3DDEVICE9 pDevice;
	__asm
	{
		mov dword ptr ss:[ebp - 10], esp;
		mov esi, dword ptr ss:[ebp + 0x8];
		mov pDevice, esi;
	}
	EndScene(pDevice);
	__asm
	{
		jmp dwEndscene_ret;
	}
}
bool Mask(const BYTE* pData, const BYTE* bMask, const char* szMask)
{
    for(;*szMask;++szMask,++pData,++bMask)
        if(*szMask=='x' && *pData!=*bMask ) 
            return false;
    return (*szMask) == NULL;
}
DWORD FindPattern(DWORD dwAddress,DWORD dwLen, BYTE*bMask, char*szMask)
{
	for(DWORD i=0; i<dwLen; i++)if(Mask((BYTE*)(dwAddress + i), bMask, szMask))return(DWORD)(dwAddress+i);
	return 0;
}
void MakeJMP(BYTE*pAddress, DWORD dwJumpTo, DWORD dwLen)
{
    DWORD dwOldProtect, dwBkup, dwRelAddr;
    VirtualProtect(pAddress, dwLen, PAGE_EXECUTE_READWRITE, &dwOldProtect);
    dwRelAddr = (DWORD)(dwJumpTo - (DWORD) pAddress) - 5;
    *pAddress = 0xE9;
    *((DWORD *)(pAddress + 0x1)) = dwRelAddr;
    for(DWORD x = 0x5; x < dwLen; x++) *(pAddress + x) = 0x90;
	VirtualProtect(pAddress, dwLen, dwOldProtect, &dwBkup);
    return;
}
void MyHook(void)
{
	DWORD hD3D = NULL;
	while(!hD3D) hD3D = (DWORD)GetModuleHandle("d3d9.dll");
	DWORD PPPDevice = FindPattern(hD3D, 0x128000, (PBYTE)"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86", "xx????xx????xx");
	WriteMemory(&vTable, (void *)(PPPDevice + 2), 4);
	dwEndscene_hook = vTable[42] + 0x2A;
	dwEndscene_ret = dwEndscene_hook + 0x6;
	while(1)
	{
		Sleep(100);
		MakeJMP((PBYTE)dwEndscene_hook, (DWORD)MyEndscene, 6);
	}
}
BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpvReserved)
{
	if(dwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)MyHook, NULL, NULL, NULL);
	}
	return TRUE;
}