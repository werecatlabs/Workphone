// Hardware probe of the same output worker exercised by WPAudio.output.
#define WIN32_LEAN_AND_MEAN
#if defined(_WIN32_WINNT) && _WIN32_WINNT < 0x0A00
#undef _WIN32_WINNT
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <windows.h>
#include <xaudio2.h>
#include <WPAudio/CoreOutputPump.hpp>
#include <cmath>
#include <iostream>
#include <vector>
#include <cstring>
using workphone::audio::CoreOutputPump;
struct XAudioSink final : CoreOutputPump::Sink, IXAudio2VoiceCallback {
    IXAudio2 *engine=nullptr;
    IXAudio2MasteringVoice *master=nullptr;
    IXAudio2SourceVoice *source=nullptr;
    Completion callback=nullptr; void *user=nullptr;
    HRESULT failure=S_OK;
    bool endpointMissing=false;
    bool open(unsigned rate,Completion cb,void *value) override {
        callback=cb;user=value;
        failure=XAudio2Create(&engine);if(FAILED(failure))return false;
        failure=engine->CreateMasteringVoice(&master,2,rate);
        if(FAILED(failure)){endpointMissing=failure==HRESULT_FROM_WIN32(ERROR_NOT_FOUND);return false;}
        WAVEFORMATEX format{};format.wFormatTag=WAVE_FORMAT_IEEE_FLOAT;format.nChannels=2;
        format.nSamplesPerSec=rate;format.wBitsPerSample=32;format.nBlockAlign=8;format.nAvgBytesPerSec=rate*8;
        failure=engine->CreateSourceVoice(&source,&format,0,1,this);
        if(FAILED(failure))return false;
        failure=source->Start();return SUCCEEDED(failure);
    }
    bool submit(const float *data,unsigned frames,unsigned slot) override {
        XAUDIO2_BUFFER buffer{};buffer.AudioBytes=frames*8;
        buffer.pAudioData=reinterpret_cast<const BYTE*>(data);
        buffer.pContext=reinterpret_cast<void*>(static_cast<uintptr_t>(slot+1));
        return SUCCEEDED(source->SubmitSourceBuffer(&buffer));
    }
    void close() noexcept override {
        if(source){source->Stop();source->DestroyVoice();source=nullptr;}
        if(master){master->DestroyVoice();master=nullptr;}
        if(engine){engine->Release();engine=nullptr;}
    }
    void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
    void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
    void STDMETHODCALLTYPE OnStreamEnd() override {}
    void STDMETHODCALLTYPE OnBufferStart(void*) override {}
    void STDMETHODCALLTYPE OnBufferEnd(void *slot) override {callback(user,unsigned(reinterpret_cast<uintptr_t>(slot)-1),true);}
    void STDMETHODCALLTYPE OnLoopEnd(void*) override {}
    void STDMETHODCALLTYPE OnVoiceError(void*,HRESULT) override {callback(user,0,false);}
};
int main() {
    const auto com=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    if(FAILED(com)) return 1;
    XAudioSink sink;CoreOutputPump pump;
    wp_audio_context_desc desc{sizeof(desc),48000,128,256};
    if(pump.start(sink,desc)!=WP_AUDIO_OK){
        std::cout<<(sink.endpointMissing?"SKIP: no XAudio2 endpoint, HRESULT ":"XAudio2 open failed, HRESULT ")<<std::hex<<sink.failure<<'\n';
        CoUninitialize();return sink.endpointMissing?77:1;
    }
    std::vector<unsigned char> wav(44+48000*4);
    auto u16=[&](size_t at,uint16_t v){wav[at]=(unsigned char)v;wav[at+1]=(unsigned char)(v>>8);};
    auto u32=[&](size_t at,uint32_t v){for(size_t i=0;i<4;++i)wav[at+i]=(unsigned char)(v>>(i*8));};
    std::memcpy(wav.data(),"RIFF",4);u32(4,(uint32_t)wav.size()-8);
    std::memcpy(wav.data()+8,"WAVEfmt ",8);u32(16,16);u16(20,3);u16(22,1);
    u32(24,48000);u32(28,192000);u16(32,4);u16(34,32);
    std::memcpy(wav.data()+36,"data",4);u32(40,48000*4);
    for(unsigned i=0;i<48000;++i){const float tone=0.1f*std::sin(6.28318530718f*440*i/48000.0f);
        uint32_t bits;std::memcpy(&bits,&tone,4);u32(44+size_t(i)*4,bits);}
    wp_audio_clip *clip=nullptr;int result=1;
    if(wp_audio_clip_decode(wav.data(),wav.size(),&clip)==WP_AUDIO_OK){
        wp_audio_command start{};start.size=sizeof(start);start.type=WP_AUDIO_COMMAND_START;
        start.start={sizeof(start.start),clip,0,1,0,1,0};
        if(pump.submit(start)==WP_AUDIO_OK){
            wp_audio_completion_event event{};event.size=sizeof(event);
            const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
            while(pump.state()==CoreOutputPump::State::Running&&std::chrono::steady_clock::now()<deadline){
                wp_audio_acknowledgement ack{};while(pump.poll(ack)==WP_AUDIO_OK){}
                if(pump.poll(event)==WP_AUDIO_OK){
                    // Render completion precedes sink consumption by at most three blocks.
                    const auto end=pump.submittedBlocks()+1;
                    while(pump.completedBlocks()<end&&pump.state()==CoreOutputPump::State::Running&&std::chrono::steady_clock::now()<deadline)
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    if(event.reason==WP_AUDIO_COMPLETION_NATURAL&&pump.completedBlocks()>=end)result=0;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }
    std::cout<<"XAudio2 completed "<<pump.completedBlocks()<<" core blocks; captured output/listening not established.\n";
    pump.stop();wp_audio_clip_destroy(clip);CoUninitialize();return result;
}
