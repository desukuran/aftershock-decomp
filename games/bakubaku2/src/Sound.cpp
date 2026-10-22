// The sound effects: DirectMusic segments (eat.wav, go.wav).
#include "Game.h"
#include <dmusici.h>

// 0x001070F4
static IDirectMusicLoader8* g_pLoader;
// 0x001070F8
static IDirectMusicPerformance8* g_pPerformance;

// 0x0001A8E0
// Quirk: the default audio path type is 1, not one of the DMUS_APATH types.
static void InitDirectMusic()
{
    IDirectMusicHeap* pNormalHeap;
    IDirectMusicHeap* pPhysicalHeap;
    DirectMusicCreateDefaultHeap(&pNormalHeap);
    DirectMusicCreateDefaultPhysicalHeap(&pPhysicalHeap);
    DirectMusicInitializeEx(pNormalHeap, pPhysicalHeap, DirectMusicDefaultFactory);
    pNormalHeap->Release();
    pPhysicalHeap->Release();
    DirectMusicCreateInstance(CLSID_DirectMusicLoader, NULL, IID_IDirectMusicLoader8, (void**)&g_pLoader);
    DirectMusicCreateInstance(CLSID_DirectMusicPerformance, NULL, IID_IDirectMusicPerformance8,
                              (void**)&g_pPerformance);
    g_pPerformance->InitAudioX(1, 64, 128, 0);
}

// 0x0001A970
// DirectMusic starts with the first sound loaded.
void CSoundEffect::Load(const char* strFile)
{
    if (g_pLoader == NULL && g_pPerformance == NULL)
        InitDirectMusic();
    g_pLoader->LoadObjectFromFile(CLSID_DirectMusicSegment, IID_IDirectMusicSegment8, strFile,
                                  (void**)&m_pSegment);
}

// 0x0001A8B0
void CSoundEffect::Play()
{
    g_pPerformance->PlaySegmentEx(m_pSegment, NULL, NULL, DMUS_SEGF_SECONDARY, 0, NULL, NULL, NULL);
}
