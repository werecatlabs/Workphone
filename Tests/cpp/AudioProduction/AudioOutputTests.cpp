#include <WPAudio/CoreOutputPump.hpp>
#include <array>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <cstdio>
using workphone::audio::CoreOutputPump;
#define CHECK(x) do { if(!(x)) throw std::runtime_error(#x); } while(0)
struct Sink final : CoreOutputPump::Sink
{
    struct Block { const float *data=nullptr; unsigned frames=0, slot=0; };
    std::array<Block,3> blocks{};
    std::mutex mutex;
    Completion callback=nullptr; void *user=nullptr;
    unsigned count=0, closes=0;
    bool rejectOpen=false, rejectSubmit=false, completeDuringClose=false;
    bool open(unsigned, Completion cb, void *value) override
    { callback=cb; user=value; return !rejectOpen; }
    bool submit(const float *data,unsigned frames,unsigned slot) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        if(rejectSubmit) return false;
        CHECK(count<3); blocks[count++]={data,frames,slot}; return true;
    }
    unsigned queued() { std::lock_guard<std::mutex> lock(mutex); return count; }
    bool consume(float &peak, bool fail=false)
    {
        Block block;
        { std::lock_guard<std::mutex> lock(mutex);
          if(!count) return false;
          block=blocks[0]; for(unsigned i=1;i<count;++i) blocks[i-1]=blocks[i]; --count; }
        for(unsigned i=0;i<block.frames*2;++i) peak=std::max(peak,block.data[i]);
        callback(user,block.slot,!fail); return true;
    }
    void close() noexcept override
    {
        ++closes;
        // Simulate callbacks arriving during native DestroyVoice's fence.
        float ignored=0; if(completeDuringClose) while(consume(ignored)) {}
        count=0; callback=nullptr; user=nullptr;
    }
};
template<class F> void until(F predicate)
{
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    while(!predicate())
    { CHECK(std::chrono::steady_clock::now()<deadline); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
}
int main()
{
    try
    {
        wp_audio_memory_stats before{sizeof(before)},after{sizeof(after)};
        CHECK(wp_audio_memory_snapshot(&before)==WP_AUDIO_OK);
        wp_audio_context_desc desc{sizeof(desc),48000,4,32};
        std::array<unsigned char,48> wav{};
        std::memcpy(wav.data(),"RIFF",4); wav[4]=40;
        std::memcpy(wav.data()+8,"WAVEfmt ",8); wav[16]=16; wav[20]=1; wav[22]=1;
        wav[24]=0x80; wav[25]=0xbb; wav[28]=0; wav[29]=0x77; wav[30]=1;
        wav[32]=2; wav[34]=16; std::memcpy(wav.data()+36,"data",4); wav[40]=4;
        wav[45]=64; wav[47]=64; // Two PCM16 samples at 0.5.
        wp_audio_clip *clip=nullptr;
        CHECK(wp_audio_clip_decode(wav.data(),wav.size(),&clip)==WP_AUDIO_OK);
        Sink sink; CoreOutputPump pump;
        CHECK(pump.start(sink,desc)==WP_AUDIO_OK);
        until([&]{return sink.queued()==3 && pump.submittedBlocks()==3;});
        CHECK(pump.submittedBlocks()==3); // Backpressure: no callback, no overwrite.
        wp_audio_command command{}; command.size=sizeof(command); command.type=WP_AUDIO_COMMAND_START;
        command.start={sizeof(command.start),clip,0,0.5f,0,1,0}; command.request_id=91;
        CHECK(pump.submit(command)==WP_AUDIO_OK);
        wp_audio_clip_destroy(clip); clip=nullptr;
        float peak=0; wp_audio_acknowledgement ack{};
        until([&]{sink.consume(peak); return pump.poll(ack)==WP_AUDIO_OK;});
        CHECK(ack.result==WP_AUDIO_OK&&ack.request_id==91);
        wp_audio_completion_event event{}; event.size=sizeof(event);
        until([&]{sink.consume(peak); return pump.poll(event)==WP_AUDIO_OK;});
        CHECK(event.reason==WP_AUDIO_COMPLETION_NATURAL&&event.voice.generation==ack.voice.generation);
        until([&]{sink.consume(peak); return peak>0;});
        CHECK(peak==0.25f); // Same native renderer through worker and sink.
        sink.completeDuringClose=true; pump.stop(); pump.stop(); CHECK(sink.closes==1);
        CHECK(pump.state()==CoreOutputPump::State::Stopped);
        CHECK(pump.submit(command)==WP_AUDIO_INVALID_ARGUMENT);
        for(int i=0;i<1000;++i)
        {
            Sink cycle; cycle.completeDuringClose=true;
            CHECK(pump.start(cycle,desc)==WP_AUDIO_OK); pump.stop(); CHECK(cycle.closes==1);
        }
        {
            Sink failOpen; CoreOutputPump failing; failOpen.rejectOpen=true;
            CHECK(failing.start(failOpen,desc)==WP_AUDIO_UNSUPPORTED&&failOpen.closes==1);
        }
        {
            Sink failSubmit; CoreOutputPump failing; failSubmit.rejectSubmit=true;
            CHECK(failing.start(failSubmit,desc)==WP_AUDIO_OK);
            until([&]{return failing.state()==CoreOutputPump::State::Failed;}); failing.stop();
        }
        {
            Sink failCallback; CoreOutputPump failing;
            CHECK(failing.start(failCallback,desc)==WP_AUDIO_OK);
            CHECK(wp_audio_clip_decode(wav.data(),wav.size(),&clip)==WP_AUDIO_OK);
            command.type=WP_AUDIO_COMMAND_START; command.start.clip=clip; command.start.loop=1;
            CHECK(failing.submit(command)==WP_AUDIO_OK); wp_audio_clip_destroy(clip); clip=nullptr;
            until([&]{failCallback.consume(peak); return failing.poll(ack)==WP_AUDIO_OK;});
            CHECK(ack.result==WP_AUDIO_OK);
            until([&]{return failCallback.consume(peak,true);});
            CHECK(failing.state()==CoreOutputPump::State::Failed);
            until([&]{return failing.poll(event)==WP_AUDIO_OK;});
            CHECK(event.reason==WP_AUDIO_COMPLETION_DEVICE_FAILURE);
            failing.stop();
        }
        CHECK(wp_audio_memory_snapshot(&after)==WP_AUDIO_OK);
        CHECK(after.resident_bytes==before.resident_bytes);
        CHECK(after.allocations-before.allocations==after.deallocations-before.deallocations);
        std::puts("Core output worker: signal, backpressure, failure, callback fence and 1000 teardown cycles passed");
        return 0;
    }
    catch(const std::exception &error) { std::fprintf(stderr,"%s\n",error.what()); return 1; }
}
