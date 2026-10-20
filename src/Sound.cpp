// Sound effects through XACT.
#include "Sound.h"
#include "Game.h"

// 0x000376B0
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

    DWORD dwAllocSize = (dwFileSize + 2047) & ~2047;
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

// 0x00037830
void CSound::Init()
{
    XACT_RUNTIME_PARAMETERS params;
    params.dwMax2DHwVoices = 128;
    params.dwMax3DHwVoices = 0;
    params.dwMaxConcurrentStreams = 20;
    params.dwMaxNotifications = 0;
    params.dwInteractiveAudioLookaheadTime = 0;
    if (FAILED(XACTEngineCreate(&params, &m_pEngine)))
    {
        OutputDebugStringA("XACT couldn't start\n");
        return;
    }

    XACT_WAVEBANK_STREAMING_PARAMETERS stream;
    stream.hFile = CreateFileA("D:\\media\\snd\\sfx.xwb", GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL);
    if (stream.hFile == INVALID_HANDLE_VALUE)
        return;
    stream.dwOffset = 0;
    stream.dwPacketSizeInMilliSecs = 400;
    stream.dwPrimePacketSizeInMilliSecs = 400;
    if (FAILED(m_pEngine->RegisterStreamedWaveBank(&stream, &m_pWaveBank)))
        return;

    DWORD dwSize;
    if (FAILED(LoadFile("D:\\media\\snd\\pool.xsb", &m_pSoundBankData, &dwSize)))
        return;
    if (FAILED(m_pEngine->CreateSoundBank(m_pSoundBankData, dwSize, &m_pSoundBank)))
        return;

    DSEFFECTIMAGELOC loc;
    loc.dwI3DL2ReverbIndex = 0;
    loc.dwCrosstalkIndex = 1;
    if (FAILED(XAudioDownloadEffectsImage("d:\\media\\snd\\dsstdfx.bin", &loc, XAUDIO_DOWNLOADFX_EXTERNFILE, NULL)))
        OutputDebugStringA("couldn't download effects image\n");
}

// 0x000377F0
void CSound::Play(DWORD dwCue)
{
    if (g_Game.bSound)
        m_pSoundBank->Play(dwCue, NULL, XACT_FLAG_SOUNDCUE_AUTORELEASE, NULL);
}
