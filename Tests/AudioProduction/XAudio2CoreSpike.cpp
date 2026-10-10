// M0 output spike: production core blocks, one output voice, bounded ring,
// callback signals only; rendering and submission happen on the control owner.
#define WIN32_LEAN_AND_MEAN
#if defined(_WIN32_WINNT) && _WIN32_WINNT < 0x0A00
#undef _WIN32_WINNT
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <windows.h>
#include <xaudio2.h>
#include <workphone_audio_core.h>
#include <array>
#include <atomic>
#include <cmath>
#include <iostream>
#include <vector>
#include <cstring>

struct Callback final : IXAudio2VoiceCallback {
    HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    std::atomic<unsigned> completed{0};
    std::atomic<bool> failed{false};
    void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
    void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
    void STDMETHODCALLTYPE OnStreamEnd() override {}
    void STDMETHODCALLTYPE OnBufferStart(void *) override {}
    void STDMETHODCALLTYPE OnBufferEnd(void *) override { ++completed; SetEvent(event); }
    void STDMETHODCALLTYPE OnLoopEnd(void *) override {}
    void STDMETHODCALLTYPE OnVoiceError(void *, HRESULT) override { failed=true; SetEvent(event); }
    ~Callback() { if(event) CloseHandle(event); }
};
int main() {
    const HRESULT com=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    if(FAILED(com)) { std::cerr<<"COM initialization failed\n"; return 1; }
    IXAudio2 *engine=nullptr;
    IXAudio2MasteringVoice *master=nullptr;
    IXAudio2SourceVoice *source=nullptr;
    wp_audio_context *context=nullptr;
    wp_audio_clip *clip=nullptr;
    Callback callback;
    int result=1;
    const auto cleanup=[&] {
        // DestroyVoice fences callbacks before stack ring/callback/sample data dies.
        if(source) { source->Stop(); source->DestroyVoice(); }
        if(master) master->DestroyVoice();
        if(engine) engine->Release();
        wp_audio_context_destroy(context); wp_audio_clip_destroy(clip); CoUninitialize();
    };
    if(!callback.event || FAILED(XAudio2Create(&engine))) { cleanup(); return 1; }
    auto hr=engine->CreateMasteringVoice(&master,2,48000);
    if(FAILED(hr)) { std::cout<<"SKIP: no XAudio2 output device, HRESULT "<<std::hex<<hr<<'\n'; cleanup(); return 77; }
    WAVEFORMATEX format{};
    format.wFormatTag=WAVE_FORMAT_IEEE_FLOAT; format.nChannels=2; format.nSamplesPerSec=48000;
    format.wBitsPerSample=32; format.nBlockAlign=8; format.nAvgBytesPerSec=384000;
    if(FAILED(engine->CreateSourceVoice(&source,&format,0,1,&callback))) { cleanup(); return 1; }
    // Generated mono float tone; no external fixture or filesystem dependency.
    std::vector<unsigned char> wav(44+48000*4);
    auto u16=[&](size_t at,uint16_t v) { wav[at]=(unsigned char)v; wav[at+1]=(unsigned char)(v>>8); };
    auto u32=[&](size_t at,uint32_t v) { for(size_t i=0;i<4;++i) wav[at+i]=(unsigned char)(v>>(i*8)); };
    std::memcpy(wav.data(),"RIFF",4); u32(4,(uint32_t)wav.size()-8);
    std::memcpy(wav.data()+8,"WAVEfmt ",8); u32(16,16); u16(20,3); u16(22,1);
    u32(24,48000); u32(28,192000); u16(32,4); u16(34,32);
    std::memcpy(wav.data()+36,"data",4); u32(40,48000*4);
    for(unsigned i=0;i<48000;++i) {
        const float tone=0.1f*std::sin(6.28318530718f*440.0f*(float)i/48000.0f);
        uint32_t bits; std::memcpy(&bits,&tone,4); u32(44+(size_t)i*4,bits);
    }
    wp_audio_context_desc desc{sizeof(desc),48000,128,256};
    if(wp_audio_clip_decode(wav.data(),wav.size(),&clip)!=WP_AUDIO_OK ||
       wp_audio_context_create(&desc,&context)!=WP_AUDIO_OK) { cleanup(); return 1; }
    wp_audio_voice_desc voice{sizeof(voice),clip,0,1,0,1,0}; wp_audio_voice_handle handle{};
    if(wp_audio_voice_create(context,&voice,&handle)!=WP_AUDIO_OK) { cleanup(); return 1; }
    std::array<std::array<float,512>,3> ring{};
    unsigned submitted=0;
    const unsigned blocks=188; // 48,128 frames, includes terminal silence.
    const auto submit=[&] {
        auto &buffer=ring[submitted%ring.size()];
        if(wp_audio_render(context,buffer.data(),256)!=WP_AUDIO_OK) return false;
        XAUDIO2_BUFFER data{}; data.AudioBytes=sizeof(float)*512;
        data.pAudioData=reinterpret_cast<const BYTE *>(buffer.data());
        if(submitted+1==blocks) data.Flags=XAUDIO2_END_OF_STREAM;
        if(FAILED(source->SubmitSourceBuffer(&data))) return false;
        ++submitted; return true;
    };
    bool ok=true;
    for(unsigned i=0;i<ring.size();++i) if(!submit()) ok=false;
    if(ok && SUCCEEDED(source->Start())) {
        while(callback.completed.load()<blocks && !callback.failed) {
            if(WaitForSingleObject(callback.event,2000)!=WAIT_OBJECT_0) { ok=false; break; }
            while(submitted<blocks && submitted-callback.completed.load()<ring.size())
                if(!submit()) { ok=false; break; }
            if(!ok) break;
        }
        if(ok && !callback.failed) {
            std::cout<<"XAudio2 consumed "<<callback.completed<<" core blocks at 48 kHz stereo. Output capture/listening not established.\n";
            result=0;
        }
    }
    cleanup(); return result;
}
