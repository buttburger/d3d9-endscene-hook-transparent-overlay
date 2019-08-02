#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

DWORD dwEndscene_hook;
DWORD dwEndscene_ret;
DWORD *vTable;
LPD3DXFONT pFont;
int text;


HMODULE test;
HRSRC hResourceHandle;
DWORD nImageSize;
HGLOBAL hResourceInstance;
LPVOID pResourceData;


bool SpriteCreated0 = NULL;
LPDIRECT3DTEXTURE9 IMAGE0;
LPD3DXSPRITE SPRITE0;
D3DXVECTOR3 ImagePos0(50, 50, 0.0f);


VOID WriteText( LPDIRECT3DDEVICE9 pDevice, INT x, INT y, DWORD color, CHAR *text )
{    
    RECT rect;
    SetRect( &rect, x, y, x, y );
    pFont->DrawText( NULL, text, -1, &rect, DT_NOCLIP | DT_LEFT, color );
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
	if( pFont )
    {
        pFont->Release();
        pFont = NULL;
    }
    if( !pFont )
    {
        D3DXCreateFont( pDevice, 14,0,FW_BOLD,1,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH | FF_DONTCARE,"Arial",&pFont );
    }

	WriteText(pDevice, 15, 80, D3DCOLOR_ARGB(255,255,000,000), "My Private Hook");

	if (SpriteCreated0 == FALSE)
	{
		D3DXCreateTextureFromFile(pDevice, "test.png", &IMAGE0); //place png in game dir
		SpriteCreated0 = TRUE;
	}
	D3DXCreateSprite(pDevice, &SPRITE0);


	SPRITE0->Begin(D3DXSPRITE_ALPHABLEND);
	SPRITE0->Draw(IMAGE0, NULL, NULL, &ImagePos0, 0xFFFFFFFF);
	SPRITE0->End();
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

DWORD FindPattern(DWORD dwAddress,DWORD dwLen,BYTE *bMask,char * szMask)
{
    for(DWORD i=0; i<dwLen; i++)
        if(Mask((BYTE*)(dwAddress + i), bMask, szMask))
            return (DWORD)(dwAddress+i);
    return 0;
}

void MakeJMP(BYTE *pAddress, DWORD dwJumpTo, DWORD dwLen)
{
    DWORD dwOldProtect, dwBkup, dwRelAddr;
    VirtualProtect(pAddress, dwLen, PAGE_EXECUTE_READWRITE, &dwOldProtect);
    dwRelAddr = (DWORD) (dwJumpTo - (DWORD) pAddress) - 5;
    *pAddress = 0xE9;
    *((DWORD *)(pAddress + 0x1)) = dwRelAddr;
    for(DWORD x = 0x5; x < dwLen; x++) *(pAddress + x) = 0x90;
	VirtualProtect(pAddress, dwLen, dwOldProtect, &dwBkup);
    return;
}

void MyHook(void)
{
    DWORD hD3D = NULL;
    while (!hD3D) hD3D = (DWORD)GetModuleHandle("d3d9.dll");

    DWORD PPPDevice = FindPattern(hD3D, 0x128000, (PBYTE)"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86", "xx????xx????xx");
    WriteMemory( &vTable, (void *)(PPPDevice + 2), 4);
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
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)MyHook, NULL, NULL, NULL);
    }
    return TRUE;
}