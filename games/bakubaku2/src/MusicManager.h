// Custom soundtrack playback: the XDK BackgroundMusic sample's CMusicManager,
// an earlier revision than the other AfterShock games': no packet timer,
// pausing, a volume, random songs. Plays the "Game Soundtrack" (Tremors and
// four slots for the user's own WMA files on the disc) or the user's
// soundtracks from the hard disk.
#ifndef BAKU_MUSICMANAGER_H
#define BAKU_MUSICMANAGER_H

#include <xtl.h>

// 2048 16-bit stereo samples per packet.
#define PACKET_SIZE     (2048 * 2 * 2)
#define PACKET_TIME     (2048.0f / 44100.0f)
#define PACKET_COUNT    43
#define EXTRA_PACKETS   10

#define MAX_SOUNDTRACK_NAME 32
#define MAX_SONG_NAME       32

extern FLOAT FADE_TIME;

enum MM_STATE
{
    MM_STOPPED,
    MM_PAUSED,
    MM_PLAYING,
};

// The game's own songs, used by the "Game Soundtrack".
struct GAMESONG
{
    const WCHAR* strName;                   // +0x00
    const char*  strFile;                   // +0x04
    DWORD        dwLength;                  // +0x08 in ms
};                                          // 0x0C bytes
#define NUM_GAME_SONGS 5
extern GAMESONG g_GameSongs[NUM_GAME_SONGS];

class CSoundtrack
{
public:
    WCHAR m_strName[MAX_SOUNDTRACK_NAME];   // +0x00
    UINT  m_uSongCount;                     // +0x40
    BOOL  m_bGameSoundtrack;                // +0x44
    UINT  m_uSoundtrackID;                  // +0x48 (the game soundtrack: an index)

    void GetSongInfo(UINT uSongIndex, DWORD* pdwID, DWORD* pdwLength, WCHAR* strName);
};

class CMusicManager
{
public:
    CMusicManager();
    ~CMusicManager();
    HRESULT Initialize();

    HRESULT Play();
    HRESULT Stop();
    HRESULT Pause();
    HRESULT SetRandom(BOOL bRandom);
    HRESULT SetGlobal(BOOL bGlobal);
    HRESULT SetVolume(FLOAT fVolume);
    HRESULT GetCurrentInfo(WCHAR* strSoundtrack, WCHAR* strSong, DWORD* pdwLength);
    FLOAT   GetPlaybackPosition();
    HRESULT NextSoundtrack();
    HRESULT NextSong();
    HRESULT PrevSong();
    HRESULT RandomSong(BOOL bAnySoundtrack);
    HRESULT Process();

    BOOL    m_bGlobal;                          // +0x00 go on to the next soundtrack at the end of one
    BOOL    m_bRandom;                          // +0x04
    FLOAT   m_fVolume;                          // +0x08 in 1/100 dB
    int     m_mmState;                          // +0x0C MM_STATE
    DWORD   m_dwPacketsCompleted;               // +0x10

    CSoundtrack* m_aSoundtracks;                // +0x14
    UINT    m_uSoundtrackCount;                 // +0x18
    UINT    m_uCurrentSoundtrack;               // +0x1C
    WCHAR   m_strSong[MAX_SONG_NAME];           // +0x20
    DWORD   m_dwLength;                         // +0x60
    DWORD   m_dwSong;                           // +0x64
    DWORD   m_dwSongID;                         // +0x68
    HANDLE  m_hDecodingFile;                    // +0x6C

    XWmaFileMediaObject* m_pDecoder;            // +0x70
    LPDIRECTSOUNDSTREAM  m_pStream[2];          // +0x74
    DWORD   m_dwStream;                         // +0x7C the stream being fed
    BYTE*   m_pSampleData;                      // +0x80

    DWORD   m_adwStatus[PACKET_COUNT + EXTRA_PACKETS];  // +0x84

private:
    HRESULT LoadSoundtracks();
    HRESULT SelectSoundtrack(DWORD dwSoundtrack);
    void    SelectSong(DWORD dwSong);
    HRESULT FindNextSong();
    HRESULT Prepare();
    HRESULT Cleanup();
    BOOL    FindFreePacket(DWORD* pdwPacket, DWORD dwStream);
    HRESULT ProcessSource(DWORD dwPacket, XMEDIAPACKET* pxmp);

    friend void CALLBACK StreamCallback(LPVOID pStreamContext, LPVOID pPacketContext, DWORD dwStatus);
};                                              // 0x158 bytes

#endif
