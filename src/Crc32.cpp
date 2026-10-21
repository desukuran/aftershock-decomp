// The media check: CRC-32s of the media files, against values built into the
// game ("CRC Implementation by PeteKnep", says the readme).
#include "Crc32.h"
#include "Game.h"

CCrc32 g_Crc32;     // 0x000A5D00

struct MEDIAFILE
{
    const char* strPath;
    DWORD       dwCrc;
};

// 0x000A2A68 (paths), 0x0009DC28 (CRCs)
static const MEDIAFILE g_HeadFiles[] =
{
    { "d:\\media\\heads\\1.tga", 0x6800E7F0 },
    { "d:\\media\\heads\\10.tga", 0x1D1C45C8 },
    { "d:\\media\\heads\\11.tga", 0x88606C84 },
    { "d:\\media\\heads\\12.tga", 0x8CF61B93 },
    { "d:\\media\\heads\\13.tga", 0x63DB64AB },
    { "d:\\media\\heads\\14.tga", 0xA3A515E1 },
    { "d:\\media\\heads\\15.tga", 0x6917BE8E },
    { "d:\\media\\heads\\16.tga", 0xD360BEAF },
    { "d:\\media\\heads\\17.tga", 0xD30D187E },
    { "d:\\media\\heads\\18.tga", 0xD21D75FE },
    { "d:\\media\\heads\\19.tga", 0x73704751 },
    { "d:\\media\\heads\\2.tga", 0xD92EFD4B },
    { "d:\\media\\heads\\20.tga", 0x7966834C },
    { "d:\\media\\heads\\21.tga", 0x27620D75 },
    { "d:\\media\\heads\\22.tga", 0xCEAAE81E },
    { "d:\\media\\heads\\23.tga", 0x56700CE4 },
    { "d:\\media\\heads\\24.tga", 0x5292154C },
    { "d:\\media\\heads\\25.tga", 0xAFDE1B8C },
    { "d:\\media\\heads\\26.tga", 0xC998C8A4 },
    { "d:\\media\\heads\\27.tga", 0xC78A025F },
    { "d:\\media\\heads\\28.tga", 0x4705A8AB },
    { "d:\\media\\heads\\29.tga", 0x821670AE },
    { "d:\\media\\heads\\3.tga", 0x191AF5DB },
    { "d:\\media\\heads\\30.tga", 0x3F422F49 },
    { "d:\\media\\heads\\31.tga", 0xB7E85086 },
    { "d:\\media\\heads\\32.tga", 0x863DCDD2 },
    { "d:\\media\\heads\\33.tga", 0x83FC63EA },
    { "d:\\media\\heads\\34.tga", 0x9D57CA16 },
    { "d:\\media\\heads\\4.tga", 0x8131F775 },
    { "d:\\media\\heads\\5.tga", 0x7348A905 },
    { "d:\\media\\heads\\6.tga", 0xE53FF654 },
    { "d:\\media\\heads\\7.tga", 0x0A0930E8 },
    { "d:\\media\\heads\\8.tga", 0xFC0A9B33 },
    { "d:\\media\\heads\\9.tga", 0x4247FA21 },
};

// 0x000A2AF0, 0x0009DCB0
static const MEDIAFILE g_MediaFiles[] =
{
    { "d:\\media\\aftershock.jpg", 0xE6DBB7A3 },
    { "d:\\media\\axe.tga", 0xE075DDA1 },
    { "d:\\media\\axestuck.tga", 0xA81B0416 },
    { "d:\\media\\bat.tga", 0xCFB921E0 },
    { "d:\\media\\black.tga", 0x4133A95E },
    { "d:\\media\\blood.tga", 0x92598667 },
    { "d:\\media\\bloodspot.tga", 0x37653675 },
    { "d:\\media\\body.tga", 0x6027530B },
    { "d:\\media\\bodygib.tga", 0x0816A35F },
    { "d:\\media\\bodygib2.tga", 0x690A6A06 },
    { "d:\\media\\bubble.tga", 0xDA314039 },
    { "d:\\media\\bullet.tga", 0xAEA6D60F },
    { "d:\\media\\bulletcountholder.tga", 0xB9F79B05 },
    { "d:\\media\\bulletshell.tga", 0xAB51B355 },
    { "d:\\media\\bullettime.tga", 0xD60EC841 },
    { "d:\\media\\bullettrail.tga", 0x449940A8 },
    { "d:\\media\\chainsaw.tga", 0x202EB545 },
    { "d:\\media\\flash.tga", 0x7CBE6FAE },
    { "d:\\media\\foot.tga", 0x97739917 },
    { "d:\\media\\glasses.tga", 0xA7560DB4 },
    { "d:\\media\\hand.tga", 0x2D7221D0 },
    { "d:\\media\\handgun.tga", 0x916615CF },
    { "d:\\media\\hat.tga", 0xE3853DB1 },
    { "d:\\media\\head.tga", 0xD3635D28 },
    { "d:\\media\\headgib.tga", 0x3C3CEFC8 },
    { "d:\\media\\headgib2.tga", 0x0E61D8B8 },
    { "d:\\media\\healthbar.tga", 0x6CC91513 },
    { "d:\\media\\healthbar2.tga", 0xE602F096 },
    { "d:\\media\\healthbar3.tga", 0x83B50178 },
    { "d:\\media\\jetexhaust.tga", 0x633BCDD6 },
    { "d:\\media\\jetpack.tga", 0x2B02FF03 },
    { "d:\\media\\knife.tga", 0xEBDA11AA },
    { "d:\\media\\knifestuck.tga", 0x596895B8 },
    { "d:\\media\\logo.tga", 0x42E2B6DE },
    { "d:\\media\\machinegun.tga", 0x6A6B2DDD },
    { "d:\\media\\madnesstracks.tga", 0xD7872D5D },
    { "d:\\media\\nightstick.tga", 0x87D83D03 },
    { "d:\\media\\ninja.tga", 0x22CD4917 },
    { "d:\\media\\patch.tga", 0x0ABF68C8 },
    { "d:\\media\\rifle.tga", 0x1350431A },
    { "d:\\media\\shadow.tga", 0xFF3CF3F7 },
    { "d:\\media\\shotgun.tga", 0x6BD2D07C },
    { "d:\\media\\shotgunshell.tga", 0xA380C0F3 },
    { "d:\\media\\smoke.tga", 0xCB20E7D5 },
    { "d:\\media\\sword.tga", 0xAA0740CC },
    { "d:\\media\\swordstuck.tga", 0x2B8A46A3 },
    { "d:\\media\\talk.tga", 0xF4804EC0 },
    { "d:\\media\\target.tga", 0xA4F1198F },
    { "d:\\media\\texture.tga", 0xA1317628 },
    { "d:\\media\\white.tga", 0x520CD6B8 },
};

// 0x000A2BB8, 0x0009DD78
static const MEDIAFILE g_LevelFiles[] =
{
    { "d:\\media\\levels\\1.JPG", 0x6B4899AF },
    { "d:\\media\\levels\\10.JPG", 0x0D8F8D8E },
    { "d:\\media\\levels\\11.JPG", 0x5669BC27 },
    { "d:\\media\\levels\\12.JPG", 0x9CD0B0F9 },
    { "d:\\media\\levels\\13.JPG", 0x17EE079A },
    { "d:\\media\\levels\\14.JPG", 0xF579BD41 },
    { "d:\\media\\levels\\2.JPG", 0x2323EDC2 },
    { "d:\\media\\levels\\3.JPG", 0x878AB492 },
    { "d:\\media\\levels\\4.JPG", 0x2E06BFC5 },
    { "d:\\media\\levels\\5.JPG", 0x8BB02B59 },
    { "d:\\media\\levels\\6.JPG", 0xE0426936 },
    { "d:\\media\\levels\\7.JPG", 0x5F3FF944 },
    { "d:\\media\\levels\\8.JPG", 0xB9409F8C },
    { "d:\\media\\levels\\9.JPG", 0xEC95302C },
    { "d:\\media\\levels\\cutscene1.JPG", 0x65DFEC93 },
    { "d:\\media\\levels\\cutscene2.JPG", 0xA3E4C75B },
};

// 0x000A2BF8, 0x0009DDB8
static const MEDIAFILE g_MenuFiles[] =
{
    { "d:\\media\\menu\\1.tga", 0x9E4956FA },
    { "d:\\media\\menu\\10.tga", 0xB536B587 },
    { "d:\\media\\menu\\11.tga", 0xD8A89DBC },
    { "d:\\media\\menu\\2.tga", 0x4ADE6A6F },
    { "d:\\media\\menu\\3.tga", 0x83656A54 },
    { "d:\\media\\menu\\4.tga", 0x2734986E },
    { "d:\\media\\menu\\5.tga", 0x51AA5DE1 },
    { "d:\\media\\menu\\6.tga", 0xAD4096B5 },
    { "d:\\media\\menu\\7.tga", 0x5761750D },
    { "d:\\media\\menu\\8.tga", 0x4D671AB1 },
    { "d:\\media\\menu\\9.tga", 0xB5DCF23D },
    { "d:\\media\\menu\\arrow.tga", 0x89B761AD },
    { "d:\\media\\menu\\mainmenu.tga", 0xD7EDC1AB },
    { "d:\\media\\menu\\optionsmenu.tga", 0xCDD8955B },
    { "d:\\media\\menu\\snm1.tga", 0x4BD9D6B9 },
    { "d:\\media\\menu\\snm2.tga", 0x0C41189B },
    { "d:\\media\\menu\\tick.tga", 0x66409D21 },
    { "d:\\media\\menu\\unlock.tga", 0x638F3279 },
    { "d:\\media\\menu\\unlock1.tga", 0xF6818946 },
    { "d:\\media\\menu\\unlock2.tga", 0x73B22C20 },
    { "d:\\media\\menu\\unlock3.tga", 0x0D4F05EB },
    { "d:\\media\\menu\\unlock4.tga", 0x4E0206AC },
    { "d:\\media\\menu\\unlock5.tga", 0x943CA0A2 },
    { "d:\\media\\menu\\unlock6.tga", 0x6C485737 },
};

// 0x00011000
// The standard (reflected, 0xEDB88320) CRC-32 table.
CCrc32::CCrc32()
{
    for (int i = 0; i < 256; i++)
    {
        DWORD crc = i;
        for (int bit = 0; bit < 8; bit++)
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
        m_adwTable[i] = crc;
    }
}

// 0x000110A0
// The CRC-32 of a whole file, or 0 if it can't be opened.
DWORD CCrc32::FileCrc(const char* strPath)
{
    FILE* f = fopen(strPath, "rb");
    if (f == NULL)
        return 0;

    DWORD crc = 0xFFFFFFFF;
    BYTE buffer[512];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), f)) != 0)
    {
        for (size_t i = 0; i < n; i++)
            crc = (crc >> 8) ^ m_adwTable[(buffer[i] ^ crc) & 0xFF];
    }
    fclose(f);
    return ~crc;
}

// 0x00012060
// Never returns.
void MediaError(const char* strPath)
{
    WCHAR strText[512];
    swprintf(strText, L"%S may be damaged", strPath);
    for (;;)
    {
        g_Game.pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xFFFF0000, 1.0f, 0);
        g_Game.font.DrawText(L"Media Error!", 100.0f, 100.0f, 0, 0, 0);
        g_Game.font.DrawText(L"Please verify installation!", 100.0f, 150.0f, 0, 0, 0);
        g_Game.font.DrawText(strText, 100.0f, 200.0f, 0, 0, 0);
        g_Game.pDevice->Present(NULL, NULL, NULL, NULL);
        Sleep(100);
    }
}

// 0x00012120
// Checks every file's CRC; on the first mismatch, shows the media error
// screen for it (forever).
void VerifyMedia()
{
    UINT i;
    for (i = 0; i < ARRAYSIZE(g_HeadFiles); i++)
    {
        if (g_Crc32.FileCrc(g_HeadFiles[i].strPath) != g_HeadFiles[i].dwCrc)
            MediaError(g_HeadFiles[i].strPath);
    }
    for (i = 0; i < ARRAYSIZE(g_MediaFiles); i++)
    {
        if (g_Crc32.FileCrc(g_MediaFiles[i].strPath) != g_MediaFiles[i].dwCrc)
            MediaError(g_MediaFiles[i].strPath);
    }
    for (i = 0; i < ARRAYSIZE(g_LevelFiles); i++)
    {
        if (g_Crc32.FileCrc(g_LevelFiles[i].strPath) != g_LevelFiles[i].dwCrc)
            MediaError(g_LevelFiles[i].strPath);
    }
    for (i = 0; i < ARRAYSIZE(g_MenuFiles); i++)
    {
        if (g_Crc32.FileCrc(g_MenuFiles[i].strPath) != g_MenuFiles[i].dwCrc)
            MediaError(g_MenuFiles[i].strPath);
    }
}
