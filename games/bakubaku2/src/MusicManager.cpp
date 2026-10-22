// Custom soundtrack playback (the XDK BackgroundMusic sample's music manager).
#include "Game.h"

// 0x000F72FC
// Tremors (by Entropy Music, 4:15), then four songs the user can put on the
// disc in place of the empty trackN.wma files (their length is unknown).
GAMESONG g_GameSongs[NUM_GAME_SONGS] =
{
    { L"Tremors", "D:\\Media\\music\\tremors.wma", 255000 },
    { L"User Track 1", "D:\\Media\\music\\track1.wma", 0 },
    { L"User Track 2", "D:\\Media\\music\\track2.wma", 0 },
    { L"User Track 3", "D:\\Media\\music\\track3.wma", 0 },
    { L"User Track 4", "D:\\Media\\music\\track4.wma", 0 },
};

// 0x000F7338 (not const, so it is read at run time)
FLOAT FADE_TIME = PACKET_COUNT * PACKET_TIME;

// 0x000194A0
// A packet finished playing: count it if it was on the current stream.
void CALLBACK StreamCallback(LPVOID pStreamContext, LPVOID pPacketContext, DWORD dwStatus)
{
    CMusicManager* pThis = (CMusicManager*)pStreamContext;
    if ((DWORD)pPacketContext == pThis->m_dwStream && dwStatus == XMEDIAPACKET_STATUS_SUCCESS)
        pThis->m_dwPacketsCompleted++;
}

// 0x000194C0
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

// (inlined into Prepare)
HANDLE CSoundtrack::OpenSong(DWORD dwSongID)
{
    if (m_bGameSoundtrack)
        return CreateFile(g_GameSongs[dwSongID].strFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                          FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL);
    else
        return XOpenSoundtrackSong(dwSongID, TRUE);
}

// 0x00019110
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

// 0x00019520
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

// 0x00019690
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

// 0x00019810
// Also pops up the music HUD.
HRESULT CMusicManager::Play()
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
    g_bMusicOn = true;
    g_dwMusicHudTime = GetTickCount();
    return S_OK;
}

// 0x00019590
HRESULT CMusicManager::Stop()
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
    g_bMusicOn = false;
    return S_OK;
}

// 0x00019160
HRESULT CMusicManager::Pause()
{
    if (m_mmState != MM_PLAYING)
        return S_FALSE;
    if (m_pStream[0])
        m_pStream[0]->Pause(DSSTREAMPAUSE_PAUSE);
    if (m_pStream[1])
        m_pStream[1]->Pause(DSSTREAMPAUSE_PAUSE);
    m_mmState = MM_PAUSED;
    g_bMusicOn = false;
    return S_OK;
}

// 0x000191A0
HRESULT CMusicManager::SetRandom(BOOL bRandom)
{
    m_bRandom = bRandom;
    return S_OK;
}

// 0x000191B0
HRESULT CMusicManager::SetGlobal(BOOL bGlobal)
{
    m_bGlobal = bGlobal;
    return S_OK;
}

// 0x000191C0
HRESULT CMusicManager::SetVolume(FLOAT fVolume)
{
    m_fVolume = fVolume;
    if (m_pStream[0])
        m_pStream[0]->SetVolume((LONG)fVolume);
    if (m_pStream[1])
        m_pStream[1]->SetVolume((LONG)fVolume);
    return S_OK;
}

// 0x00019200
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

// 0x00019250
FLOAT CMusicManager::GetPlaybackPosition()
{
    return m_dwPacketsCompleted * PACKET_TIME;
}

// 0x00019AC0
HRESULT CMusicManager::NextSoundtrack()
{
    if (m_mmState == MM_STOPPED)
    {
        return SelectSoundtrack((m_uCurrentSoundtrack + 1) % m_uSoundtrackCount);
    }
    return S_FALSE;
}

// 0x00019860
HRESULT CMusicManager::NextSong()
{
    if (m_mmState == MM_STOPPED)
    {
        return SelectSong((m_dwSong + 1) % m_aSoundtracks[m_uCurrentSoundtrack].GetSongCount());
    }
    return S_FALSE;
}

// 0x000198A0
// Quirk: from the first song the unsigned wrap gives 0xFFFFFFFF % count,
// not the last song.
HRESULT CMusicManager::PrevSong()
{
    if (m_mmState == MM_STOPPED)
    {
        return SelectSong((m_dwSong - 1) % m_aSoundtracks[m_uCurrentSoundtrack].GetSongCount());
    }
    return S_FALSE;
}

// 0x00019B10
// A random song of this soundtrack, or of a random one.
HRESULT CMusicManager::RandomSong(BOOL bAnySoundtrack)
{
    if (m_mmState == MM_STOPPED)
    {
        if (bAnySoundtrack)
        {
            SelectSoundtrack(rand() % m_uSoundtrackCount);
        }
        return SelectSong(rand() % m_aSoundtracks[m_uCurrentSoundtrack].GetSongCount());
    }
    else
        return S_FALSE;
}

// 0x00019270
// The game soundtrack first, then the user's soundtracks that have songs.
HRESULT CMusicManager::LoadSoundtracks()
{
    HANDLE hSoundtrack;
    XSOUNDTRACK_DATA stData;
    UINT uAllocatedSoundtracks;

    m_aSoundtracks = (CSoundtrack*)malloc(sizeof(CSoundtrack));
    if (!m_aSoundtracks)
        return E_OUTOFMEMORY;
    uAllocatedSoundtracks = 1;

    m_aSoundtracks[0].m_bGameSoundtrack = TRUE;
    m_aSoundtracks[0].m_uSoundtrackID = 0;
    m_aSoundtracks[0].m_uSongCount = NUM_GAME_SONGS;
    wcscpy(m_aSoundtracks[0].m_strName, L"Game Soundtrack");
    m_uSoundtrackCount = 1;

    hSoundtrack = XFindFirstSoundtrack(&stData);
    if (INVALID_HANDLE_VALUE != hSoundtrack)
    {
        do
        {
            // Double the buffer when it is full.
            if (m_uSoundtrackCount + 1 > uAllocatedSoundtracks)
            {
                void* pNewAlloc = realloc(m_aSoundtracks, (uAllocatedSoundtracks * 2) * sizeof(CSoundtrack));
                if (!pNewAlloc)
                {
                    free(m_aSoundtracks);
                    m_aSoundtracks = NULL;
                    XFindClose(hSoundtrack);
                    return E_OUTOFMEMORY;
                }
                m_aSoundtracks = (CSoundtrack*)pNewAlloc;
                uAllocatedSoundtracks *= 2;
            }

            if (stData.uSongCount > 0)
            {
                m_aSoundtracks[m_uSoundtrackCount].m_bGameSoundtrack = FALSE;
                m_aSoundtracks[m_uSoundtrackCount].m_uSoundtrackID = stData.uSoundtrackId;
                m_aSoundtracks[m_uSoundtrackCount].m_uSongCount = stData.uSongCount;
                wcscpy(m_aSoundtracks[m_uSoundtrackCount].m_strName, stData.szName);

                m_uSoundtrackCount++;
            }
        } while (XFindNextSoundtrack(hSoundtrack, &stData));

        XFindClose(hSoundtrack);
    }

    // Shrink it to what is used.
    VOID* pRealloc = realloc(m_aSoundtracks, m_uSoundtrackCount * sizeof(CSoundtrack));
    if (!pRealloc)
    {
        free(m_aSoundtracks);
        m_aSoundtracks = NULL;
        return E_OUTOFMEMORY;
    }

    m_aSoundtracks = (CSoundtrack*)pRealloc;

    return S_OK;
}

// 0x000198E0
HRESULT CMusicManager::SelectSoundtrack(DWORD dwSoundtrack)
{
    m_uCurrentSoundtrack = dwSoundtrack;
    SelectSong(0);
    return S_OK;
}

// (inlined everywhere)
HRESULT CMusicManager::SelectSong(DWORD dwSong)
{
    m_dwSong = dwSong;
    m_aSoundtracks[m_uCurrentSoundtrack].GetSongInfo(m_dwSong, &m_dwSongID, &m_dwLength, m_strSong);

    return S_OK;
}

// 0x00019910
HRESULT CMusicManager::FindNextSong()
{
    if (m_bRandom)
    {
        if (m_bGlobal)
        {
            SelectSoundtrack(rand() % m_uSoundtrackCount);
        }
        SelectSong(rand() % m_aSoundtracks[m_uCurrentSoundtrack].GetSongCount());
    }
    else
    {
        if (m_bGlobal &&
            m_dwSong == m_aSoundtracks[m_uCurrentSoundtrack].GetSongCount() - 1)
        {
            SelectSoundtrack((m_uCurrentSoundtrack + 1) % m_uSoundtrackCount);
        }
        else
            SelectSong((m_dwSong + 1) % m_aSoundtracks[m_uCurrentSoundtrack].GetSongCount());
    }

    return S_OK;
}

// 0x00019600
// Opens the current song and creates a WMA decoder for it.
HRESULT CMusicManager::Prepare()
{
    m_hDecodingFile = m_aSoundtracks[m_uCurrentSoundtrack].OpenSong(m_dwSongID);

    WMAXMODECODERPARAMETERS Params = { 0 };
    Params.hFile = m_hDecodingFile;
    Params.dwLookaheadBufferSize = 64 * 1024;

    HRESULT hr = XWmaDecoderCreateMediaObject((LPCWMAXMODECODERPARAMETERS)&Params, &m_pDecoder);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

// 0x00019410
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

// 0x00019450
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

// 0x000199F0
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

// 0x00019BA0
// Called every frame.
HRESULT CMusicManager::Process()
{
    HRESULT hr;
    DWORD dwPacket;

    if (m_pDecoder)
    {
        m_pDecoder->DoWork();

        while (FindFreePacket(&dwPacket, m_dwStream))
        {
            XMEDIAPACKET xmp;
            hr = ProcessSource(dwPacket, &xmp);
            if (FAILED(hr))
                return hr;

            // Nothing decoded: the end of the song, or the decoder isn't
            // ready yet.
            if (xmp.dwMaxSize > 0)
            {
                hr = ProcessStream(dwPacket, &xmp);
                if (FAILED(hr))
                    return hr;
            }
            else
                break;
        }
    }

    return S_OK;
}

// (inlined into Process)
HRESULT CMusicManager::ProcessStream(DWORD dwPacket, XMEDIAPACKET* pxmp)
{
    HRESULT hr;

    pxmp->pdwStatus = &m_adwStatus[dwPacket];
    pxmp->pdwCompletedSize = NULL;
    pxmp->pContext = (LPVOID)m_dwStream;

    hr = m_pStream[m_dwStream]->Process(pxmp, NULL);
    if (FAILED(hr))
        return hr;

    return S_OK;
}
