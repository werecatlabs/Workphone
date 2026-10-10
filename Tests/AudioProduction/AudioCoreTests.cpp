#include <workphone_audio_core.h>
#include <workphone_audio_mgr.h>
#include <workphone_audio_sound.h>
#include <workphone_audio_listener.h>
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <limits>
#include <chrono>
#define CHECK(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)
static wp_vec3f vector(float x, float y, float z) { return {x,y,z}; }
using Bytes = std::vector<unsigned char>;
static void put16(Bytes &b, size_t at, uint16_t v) { b[at] = (unsigned char)v; b[at+1] = (unsigned char)(v>>8); }
static void put32(Bytes &b, size_t at, uint32_t v) { for (size_t i=0;i<4;++i) b[at+i]=(unsigned char)(v>>(8*i)); }
static Bytes wav(unsigned bits = 16, unsigned channels = 1, unsigned frames = 8, bool floating = false) {
    unsigned bytes = frames * channels * (bits/8);
    Bytes b(44 + bytes + (bytes & 1), 0);
    std::copy_n("RIFF",4,b.begin()); put32(b,4,(uint32_t)b.size()-8);
    std::copy_n("WAVEfmt ",8,b.begin()+8); put32(b,16,16); put16(b,20,floating?3:1);
    put16(b,22,(uint16_t)channels); put32(b,24,48000); put32(b,28,48000*channels*(bits/8));
    put16(b,32,(uint16_t)(channels*(bits/8))); put16(b,34,(uint16_t)bits);
    std::copy_n("data",4,b.begin()+36); put32(b,40,bytes);
    // Constant +0.5, chosen for exact gain/cursor assertions at every PCM width.
    for (size_t i=44;i<44+bytes;i+=bits/8) {
        if (floating) put32(b,i,0x3f000000);
        else if (bits==8) b[i]=192;
        else b[i+bits/8-1]=64;
    }
    return b;
}
static wp_audio_result inspect(const Bytes &b) { wp_audio_wav_info i{}; i.size=sizeof(i); return wp_audio_wav_inspect(b.data(),b.size(),&i); }
static void decode() {
    for (auto bits : {8u,16u,24u,32u}) for (auto channels : {1u,2u}) {
        auto b=wav(bits,channels,7); wp_audio_clip *clip=nullptr;
        CHECK(inspect(b)==WP_AUDIO_OK); CHECK(wp_audio_clip_decode(b.data(),b.size(),&clip)==WP_AUDIO_OK);
        CHECK(wp_audio_clip_frames(clip)==7); wp_audio_clip_destroy(clip);
    }
    auto b=wav();
    for (size_t n=0;n<b.size();++n) { Bytes shortFile(b.begin(),b.begin()+n); CHECK(inspect(shortFile)!=WP_AUDIO_OK); }
    auto bad=b; put32(bad,4,4); CHECK(inspect(bad)==WP_AUDIO_INVALID_FORMAT);
    bad=b; put32(bad,40,0xffffffffu); CHECK(inspect(bad)==WP_AUDIO_INVALID_FORMAT);
    bad=b; put16(bad,32,1); CHECK(inspect(bad)==WP_AUDIO_INVALID_FORMAT);
    bad=b; put32(bad,28,1); CHECK(inspect(bad)==WP_AUDIO_INVALID_FORMAT);
    bad=b; put16(bad,20,0xfffe); CHECK(inspect(bad)==WP_AUDIO_UNSUPPORTED);
    bad=b; put32(bad,24,0); CHECK(inspect(bad)==WP_AUDIO_UNSUPPORTED);
    bad=b; bad.push_back(0); put32(bad,4,(uint32_t)bad.size()-8); CHECK(inspect(bad)==WP_AUDIO_INVALID_FORMAT);
    bad=wav(8,1,1); bad.pop_back(); put32(bad,4,(uint32_t)bad.size()-8); CHECK(inspect(bad)==WP_AUDIO_INVALID_FORMAT);
    bad=b; bad.insert(bad.end(),{'J','U','N','K',1,0,0,0,0,0}); put32(bad,4,(uint32_t)bad.size()-8);
    CHECK(inspect(bad)==WP_AUDIO_OK); // validate legal trailing chunk
    bad[bad.size()-6]=255; CHECK(inspect(bad)==WP_AUDIO_INVALID_FORMAT);
    // data before fmt is legal.
    Bytes reordered(b.begin(),b.begin()+12); reordered.insert(reordered.end(),b.begin()+36,b.end());
    reordered.insert(reordered.end(),b.begin()+12,b.begin()+36); CHECK(inspect(reordered)==WP_AUDIO_OK);
    bad=b; bad.insert(bad.end(),b.begin()+12,b.begin()+36); put32(bad,4,(uint32_t)bad.size()-8);
    CHECK(inspect(bad)==WP_AUDIO_INVALID_FORMAT);
    auto f=wav(32,1,8,true); wp_audio_clip *clip=nullptr;
    CHECK(wp_audio_clip_decode(f.data(),f.size(),&clip)==WP_AUDIO_OK); wp_audio_clip_destroy(clip);
    put32(f,44,0x7fc00000); CHECK(wp_audio_clip_decode(f.data(),f.size(),&clip)==WP_AUDIO_INVALID_FORMAT); CHECK(!clip);
    CHECK(inspect(f)==WP_AUDIO_INVALID_FORMAT);
    // Deterministic malformed corpus: all byte values at all positions.
    for (size_t i=0;i<b.size();++i) for (unsigned value=0;value<256;++value) {
        bad=b; bad[i]=(unsigned char)value;
        auto result=wp_audio_clip_decode(bad.data(),bad.size(),&clip);
        if (result==WP_AUDIO_OK) wp_audio_clip_destroy(clip); else CHECK(!clip);
    }
}
struct Fixture {
    wp_audio_clip *clip=nullptr;
    wp_audio_context *context=nullptr;
    Fixture(unsigned frames=8, unsigned capacity=2) {
        auto b=wav(16,1,frames); CHECK(wp_audio_clip_decode(b.data(),b.size(),&clip)==WP_AUDIO_OK);
        wp_audio_context_desc d{sizeof(d),48000,capacity,256}; CHECK(wp_audio_context_create(&d,&context)==WP_AUDIO_OK);
    }
    ~Fixture() { wp_audio_context_destroy(context); wp_audio_clip_destroy(clip); }
    wp_audio_voice_handle start(float gain=1, float pan=0, bool loop=false, uint64_t when=0) {
        wp_audio_voice_desc d{sizeof(d),clip,when,gain,pan,1,loop}; wp_audio_voice_handle h{};
        CHECK(wp_audio_voice_create(context,&d,&h)==WP_AUDIO_OK); return h;
    }
    std::vector<float> render(unsigned n) {
        std::vector<float> out(n*2); CHECK(wp_audio_render(context,out.data(),n)==WP_AUDIO_OK); return out;
    }
};
static void contracts() {
    Fixture f; auto a=f.start(), b=f.start();
    wp_audio_voice_desc d{sizeof(d),f.clip,0,1,0,1,0}; wp_audio_voice_handle c{};
    CHECK(wp_audio_voice_create(f.context,&d,&c)==WP_AUDIO_LIMIT);
    CHECK(wp_audio_voice_release(f.context,a)==WP_AUDIO_OK);
    CHECK(wp_audio_voice_release(f.context,a)==WP_AUDIO_STALE_HANDLE);
    c=f.start(); CHECK(c.slot==a.slot && c.generation!=a.generation);
    CHECK(wp_audio_voice_gain(f.context,a,1)==WP_AUDIO_STALE_HANDLE);
    Fixture other; CHECK(wp_audio_voice_stop(other.context,c)==WP_AUDIO_STALE_HANDLE);
    wp_audio_clip_destroy(other.clip); other.clip=nullptr; // no active voice in this context
    CHECK(wp_audio_voice_gain(f.context,b,std::numeric_limits<float>::quiet_NaN())==WP_AUDIO_INVALID_ARGUMENT);
    CHECK(wp_audio_context_gain(f.context,2,0)==WP_AUDIO_INVALID_ARGUMENT);
    float out[2]{}; CHECK(wp_audio_render(f.context,out,257)==WP_AUDIO_INVALID_ARGUMENT);
    CHECK(wp_audio_context_clock(f.context)==0);
    CHECK(wp_audio_voice_seek(f.context,b,9)==WP_AUDIO_INVALID_ARGUMENT);
    auto mgr=wp_audio_mgr_create(); CHECK(mgr); CHECK(!wp_audio_mgr_load(mgr));
    CHECK(!wp_audio_mgr_platform_init()); wp_audio_mgr_destroy(mgr);
    auto sound=wp_audio_sound_create(); CHECK(sound); CHECK(!wp_audio_sound_load(sound,reinterpret_cast<const wp_c8 *>("missing.wav"),0));
    wp_audio_sound_play(sound); CHECK(!wp_audio_sound_is_loaded(sound)); CHECK(!wp_audio_sound_is_playing(sound));
    wp_audio_sound_destroy(sound);
    auto l=wp_audio_listener_create(); CHECK(l); wp_audio_listener_set_name(l,reinterpret_cast<const wp_c8 *>("listener"));
    CHECK(std::string(reinterpret_cast<const char *>(wp_audio_listener_get_name(l)))=="listener");
    wp_audio_listener_set_orientation(l,vector(2,0,0),vector(1,2,0));
    auto forward=wp_audio_listener_get_forward(l), up=wp_audio_listener_get_up(l);
    CHECK(forward.x==1 && up.y==1 && up.x==0);
    wp_audio_listener_set_orientation(l,vector(0,0,0),vector(0,0,0));
    CHECK(wp_audio_listener_get_forward(l).x==1); wp_audio_listener_destroy(l);
}
static void render() {
    // Stereo ramp verifies channel mapping and fractional resampling, rather
    // than relying only on constant signals which hide cursor errors.
    {
        auto bytes=wav(16,2,8);
        for(unsigned i=0;i<8;++i) { put16(bytes,44+i*4,(uint16_t)(i*4096)); put16(bytes,46+i*4,(uint16_t)(32768-i*4096)); }
        wp_audio_clip *clip=nullptr; wp_audio_context *context=nullptr;
        CHECK(wp_audio_clip_decode(bytes.data(),bytes.size(),&clip)==WP_AUDIO_OK);
        wp_audio_context_desc desc{sizeof(desc),96000,1,32}; CHECK(wp_audio_context_create(&desc,&context)==WP_AUDIO_OK);
        wp_audio_voice_desc voice{sizeof(voice),clip,0,1,0,1,0}; wp_audio_voice_handle h{};
        CHECK(wp_audio_voice_create(context,&voice,&h)==WP_AUDIO_OK);
        float out[8]{}; CHECK(wp_audio_render(context,out,4)==WP_AUDIO_OK);
        CHECK(out[0]==0 && out[2]==0.0625f && out[4]==0.125f && out[6]==0.1875f);
        CHECK(out[1]==-1 && out[5]==0.875f);
        wp_audio_context_destroy(context); wp_audio_clip_destroy(clip);
    }
    Fixture f; auto a=f.start(1,-1), b=f.start(0.5f,1);
    CHECK(wp_audio_context_gain(f.context,0.5f,0)==WP_AUDIO_OK);
    auto out=f.render(2); CHECK(out[0]==0.25f && out[1]==0.125f); // master applied once
    CHECK(wp_audio_voice_pause(f.context,a,1)==WP_AUDIO_OK);
    out=f.render(2); CHECK(out[0]==0 && out[1]==0.125f);
    wp_audio_voice_state state; uint64_t cursor;
    CHECK(wp_audio_voice_status(f.context,a,&state,&cursor)==WP_AUDIO_OK); CHECK(state==WP_AUDIO_PAUSED && cursor==2);
    CHECK(wp_audio_context_gain(f.context,0.25f,1)==WP_AUDIO_OK); out=f.render(1); CHECK(out[0]==0 && out[1]==0);
    CHECK(wp_audio_context_gain(f.context,0.5f,0)==WP_AUDIO_OK);
    CHECK(wp_audio_voice_pause(f.context,a,0)==WP_AUDIO_OK); out=f.render(1); CHECK(out[0]==0.25f && out[1]==0.125f);
    CHECK(wp_audio_voice_stop(f.context,a)==WP_AUDIO_OK); CHECK(wp_audio_voice_stop(f.context,a)==WP_AUDIO_OK);
    wp_audio_completion reason; CHECK(wp_audio_voice_poll(f.context,a,&reason)==WP_AUDIO_OK); CHECK(reason==WP_AUDIO_COMPLETION_STOPPED);
    CHECK(wp_audio_voice_poll(f.context,a,&reason)==WP_AUDIO_OK); CHECK(reason==WP_AUDIO_COMPLETION_NONE);
    out=f.render(2); CHECK(wp_audio_voice_status(f.context,b,&state,&cursor)==WP_AUDIO_OK); CHECK(state==WP_AUDIO_FINISHED && cursor==8);
    CHECK(wp_audio_voice_poll(f.context,b,&reason)==WP_AUDIO_OK); CHECK(reason==WP_AUDIO_COMPLETION_NATURAL);
    CHECK(wp_audio_voice_restart(f.context,a)==WP_AUDIO_OK); out=f.render(1); CHECK(out[0]==0.25f);
    CHECK(wp_audio_voice_seek(f.context,a,8)==WP_AUDIO_OK); out=f.render(1); CHECK(out[0]==0);
    Fixture scheduled; auto s=scheduled.start(1,0,true,3); out=scheduled.render(5);
    CHECK(out[0]==0 && out[4]==0 && out[6]==0.5f);
    out=scheduled.render(20); for (float x:out) CHECK(x==0.5f);
    CHECK(wp_audio_voice_loop(scheduled.context,s,0)==WP_AUDIO_OK); out=scheduled.render(8);
    CHECK(wp_audio_voice_status(scheduled.context,s,&state,&cursor)==WP_AUDIO_OK); CHECK(state==WP_AUDIO_FINISHED);
    // Block partition must not change signal or transport.
    Fixture whole(512), split(512); whole.start(1,0,true); split.start(1,0,true);
    wp_audio_clip_destroy(whole.clip); whole.clip=nullptr; // voice pins last good data
    auto expected=whole.render(256); auto first=split.render(63), second=split.render(193);
    first.insert(first.end(),second.begin(),second.end()); CHECK(first==expected);
    Fixture stress(512,128); for (unsigned i=0;i<128;++i) stress.start(0.001f,0,true);
    auto start=std::chrono::steady_clock::now();
    float block[512]; for(unsigned i=0;i<1000;++i) CHECK(wp_audio_render(stress.context,block,256)==WP_AUDIO_OK);
    auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"128 voices, 1000 x 256-frame blocks: "<<ms<<" ms, average "<<ms/1000<<" ms/block\n";
}
int main(int argc,char **argv) {
    try {
        const std::string suite=argc>1?argv[1]:"all";
        if(suite=="decode"||suite=="all") decode();
        if(suite=="contracts"||suite=="all") contracts();
        if(suite=="render"||suite=="all") render();
        std::cout<<suite<<" passed\n"; return 0;
    } catch(const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
