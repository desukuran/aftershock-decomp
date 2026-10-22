// Sound effects through XACT.
#include "Game.h"

// 0x00028BD0
CWmaStream::CWmaStream()
{
    m_pSourceXMO = NULL;
    m_pDestXMO = NULL;
    m_pvSourceBuffer = NULL;
    m_hrOpenResult = E_PENDING;
    for (int i = 0; i < 8; i++)
        m_adwStatus[i] = 0;
    m_dwFileProgress = 0;
    m_dwUnknown30 = 0;
    ZeroMemory(&m_ContentDesc, sizeof(m_ContentDesc));
}

// 0x00028C20
CWmaStream::~CWmaStream()
{
    if (m_pSourceXMO)
        m_pSourceXMO->Release();
    if (m_pDestXMO)
        m_pDestXMO->Release();
    if (m_pvSourceBuffer)
        delete m_pvSourceBuffer;
    if (m_ContentDesc.pTitle)
        delete m_ContentDesc.pTitle;
    if (m_ContentDesc.pDescription)
        delete m_ContentDesc.pDescription;
    if (m_ContentDesc.pAuthor)
        delete m_ContentDesc.pAuthor;
}

// 0x00024230
// Reads a whole file into memory (rounded up to 2 KB sectors, as the file
// is opened unbuffered).
HRESULT LoadFile(const char* strFile, void** ppData, DWORD* pdwSize)
{
    if (strFile == NULL || ppData == NULL)
        return E_INVALIDARG;
    *ppData = NULL;
    if (pdwSize != NULL)
        *pdwSize = 0;

    HANDLE hFile = CreateFileA(strFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                               FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return E_FAIL;

    DWORD dwFileSize = GetFileSize(hFile, NULL);
    if (dwFileSize == INVALID_FILE_SIZE)
    {
        CloseHandle(hFile);
        return E_FAIL;
    }

    DWORD dwAllocSize = (dwFileSize + 2047) / 2048 * 2048;
    void* pData = malloc(dwAllocSize);
    if (pData == NULL)
    {
        CloseHandle(hFile);
        return E_OUTOFMEMORY;
    }

    OVERLAPPED overlapped;
    ZeroMemory(&overlapped, sizeof(overlapped));
    DWORD dwRead;
    if ((ReadFile(hFile, pData, dwAllocSize, NULL, &overlapped) || GetLastError() == ERROR_IO_PENDING) &&
        GetOverlappedResult(hFile, &overlapped, &dwRead, TRUE))
    {
        *ppData = pData;
        if (pdwSize != NULL)
            *pdwSize = dwRead;
        CloseHandle(hFile);
        return S_OK;
    }

    free(pData);
    CloseHandle(hFile);
    return GetLastError();
}

// 0x000241D0
// Full HRTF 3D sound, 64 3D voices.
void CSound::Create()
{
    DirectSoundUseFullHRTF();

    XACT_RUNTIME_PARAMETERS params;
    params.dwMax2DHwVoices = 128;
    params.dwMax3DHwVoices = 64;
    params.dwMaxConcurrentStreams = 20;
    params.dwMaxNotifications = 0;
    params.dwInteractiveAudioLookaheadTime = 0;
    if (FAILED(XACTEngineCreate(&params, &m_pEngine)))
        OutputDebugStringA("XACT couldn't start\n");
}

// 0x00024370
// Also downloads the standard effects image (its reverb and crosstalk).
void CSound::LoadBanks()
{
    DWORD dwSize;
    if (FAILED(LoadFile("D:\\media\\snd\\jaystarwars.xwb", &m_pWaveBankData, &dwSize)))
    {
        OutputDebugStringA("couldn't load wave bank\n");
        return;
    }
    if (FAILED(m_pEngine->RegisterWaveBank(m_pWaveBankData, dwSize, &m_pWaveBank)))
    {
        OutputDebugStringA("couldn't register wave bank\n");
        return;
    }
    if (FAILED(LoadFile("D:\\media\\snd\\starwarsbank.xsb", &m_pSoundBankData, &dwSize)))
    {
        OutputDebugStringA("couldn't load sound bank\n");
        return;
    }
    if (FAILED(m_pEngine->CreateSoundBank(m_pSoundBankData, dwSize, &m_pSoundBank)))
    {
        OutputDebugStringA("couldn't create sound bank\n");
        return;
    }

    DSEFFECTIMAGELOC loc;
    loc.dwI3DL2ReverbIndex = 0;
    loc.dwCrosstalkIndex = 1;
    if (FAILED(XAudioDownloadEffectsImage("d:\\media\\snd\\dsstdfx.bin", &loc, XAUDIO_DOWNLOADFX_EXTERNFILE, NULL)))
        OutputDebugStringA("couldn't download effects image\n");
}

// 0x00024190
// Plays a cue on a sound source (NULL: 2D), released when it ends.
void CSound::Play(DWORD dwCue, PXACTSOUNDSOURCE pSource)
{
    m_pSoundBank->Play(dwCue, pSource, XACT_FLAG_SOUNDCUE_AUTORELEASE, NULL);
}

// 0x00017D00
void CSound::SetListener(const D3DXVECTOR3* pPos, const D3DXVECTOR3* pFront, const D3DXVECTOR3* pTop,
                         const D3DXVECTOR3* pVelocity)
{
    m_pEngine->SetListenerPosition(pPos->x, pPos->y, pPos->z, DS3D_DEFERRED);
    m_pEngine->SetListenerOrientation(pFront->x, pFront->y, pFront->z, pTop->x, pTop->y, pTop->z, DS3D_DEFERRED);
    m_pEngine->SetListenerVelocity(pVelocity->x, pVelocity->y, pVelocity->z, DS3D_DEFERRED);
    m_pEngine->CommitDeferredSettings();
}
