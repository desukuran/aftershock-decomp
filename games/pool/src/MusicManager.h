// Custom soundtrack playback: the XDK BackgroundMusic sample's CMusicManager
// (an early revision), rebuilt from the binary. Plays the user's soundtracks
// from the hard disk, falling back on a one-song "Game Soundtrack" whose file
// doesn't exist (the game ships without music).
#ifndef AFTERSHOCK_MUSICMANAGER_H
#define AFTERSHOCK_MUSICMANAGER_H

#include <xtl.h>

// 2048 16-bit stereo samples per packet.
#define PACKET_SIZE     (2048 * 2 * 2)
#define PACKET_TIME     (2048.0f / 44100.0f)
#define PACKET_COUNT    43
#define EXTRA_PACKETS   10

#define MAX_SOUNDTRACK_NAME 32
#define MAX_SONG_NAME       32

extern const FLOAT FADE_TIME;

enum MM_STATE
{
    MM_STOPPED,
    MM_PAUSED,
    MM_PLAYING,
};

// The game's own songs, used by the "Game Soundtrack".
struct GAMESONG
{
    const WCHAR* strName;
    const char*  strFile;
    DWORD        dwLength;
};
#define NUM_GAME_SONGS 1
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

    HRESULT Play(void* pUnused);
    HRESULT Stop(void* pUnused);
    HRESULT SetGlobal(BOOL bGlobal);
    HRESULT GetCurrentInfo(WCHAR* strSoundtrack, WCHAR* strSong, DWORD* pdwLength);
    FLOAT   GetPlaybackPosition();
    HRESULT NextSoundtrack();
    HRESULT NextSong();
    HRESULT PrevSong();
    HRESULT Process();

    DWORD   m_dwLastPacketTime;                 // +0x00
    DWORD   m_dwUnused[2];                      // +0x04
    BOOL    m_bGlobal;                          // +0x0C go on to the next soundtrack at the end of one
    BOOL    m_bRandom;                          // +0x10
    FLOAT   m_fVolume;                          // +0x14 unused
    int     m_mmState;                          // +0x18 MM_STATE
    DWORD   m_dwPacketsCompleted;               // +0x1C

    CSoundtrack* m_aSoundtracks;                // +0x20
    UINT    m_uSoundtrackCount;                 // +0x24
    UINT    m_uCurrentSoundtrack;               // +0x28
    WCHAR   m_strSong[MAX_SONG_NAME];           // +0x2C
    DWORD   m_dwLength;                         // +0x6C
    DWORD   m_dwSong;                           // +0x70
    DWORD   m_dwSongID;                         // +0x74
    HANDLE  m_hDecodingFile;                    // +0x78

    XWmaFileMediaObject* m_pDecoder;            // +0x7C
    LPDIRECTSOUNDSTREAM  m_pStream[2];          // +0x80
    DWORD   m_dwStream;                         // +0x88 the stream being fed
    BYTE*   m_pSampleData;                      // +0x8C

    DWORD   m_adwStatus[PACKET_COUNT + EXTRA_PACKETS];  // +0x90

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
};                                              // 0x164 bytes

#endif
