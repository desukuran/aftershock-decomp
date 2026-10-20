// Device setup, texture loading and the 2D helpers.
#include "Game.h"

LPDIRECT3DTEXTURE8 g_BallTextures[16];      // Ball0..15.tga
LPDIRECT3DTEXTURE8 g_RedBallTexture;        // the Red vs Blue and cut throat balls
LPDIRECT3DTEXTURE8 g_BlueBallTexture;
LPDIRECT3DTEXTURE8 g_BlackBallTexture;

// A screen-space vertex for DrawRect.
struct SCREENVERTEX
{
    float x, y, z, rhw;
    D3DCOLOR color;
    float tu, tv;
};
#define D3DFVF_SCREENVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

// Most textures load the same way: full size, managed, magenta transparent.
static void LoadTexture(const char* strFile, LPDIRECT3DTEXTURE8* ppTexture)
{
    D3DXCreateTextureFromFileExA(g_Game.device, strFile, 0, 0, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
                                 D3DX_DEFAULT, D3DX_DEFAULT, 0xFFFF00FF, NULL, NULL, ppTexture);
}

// 0x00016120
void InitD3D()
{
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.BackBufferWidth = 640;
    d3dpp.BackBufferHeight = 480;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    d3dpp.BackBufferCount = 0;
    d3dpp.MultiSampleType = D3DMULTISAMPLE_2_SAMPLES_MULTISAMPLE_LINEAR;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_F24S8;
    d3dpp.FullScreen_PresentationInterval = 0;
    Direct3D_CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING,
                          &d3dpp, &g_Game.device);

    LPDIRECT3DDEVICE8 pDevice = g_Game.device;
    pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    pDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_ANISOTROPIC);
    pDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_ANISOTROPIC);
    pDevice->SetTextureStageState(0, D3DTSS_MAXANISOTROPY, 2);
    pDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);

    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 4.0f / 3.0f, 1.0f, 1000.0f);
    pDevice->SetTransform(D3DTS_PROJECTION, &matProj);

    D3DXMATRIX matView;
    D3DXVECTOR3 vEye(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vAt(0.0f, 0.0f, 1.0f);
    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);
    D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
    pDevice->SetTransform(D3DTS_VIEW, &matView);

    pDevice->SetRenderState(D3DRS_AMBIENT, 0x707070);
    pDevice->SetFlickerFilter(0);
    pDevice->SetTextureStageState(0, D3DTSS_MAXANISOTROPY, 2);
    pDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
    pDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_ANISOTROPIC);
    pDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_ANISOTROPIC);
    pDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
}

// 0x00016350
void LoadBallTextures()
{
    char strFile[256];
    for (int i = 0; i < 16; i++)
    {
        sprintf(strFile, "D:\\media\\balls\\Ball%i.tga", i);
        if (FAILED(D3DXCreateTextureFromFileExA(g_Game.device, strFile, 0, 0, 0, 0, D3DFMT_UNKNOWN,
                D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &g_BallTextures[i])))
            OutputDebugStringA("fucked up loading ball texture");
    }
    D3DXCreateTextureFromFileExA(g_Game.device, "D:\\media\\balls\\red.tga", 0, 0, 0, 0, D3DFMT_UNKNOWN,
                                 D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &g_RedBallTexture);
    D3DXCreateTextureFromFileExA(g_Game.device, "D:\\media\\balls\\blue.tga", 0, 0, 0, 0, D3DFMT_UNKNOWN,
                                 D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &g_BlueBallTexture);
    D3DXCreateTextureFromFileExA(g_Game.device, "D:\\media\\balls\\black.tga", 0, 0, 0, 0, D3DFMT_UNKNOWN,
                                 D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &g_BlackBallTexture);
}

// 0x00016990
void LoadTextures()
{
    char strFile[100];

    LoadTexture("D:\\media\\hud\\nametag.tga", &g_Game.texNameTag);
    LoadTexture("D:\\media\\music.tga", &g_Game.texMusic);
    LoadTexture("D:\\media\\winner.tga", &g_Game.texWinner);
    LoadTexture("D:\\media\\stripe.tga", &g_Game.texStripe);
    LoadTexture("D:\\media\\solid.tga", &g_Game.texSolid);
    LoadTexture("D:\\media\\red.tga", &g_Game.texRed);
    LoadTexture("D:\\media\\black.tga", &g_Game.texBlack);
    LoadTexture("D:\\media\\blue.tga", &g_Game.texBlue);
    LoadTexture("D:\\media\\menu\\mainmenu.tga", &g_Game.texMainMenu);
    sprintf(strFile, "D:\\media\\menu\\%i.tga", 1);
    LoadTexture(strFile, &g_Game.texMenuPages[0]);
    LoadTexture("D:\\media\\greenbutton.tga", &g_Game.texGreenButton);
    LoadTexture("D:\\media\\redbutton.tga", &g_Game.texRedButton);
    LoadTexture("D:\\media\\bluebutton.tga", &g_Game.texBlueButton);
    LoadTexture("D:\\media\\yellowbutton.tga", &g_Game.texYellowButton);
    LoadTexture("D:\\media\\whitebutton.tga", &g_Game.texWhiteButton);
    LoadTexture("D:\\media\\blackbutton.tga", &g_Game.texBlackButton);
    LoadTexture("D:\\media\\startbutton.tga", &g_Game.texStartButton);
    LoadTexture("D:\\media\\backbutton.tga", &g_Game.texBackButton);
    LoadTexture("D:\\media\\menu\\main.tga", &g_Game.texMain);
    LoadTexture("D:\\media\\menu\\preview.tga", &g_Game.texPreview);
    LoadTexture("D:\\media\\menu\\aftershock.tga", &g_Game.texAftershock);
    LoadTexture("D:\\media\\menu\\new.tga", &g_Game.texNew);
    LoadTexture("D:\\media\\menu\\custom.tga", &g_Game.texCustom);
    LoadTexture("D:\\media\\menu\\newgame.tga", &g_Game.texNewGame);
    LoadTexture("D:\\media\\menu\\options.tga", &g_Game.texOptions);
    LoadTexture("D:\\media\\menu\\arrow.tga", &g_Game.texArrow);
    for (int i = 0; i < 7; i++)
    {
        sprintf(strFile, "D:\\media\\edit\\%i.tga", i + 1);
        LoadTexture(strFile, &g_Game.texEdit[i]);
    }
}

// 0x000254E0
// Four unit quads (top left, top right, bottom left, bottom right; drawn as
// triangle strips): plain, mirrored left-right, flipped upside down, and one
// shaded gray toward its right edge. The normals are left uninitialized.
void CreateQuadBuffers()
{
    static const float uv[4][4][2] =
    {
        { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } },
        { { 1, 0 }, { 0, 0 }, { 1, 1 }, { 0, 1 } },
        { { 0, 1 }, { 1, 1 }, { 0, 0 }, { 1, 0 } },
        { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } },
    };
    LPDIRECT3DVERTEXBUFFER8* ppVB[4] = { &g_Game.quadVB, &g_Game.quadMirroredVB, &g_Game.quadFlippedVB, &g_Game.quadShadedVB };

    MESHVERTEX v[4];
    v[0].x = -0.5f; v[0].y =  0.5f; v[0].z = 0.0f;
    v[1].x =  0.5f; v[1].y =  0.5f; v[1].z = 0.0f;
    v[2].x = -0.5f; v[2].y = -0.5f; v[2].z = 0.0f;
    v[3].x =  0.5f; v[3].y = -0.5f; v[3].z = 0.0f;
    for (int q = 0; q < 4; q++)
    {
        for (int i = 0; i < 4; i++)
        {
            v[i].color = (q == 3 && (i & 1)) ? 0xFF7B7B7B : 0xFFFFFFFF;
            v[i].tu = uv[q][i][0];
            v[i].tv = uv[q][i][1];
        }
        g_Game.device->CreateVertexBuffer(4 * sizeof(MESHVERTEX), 0, 0, 0, ppVB[q]);
        MESHVERTEX* pVertices;
        (*ppVB[q])->Lock(0, 0, (BYTE**)&pVertices, 0);
        memcpy(pVertices, v, sizeof(v));
    }
}

// 0x000167E0
// A textured screen rectangle. Note that x is its right edge: it covers
// x - w .. x horizontally and y .. y + h vertically (w and h truncated).
void DrawRect(float x, float y, float w, float h, LPDIRECT3DTEXTURE8 pTexture)
{
    float fBottom = (int)h + y;
    float fLeft = x - (int)w;

    SCREENVERTEX v[4];
    g_Game.device->SetVertexShader(D3DFVF_SCREENVERTEX);
    for (int i = 0; i < 4; i++)
    {
        v[i].z = 0.0f;
        v[i].rhw = 0.1f;
        v[i].color = 0xFFFFFFFF;
    }
    v[0].x = fLeft - 0.5f; v[0].y = y - 0.5f;       v[0].tu = 0.0f; v[0].tv = 0.0f;
    v[1].x = x - 0.5f;     v[1].y = y - 0.5f;       v[1].tu = 1.0f; v[1].tv = 0.0f;
    v[2].x = x - 0.5f;     v[2].y = fBottom - 0.5f; v[2].tu = 1.0f; v[2].tv = 1.0f;
    v[3].x = fLeft - 0.5f; v[3].y = fBottom - 0.5f; v[3].tu = 0.0f; v[3].tv = 1.0f;
    g_Game.device->SetTexture(0, pTexture);
    g_Game.device->DrawVerticesUP(D3DPT_TRIANGLEFAN, 4, v, sizeof(SCREENVERTEX));
}
