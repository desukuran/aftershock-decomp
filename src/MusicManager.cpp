// Custom soundtrack playback (the XDK BackgroundMusic sample's music manager).
#include "MusicManager.h"
#include "Game.h"

const FLOAT FADE_TIME = PACKET_COUNT * PACKET_TIME;

// 0x000B6C4C
GAMESONG g_GameSongs[NUM_GAME_SONGS] =
{
    { L"Push Black!", "D:\\nomotherfuckingsongs.wma", 0 },
};

// 0x00036C60
// A packet finished playing: count it if it was on the current stream.
void CALLBACK StreamCallback(LPVOID pStreamContext, LPVOID pPacketContext, DWORD dwStatus)
{
    CMusicManager* pThis = (CMusicManager*)pStreamContext;
    if ((DWORD)pPacketContext == pThis->m_dwStream && dwStatus == XMEDIAPACKET_STATUS_SUCCESS)
        pThis->m_dwPacketsCompleted++;
}

// 0x00036C80
void CSoundtrack::GetSongInfo(UINT uSongIndex, DWORD* pdwID, DWORD* pdwLength, WCHAR* strName)
{
    if (m_bGameSoundtrack)
    {
        *pdwID = uSongIndex;
        *pdwLength = g_GameSongs[uSongIndex].dwLength;
        wcscpy(strName, g_GameSongs[uSongIndex].strName);
    }
    else
    {
        XGetSoundtrackSongInfo(m_uSoundtrackID, uSongIndex, pdwID, pdwLength, strName, MAX_SONG_NAME);
    }
}

// 0x00036960
CMusicManager::CMusicManager()
{
    m_mmState = MM_STOPPED;
    m_bRandom = FALSE;
    m_bGlobal = TRUE;
    m_fVolume = 0.0f;
    m_dwStream = 0;
    m_dwSong = 0;
    m_strSong[0] = 0;
    m_dwLength = 0;
    m_dwSongID = 0;
    m_aSoundtracks = NULL;
    m_uSoundtrackCount = 0;
    m_uCurrentSoundtrack = 0;
    m_dwPacketsCompleted = 0;
    m_pSampleData = NULL;
    m_pDecoder = NULL;
    m_pStream[0] = NULL;
    m_pStream[1] = NULL;
    m_hDecodingFile = INVALID_HANDLE_VALUE;
}

// 0x00036CE0
CMusicManager::~CMusicManager()
{
    Cleanup();
    if (m_pStream[0])
        m_pStream[0]->Release();
    m_pStream[0] = NULL;
    if (m_pStream[1])
        m_pStream[1]->Release();
    m_pStream[1] = NULL;
    if (m_pSampleData)
        delete[] m_pSampleData;
    free(m_aSoundtracks);
}

// 0x00036E60
HRESULT CMusicManager::Initialize()
{
    HRESULT hr = LoadSoundtracks();
    if (FAILED(hr))
        return hr;
    SelectSong(0);

    srand(GetTickCount());

    m_pSampleData = new BYTE[(PACKET_COUNT + EXTRA_PACKETS) * PACKET_SIZE];
    if (m_pSampleData == NULL)
        return E_OUTOFMEMORY;

    WAVEFORMATEX wfx = { 0 };
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;

    DSSTREAMDESC dssd = { 0 };
    dssd.dwMaxAttachedPackets = PACKET_COUNT;
    dssd.lpwfxFormat = &wfx;
    dssd.lpfnCallback = StreamCallback;
    dssd.lpvContext = this;

    hr = DirectSoundCreateStream(&dssd, &m_pStream[0]);
    if (FAILED(hr))
        return hr;
    hr = DirectSoundCreateStream(&dssd, &m_pStream[1]);
    if (FAILED(hr))
        return hr;

    // Fade in and out over the length of the queued packets.
    DSENVELOPEDESC dsed = { 0 };
    dsed.dwEG = DSEG_AMPLITUDE;
    dsed.dwMode = DSEG_MODE_ATTACK;
    dsed.dwAttack = DWORD(48000 * FADE_TIME / 512);
    dsed.dwRelease = DWORD(48000 * FADE_TIME / 512);
    dsed.dwSustain = 255;
    m_pStream[0]->SetEG(&dsed);
    m_pStream[1]->SetEG(&dsed);
    return S_OK;
}

// 0x00036FE0
// Also pops up the music HUD.
HRESULT CMusicManager::Play(void* pUnused)
{
    if (m_mmState == MM_STOPPED)
    {
        Prepare();
    }
    else if (m_mmState == MM_PAUSED)
    {
        if (m_pStream[0])
            m_pStream[0]->Pause(DSSTREAMPAUSE_RESUME);
        if (m_pStream[1])
            m_pStream[1]->Pause(DSSTREAMPAUSE_RESUME);
    }
    m_mmState = MM_PLAYING;

    g_Game.bShowMusicHud = true;
    g_Game.musicHudTime = GetTickCount();
    return S_OK;
}

// 0x00036D60
HRESULT CMusicManager::Stop(void* pUnused)
{
    if (m_mmState != MM_STOPPED)
    {
        if (m_pStream[0])
            m_pStream[0]->FlushEx(0, DSSTREAMFLUSHEX_ASYNC);
        if (m_pStream[1])
            m_pStream[1]->FlushEx(0, DSSTREAMFLUSHEX_ASYNC);
        Cleanup();
    }
    m_mmState = MM_STOPPED;
    return S_OK;
}

// 0x000369C0
HRESULT CMusicManager::SetGlobal(BOOL bGlobal)
{
    m_bGlobal = bGlobal;
    return S_OK;
}

// 0x000369D0
HRESULT CMusicManager::GetCurrentInfo(WCHAR* strSoundtrack, WCHAR* strSong, DWORD* pdwLength)
{
    if (strSoundtrack)
        wcscpy(strSoundtrack, m_aSoundtracks[m_uCurrentSoundtrack].m_strName);
    if (strSong)
        wcscpy(strSong, m_strSong);
    if (pdwLength)
        *pdwLength = m_dwLength;
    return S_OK;
}

// 0x00036A20
FLOAT CMusicManager::GetPlaybackPosition()
{
    return m_dwPacketsCompleted * PACKET_TIME;
}

// 0x000372C0
HRESULT CMusicManager::NextSoundtrack()
{
    if (m_mmState != MM_STOPPED)
        return S_FALSE;
    SelectSoundtrack((m_uCurrentSoundtrack + 1) % m_uSoundtrackCount);
    return S_OK;
}

// 0x00037040
HRESULT CMusicManager::NextSong()
{
    if (m_mmState != MM_STOPPED)
        return S_FALSE;
    SelectSong((m_dwSong + 1) % m_aSoundtracks[m_uCurrentSoundtrack].m_uSongCount);
    return S_OK;
}

// 0x00037080
HRESULT CMusicManager::PrevSong()
{
    if (m_mmState != MM_STOPPED)
        return S_FALSE;
    int nSong = m_dwSong;
    if (nSong - 1 < 0)
        nSong = m_aSoundtracks[m_uCurrentSoundtrack].m_uSongCount;
    SelectSong(nSong - 1);
    return S_OK;
}

// 0x00036A40
// The game soundtrack first, then the user's soundtracks that have songs.
HRESULT CMusicManager::LoadSoundtracks()
{
    m_aSoundtracks = (CSoundtrack*)malloc(sizeof(CSoundtrack));
    if (m_aSoundtracks == NULL)
        return E_OUTOFMEMORY;
    UINT uAllocated = 1;

    m_aSoundtracks[0].m_bGameSoundtrack = TRUE;
    m_aSoundtracks[0].m_uSoundtrackID = 0;
    m_aSoundtracks[0].m_uSongCount = NUM_GAME_SONGS;
    wcscpy(m_aSoundtracks[0].m_strName, L"Game Soundtrack");
    m_uSoundtrackCount = 1;

    XSOUNDTRACK_DATA stData;
    HANDLE hFind = XFindFirstSoundtrack(&stData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (m_uSoundtrackCount + 1 > uAllocated)
            {
                CSoundtrack* pNew = (CSoundtrack*)realloc(m_aSoundtracks, uAllocated * 2 * sizeof(CSoundtrack));
                if (pNew == NULL)
                {
                    free(m_aSoundtracks);
                    m_aSoundtracks = NULL;
                    XFindClose(hFind);
                    return E_OUTOFMEMORY;
                }
                m_aSoundtracks = pNew;
                uAllocated *= 2;
            }

            if (stData.uSongCount > 0)
            {
                CSoundtrack* pST = &m_aSoundtracks[m_uSoundtrackCount];
                pST->m_bGameSoundtrack = FALSE;
                pST->m_uSoundtrackID = stData.uSoundtrackId;
                pST->m_uSongCount = stData.uSongCount;
                wcscpy(pST->m_strName, stData.szName);
                m_uSoundtrackCount++;
            }
        } while (XFindNextSoundtrack(hFind, &stData));
        XFindClose(hFind);
    }

    CSoundtrack* pNew = (CSoundtrack*)realloc(m_aSoundtracks, m_uSoundtrackCount * sizeof(CSoundtrack));
    if (pNew == NULL)
    {
        free(m_aSoundtracks);
        m_aSoundtracks = NULL;
        return E_OUTOFMEMORY;
    }
    m_aSoundtracks = pNew;
    return S_OK;
}

// 0x000370D0
HRESULT CMusicManager::SelectSoundtrack(DWORD dwSoundtrack)
{
    m_uCurrentSoundtrack = dwSoundtrack;
    SelectSong(0);
    return S_OK;
}

// (inlined everywhere)
void CMusicManager::SelectSong(DWORD dwSong)
{
    m_dwSong = dwSong;
    m_aSoundtracks[m_uCurrentSoundtrack].GetSongInfo(m_dwSong, &m_dwSongID, &m_dwLength, m_strSong);
}

// 0x00037100
HRESULT CMusicManager::FindNextSong()
{
    if (m_bRandom)
    {
        if (m_bGlobal)
            SelectSoundtrack(rand() % m_uSoundtrackCount);
        SelectSong(rand() % m_aSoundtracks[m_uCurrentSoundtrack].m_uSongCount);
    }
    else if (m_bGlobal && m_dwSong == m_aSoundtracks[m_uCurrentSoundtrack].m_uSongCount - 1)
    {
        SelectSoundtrack((m_uCurrentSoundtrack + 1) % m_uSoundtrackCount);
    }
    else
    {
        SelectSong((m_dwSong + 1) % m_aSoundtracks[m_uCurrentSoundtrack].m_uSongCount);
    }
    return S_OK;
}

// 0x00036DD0
// Opens the current song and creates a WMA decoder for it.
HRESULT CMusicManager::Prepare()
{
    if (m_aSoundtracks[m_uCurrentSoundtrack].m_bGameSoundtrack)
        m_hDecodingFile = CreateFileA(g_GameSongs[m_dwSongID].strFile, GENERIC_READ, FILE_SHARE_READ, NULL,
                                      OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL);
    else
        m_hDecodingFile = XOpenSoundtrackSong(m_dwSongID, TRUE);

    WMAXMODECODERPARAMETERS params = { 0 };
    params.hFile = m_hDecodingFile;
    params.dwLookaheadBufferSize = 64 * 1024;
    HRESULT hr = XWmaDecoderCreateMediaObject(&params, &m_pDecoder);
    return SUCCEEDED(hr) ? S_OK : hr;
}

// 0x00036BD0
HRESULT CMusicManager::Cleanup()
{
    if (m_pDecoder)
    {
        m_pDecoder->Release();
        m_pDecoder = NULL;
    }
    if (m_hDecodingFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hDecodingFile);
        m_hDecodingFile = INVALID_HANDLE_VALUE;
    }
    m_dwPacketsCompleted = 0;
    return S_OK;
}

// 0x00036C10
// The packets of the two streams alternate (even/odd) for the first 21.
BOOL CMusicManager::FindFreePacket(DWORD* pdwPacket, DWORD dwStream)
{
    for (DWORD i = dwStream; i < PACKET_COUNT; i += 2)
    {
        if (m_adwStatus[i] != XMEDIAPACKET_STATUS_PENDING && (i > 20 || (i & 1) == dwStream))
        {
            *pdwPacket = i;
            return TRUE;
        }
    }
    return FALSE;
}

// 0x000371E0
// Decodes a packet. At the end of a song, flushes the current stream with its
// fade-out and starts the next song on the other stream.
HRESULT CMusicManager::ProcessSource(DWORD dwPacket, XMEDIAPACKET* pxmp)
{
    ZeroMemory(pxmp, sizeof(XMEDIAPACKET));

    DWORD dwStatus;
    m_pDecoder->GetStatus(&dwStatus);
    if (!(dwStatus & XMO_STATUSF_ACCEPT_OUTPUT_DATA))
        return S_FALSE;

    DWORD dwCompleted;
    pxmp->pvBuffer = m_pSampleData + dwPacket * PACKET_SIZE;
    pxmp->dwMaxSize = PACKET_SIZE;
    pxmp->pdwCompletedSize = &dwCompleted;
    HRESULT hr = m_pDecoder->Process(NULL, pxmp);
    if (FAILED(hr))
        return hr;

    if (dwCompleted == 0)
    {
        m_pStream[m_dwStream]->Discontinuity();
        m_pStream[m_dwStream]->FlushEx(0, DSSTREAMFLUSHEX_ASYNC | DSSTREAMFLUSHEX_ENVELOPE);
        Cleanup();
        FindNextSong();
        m_dwStream = (m_dwStream - 1) & 1;
        Prepare();
    }
    pxmp->dwMaxSize = dwCompleted;
    return S_OK;
}

// 0x00037310
// Called every frame while the music is on.
HRESULT CMusicManager::Process()
{
    if (m_mmState == MM_PAUSED)
    {
        if (m_pStream[0])
            m_pStream[0]->Pause(DSSTREAMPAUSE_RESUME);
        if (m_pStream[1])
            m_pStream[1]->Pause(DSSTREAMPAUSE_RESUME);
    }

    if (m_pDecoder)
    {
        m_pDecoder->DoWork();

        DWORD dwPacket;
        while (FindFreePacket(&dwPacket, m_dwStream))
        {
            m_dwLastPacketTime = GetTickCount();

            XMEDIAPACKET xmp;
            HRESULT hr = ProcessSource(dwPacket, &xmp);
            if (FAILED(hr))
                return hr;
            if (xmp.dwMaxSize == 0)
                return S_OK;

            xmp.pdwStatus = &m_adwStatus[dwPacket];
            xmp.pdwCompletedSize = NULL;
            xmp.pContext = (LPVOID)m_dwStream;
            hr = m_pStream[m_dwStream]->Process(&xmp, NULL);
            if (FAILED(hr))
                return hr;
        }

        // No free packet for 2 seconds: switch streams.
        if (GetTickCount() - m_dwLastPacketTime >= 2000)
        {
            m_dwLastPacketTime = GetTickCount();
            m_dwStream = (m_dwStream - 1) & 1;
        }
    }
    return S_OK;
}
