#include <workphone_audio_core.h>
#include <array>
#include <atomic>
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include <vector>

#define CHECK(x) do { if(!(x)) throw std::runtime_error(#x); } while(0)

namespace
{
    wp_audio_memory_stats memory()
    {
        wp_audio_memory_stats value{}; value.size = sizeof(value);
        CHECK(wp_audio_memory_snapshot(&value) == WP_AUDIO_OK); return value;
    }
    struct Fixture
    {
        wp_audio_context *context = nullptr;
        wp_audio_clip *clip = nullptr;
        float output[32]{};
        Fixture()
        {
            std::array<unsigned char, 60> wav{};
            auto put = [&](size_t at, uint32_t value, size_t bytes) {
                for(size_t i = 0; i < bytes; ++i) wav[at + i] = static_cast<unsigned char>(value >> (8 * i));
            };
            std::copy_n("RIFF", 4, wav.begin()); put(4, 52, 4);
            std::copy_n("WAVEfmt ", 8, wav.begin() + 8); put(16, 16, 4);
            put(20, 1, 2); put(22, 1, 2); put(24, 48000, 4); put(28, 96000, 4);
            put(32, 2, 2); put(34, 16, 2); std::copy_n("data", 4, wav.begin() + 36); put(40, 16, 4);
            for(size_t i = 44; i < wav.size(); i += 2) put(i, 16384, 2);
            CHECK(wp_audio_clip_decode(wav.data(), wav.size(), &clip) == WP_AUDIO_OK);
            wp_audio_context_desc desc{sizeof(desc), 48000, 2, 16};
            CHECK(wp_audio_context_create(&desc, &context) == WP_AUDIO_OK);
            CHECK(wp_audio_context_notifications(context, 1) == WP_AUDIO_OK);
        }
        ~Fixture() { wp_audio_context_destroy(context); wp_audio_clip_destroy(clip); }
        wp_audio_voice_desc description(bool loop = true)
        { return {sizeof(wp_audio_voice_desc), clip, 0, 1, 0, 1, loop ? 1 : 0}; }
        wp_audio_voice_handle voice(bool loop = true)
        {
            auto desc = description(loop); wp_audio_voice_handle handle{};
            CHECK(wp_audio_voice_create(context, &desc, &handle) == WP_AUDIO_OK); return handle;
        }
        void render(uint32_t frames = 16) { CHECK(wp_audio_render(context, output, frames) == WP_AUDIO_OK); }
        wp_audio_completion_event event()
        {
            wp_audio_completion_event value{}; value.size = sizeof(value);
            CHECK(wp_audio_completion_poll(context, &value) == WP_AUDIO_OK); return value;
        }
        void empty()
        {
            wp_audio_completion_event value{}; value.size = sizeof(value);
            CHECK(wp_audio_completion_poll(context, &value) == WP_AUDIO_EMPTY);
        }
    };

    void terminalIdentity()
    {
        Fixture fixture;
        auto first = fixture.voice(false);
        fixture.render();
        CHECK(wp_audio_voice_stop(fixture.context, first) == WP_AUDIO_OK);
        CHECK(wp_audio_voice_release(fixture.context, first) == WP_AUDIO_OK);
        auto second = fixture.voice();
        CHECK(first.slot == second.slot && first.generation != second.generation);
        CHECK(wp_audio_voice_stop(fixture.context, first) == WP_AUDIO_STALE_HANDLE);
        auto event = fixture.event();
        CHECK(event.voice.generation == first.generation && event.playback_id == 1);
        CHECK(event.reason == WP_AUDIO_COMPLETION_NATURAL && event.frame == 8);
        CHECK(wp_audio_voice_restart(fixture.context, second) == WP_AUDIO_OK);
        event = fixture.event();
        CHECK(event.reason == WP_AUDIO_COMPLETION_RESTARTED && event.playback_id == 1 && event.frame == 16);
        CHECK(wp_audio_voice_stop(fixture.context, second) == WP_AUDIO_OK);
        CHECK(wp_audio_voice_stop(fixture.context, second) == WP_AUDIO_OK);
        event = fixture.event();
        CHECK(event.reason == WP_AUDIO_COMPLETION_STOPPED && event.playback_id == 2);
        fixture.empty();
        CHECK(wp_audio_voice_restart(fixture.context, second) == WP_AUDIO_OK);
        CHECK(wp_audio_voice_release(fixture.context, second) == WP_AUDIO_OK);
        event = fixture.event();
        CHECK(event.reason == WP_AUDIO_COMPLETION_RELEASED && event.playback_id == 3);
        fixture.empty();
    }

    void boundedNotifications()
    {
        Fixture fixture;
        auto handle = fixture.voice();
        for(unsigned i = 0; i < 2048; ++i)
        {
            if(i) CHECK(wp_audio_voice_restart(fixture.context, handle) == WP_AUDIO_OK);
            CHECK(wp_audio_voice_stop(fixture.context, handle) == WP_AUDIO_OK);
        }
        CHECK(wp_audio_voice_restart(fixture.context, handle) == WP_AUDIO_LIMIT);
        wp_audio_voice_handle rejected{}; auto desc = fixture.description();
        CHECK(wp_audio_voice_create(fixture.context, &desc, &rejected) == WP_AUDIO_LIMIT);
        auto first = fixture.event(); CHECK(first.playback_id == 1);
        CHECK(wp_audio_voice_restart(fixture.context, handle) == WP_AUDIO_OK);
        // The active run owns the last credit, even though its terminal event is not published yet.
        CHECK(wp_audio_voice_create(fixture.context, &desc, &rejected) == WP_AUDIO_LIMIT);
        CHECK(wp_audio_voice_stop(fixture.context, handle) == WP_AUDIO_OK);
        for(unsigned i = 2; i <= 2049; ++i)
        {
            const auto event = fixture.event();
            CHECK(event.playback_id == i && event.reason == WP_AUDIO_COMPLETION_STOPPED);
        }
        fixture.empty();
        wp_audio_context_stats stats{}; stats.size = sizeof(stats);
        CHECK(wp_audio_context_stats_snapshot(fixture.context, &stats) == WP_AUDIO_OK);
        CHECK(stats.completions_published == 2049 && stats.notification_high_water == 2048);
        CHECK(stats.notification_rejections == 3);
    }

    void commandsAndRetirement()
    {
        Fixture fixture;
        wp_audio_command command{}; command.size = sizeof(command); command.type = WP_AUDIO_COMMAND_START;
        command.start = fixture.description();
        CHECK(wp_audio_command_submit(fixture.context, &command) == WP_AUDIO_OK);
        wp_audio_clip_destroy(fixture.clip); fixture.clip = nullptr;
        const auto before = memory(); fixture.render();
        wp_audio_acknowledgement ack{};
        CHECK(wp_audio_command_poll(fixture.context, &ack) == WP_AUDIO_OK && ack.result == WP_AUDIO_OK);
        command.voice = ack.voice; command.type = WP_AUDIO_COMMAND_MASTER_GAIN; command.value = 0.5f;
        CHECK(wp_audio_command_submit(fixture.context, &command) == WP_AUDIO_OK);
        fixture.render(); CHECK(fixture.output[0] == 0.25f);
        CHECK(wp_audio_command_poll(fixture.context, &ack) == WP_AUDIO_OK && ack.result == WP_AUDIO_OK);
        command.frame = 1;
        CHECK(wp_audio_command_submit(fixture.context, &command) == WP_AUDIO_OK);
        fixture.render(); CHECK(fixture.output[0] == 0);
        CHECK(wp_audio_command_poll(fixture.context, &ack) == WP_AUDIO_OK && ack.result == WP_AUDIO_OK);
        command.type = WP_AUDIO_COMMAND_RELEASE;
        CHECK(wp_audio_command_submit(fixture.context, &command) == WP_AUDIO_OK);
        fixture.render();
        const auto afterRender = memory();
        CHECK(before.allocations == afterRender.allocations && before.deallocations == afterRender.deallocations);
        CHECK(fixture.event().reason == WP_AUDIO_COMPLETION_RELEASED);
        CHECK(wp_audio_command_poll(fixture.context, &ack) == WP_AUDIO_OK && ack.result == WP_AUDIO_OK);
        CHECK(memory().deallocations == before.deallocations + 2);
        fixture.empty();
        command.type = WP_AUDIO_COMMAND_STOP_ALL;
        for(unsigned i = 0; i < 128; ++i) CHECK(wp_audio_command_submit(fixture.context, &command) == WP_AUDIO_OK);
        CHECK(wp_audio_command_submit(fixture.context, &command) == WP_AUDIO_LIMIT);
        fixture.render(0);
        wp_audio_context_stats stats{}; stats.size = sizeof(stats);
        CHECK(wp_audio_context_stats_snapshot(fixture.context, &stats) == WP_AUDIO_OK);
        CHECK(stats.command_high_water == 128 && stats.commands_rejected == 1);
    }

    void concurrentLifecycle()
    {
        Fixture fixture;
        std::atomic<bool> finish{false}, renderFailed{false};
        std::thread renderer([&] {
            while(!finish.load())
                if(wp_audio_render(fixture.context, fixture.output, 16) != WP_AUDIO_OK) renderFailed = true;
        });
        // Join on assertion failure as well as success.
        struct Join { std::atomic<bool> &done; std::thread &thread; ~Join() { done = true; thread.join(); } } join{finish, renderer};
        auto exchange = [&](wp_audio_command &command) {
            CHECK(wp_audio_command_submit(fixture.context, &command) == WP_AUDIO_OK);
            wp_audio_acknowledgement ack{};
            while(wp_audio_command_poll(fixture.context, &ack) == WP_AUDIO_EMPTY) std::this_thread::yield();
            CHECK(ack.result == WP_AUDIO_OK); return ack.voice;
        };
        for(unsigned i = 0; i < 1000; ++i)
        {
            wp_audio_command command{}; command.size = sizeof(command); command.type = WP_AUDIO_COMMAND_START;
            command.start = fixture.description(); command.request_id = i * 2;
            auto handle = exchange(command);
            command.type = WP_AUDIO_COMMAND_RELEASE; command.voice = handle; ++command.request_id;
            exchange(command);
            const auto event = fixture.event();
            CHECK(event.voice.context == handle.context && event.voice.generation == handle.generation);
            CHECK(event.reason == WP_AUDIO_COMPLETION_RELEASED && event.playback_id == 1);
            fixture.empty();
        }
        CHECK(!renderFailed.load());
    }

    void deviceFailure()
    {
        Fixture fixture;
        auto handle = fixture.voice();
        fixture.render();
        wp_audio_command pending{}; pending.size = sizeof(pending); pending.type = WP_AUDIO_COMMAND_START;
        pending.start = fixture.description(); pending.request_id = 713;
        CHECK(wp_audio_command_submit(fixture.context, &pending) == WP_AUDIO_OK);
        wp_audio_clip_destroy(fixture.clip); fixture.clip = nullptr;
        const auto before = memory();
        CHECK(wp_audio_context_fail(fixture.context) == WP_AUDIO_OK);
        CHECK(wp_audio_context_fail(fixture.context) == WP_AUDIO_OK);
        const auto after = memory();
        CHECK(before.allocations == after.allocations && before.deallocations == after.deallocations);
        auto event = fixture.event();
        CHECK(event.reason == WP_AUDIO_COMPLETION_DEVICE_FAILURE && event.voice.generation == handle.generation);
        CHECK(event.frame == 16); fixture.empty();
        wp_audio_voice_state state; uint64_t cursor = 0;
        CHECK(wp_audio_voice_status(fixture.context, handle, &state, &cursor) == WP_AUDIO_OK);
        CHECK(state == WP_AUDIO_FAILED && cursor == 0); // Loop clip wrapped twice.
        CHECK(wp_audio_voice_restart(fixture.context, handle) == WP_AUDIO_DEVICE_ERROR);
        CHECK(wp_audio_command_submit(fixture.context, &pending) == WP_AUDIO_DEVICE_ERROR);
        wp_audio_acknowledgement ack{};
        CHECK(wp_audio_command_poll(fixture.context, &ack) == WP_AUDIO_OK);
        CHECK(ack.result == WP_AUDIO_DEVICE_ERROR && ack.request_id == 713);
        CHECK(wp_audio_command_poll(fixture.context, &ack) == WP_AUDIO_EMPTY);
        CHECK(wp_audio_render(fixture.context, fixture.output, 16) == WP_AUDIO_DEVICE_ERROR);
        CHECK(wp_audio_context_clock(fixture.context) == 16);
        for(float value : fixture.output) CHECK(value == 0);
        CHECK(wp_audio_voice_release(fixture.context, handle) == WP_AUDIO_OK);
        fixture.empty();
        CHECK(memory().deallocations == before.deallocations + 2);
    }

    void admissionRacingFailure()
    {
        // Failure can seal admission between the producer's check and publish.
        // Every accepted command must still receive exactly one error ack.
        for(unsigned cycle = 0; cycle < 1000; ++cycle)
        {
            Fixture fixture;
            std::atomic<bool> begin{false};
            std::thread renderer([&] {
                while(!begin.load()) std::this_thread::yield();
                wp_audio_context_fail(fixture.context);
            });
            wp_audio_command command{}; command.size = sizeof(command); command.type = WP_AUDIO_COMMAND_START;
            command.start = fixture.description();
            begin = true;
            unsigned accepted = 0;
            for(unsigned i = 0; i < 120; ++i)
            {
                command.request_id = i;
                auto result = wp_audio_command_submit(fixture.context, &command);
                if(result == WP_AUDIO_DEVICE_ERROR) break;
                if(result != WP_AUDIO_OK) { renderer.join(); CHECK(false); }
                ++accepted;
            }
            renderer.join();
            wp_audio_clip_destroy(fixture.clip); fixture.clip = nullptr;
            wp_audio_acknowledgement ack{};
            for(unsigned i = 0; i < accepted; ++i)
            {
                CHECK(wp_audio_command_poll(fixture.context, &ack) == WP_AUDIO_OK);
                CHECK(ack.result == WP_AUDIO_DEVICE_ERROR && ack.request_id == i);
            }
            CHECK(wp_audio_command_poll(fixture.context, &ack) == WP_AUDIO_EMPTY);
        }
    }
}

int main()
{
    try
    {
        const auto before = memory();
        terminalIdentity(); boundedNotifications(); commandsAndRetirement(); concurrentLifecycle();
        deviceFailure(); admissionRacingFailure();
        const auto after = memory();
        CHECK(after.resident_bytes == before.resident_bytes);
        CHECK(after.allocations - before.allocations == after.deallocations - before.deallocations);
        std::puts("Audio terminal delivery, admission, master commands and 1000 concurrent cycles passed");
        return 0;
    }
    catch(const std::exception &error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
}
