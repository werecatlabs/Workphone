#pragma once
#include <workphone_audio_core.h>
#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

namespace workphone::audio
{
    // One output stream, one renderer owner, one command producer/poller.
    // Sink close must fence all callbacks, including on partial open failure.
    // submit is bounded/nonblocking and delivers exactly one completion for
    // every accepted block, and none for rejected blocks. close may cancel
    // outstanding blocks after fencing. These are native-adapter obligations.
    // The sink and this object must outlive stop(). Callback code only stores
    // atomics; all rendering/submission runs on the dedicated worker.
    class CoreOutputPump final
    {
    public:
        struct Sink
        {
            using Completion = void (*)(void *, unsigned, bool) noexcept;
            virtual ~Sink() = default;
            virtual bool open(unsigned rate, Completion callback, void *user) = 0;
            virtual bool submit(const float *stereo, unsigned frames, unsigned slot) = 0;
            virtual void close() noexcept = 0;
        };
        enum class State { Stopped, Running, Failed };
        CoreOutputPump() = default;
        ~CoreOutputPump() { stop(); }
        CoreOutputPump(const CoreOutputPump &) = delete;
        CoreOutputPump &operator=(const CoreOutputPump &) = delete;

        // All public operations belong to one control owner. No concurrent
        // start/stop/submit/poll calls by other producers are permitted.
        wp_audio_result start(Sink &sink, const wp_audio_context_desc &desc)
        {
            if(m_context) return WP_AUDIO_INVALID_ARGUMENT;
            auto result = wp_audio_context_create(&desc, &m_context);
            if(result != WP_AUDIO_OK) return result;
            wp_audio_context_notifications(m_context, 1);
            try { for(auto &block : m_blocks) block.resize(size_t(desc.max_block_frames) * 2); }
            catch(...) { stop(); return WP_AUDIO_OUT_OF_MEMORY; }
            m_frames = desc.max_block_frames; m_sink = &sink;
            m_exit = false; m_failed = false; m_submitted = 0; m_completed = 0;
            for(auto &free : m_free) free.store(true);
            try
            {
                if(!sink.open(desc.sample_rate, &completed, this))
                { stop(); return WP_AUDIO_UNSUPPORTED; }
                m_worker = std::thread([this] { run(); });
            }
            catch(...) { stop(); return WP_AUDIO_OUT_OF_MEMORY; }
            return WP_AUDIO_OK;
        }
        void stop() noexcept
        {
            m_exit.store(true);
            if(m_worker.joinable()) m_worker.join();
            if(m_sink) { m_sink->close(); m_sink = nullptr; }
            // Callback fence precedes both ring reuse and context reclamation.
            wp_audio_context_destroy(m_context); m_context = nullptr;
            for(auto &block : m_blocks) block.clear();
        }
        wp_audio_result submit(const wp_audio_command &command)
        {
            if(!m_context) return WP_AUDIO_INVALID_ARGUMENT;
            if(m_failed.load()) return WP_AUDIO_DEVICE_ERROR;
            return wp_audio_command_submit(m_context, &command);
        }
        wp_audio_result poll(wp_audio_acknowledgement &ack)
        { return m_context ? wp_audio_command_poll(m_context, &ack) : WP_AUDIO_EMPTY; }
        wp_audio_result poll(wp_audio_completion_event &event)
        { return m_context ? wp_audio_completion_poll(m_context, &event) : WP_AUDIO_EMPTY; }
        // Failed is visible as soon as a callback signals an error. The worker
        // may still be publishing terminal events; EMPTY is not a drain fence.
        State state() const noexcept
        { return !m_context ? State::Stopped : m_failed.load() ? State::Failed : State::Running; }
        uint64_t submittedBlocks() const noexcept { return m_submitted.load(); }
        uint64_t completedBlocks() const noexcept { return m_completed.load(); }
    private:
        static void completed(void *user, unsigned slot, bool ok) noexcept
        {
            auto &self = *static_cast<CoreOutputPump *>(user);
            if(!ok || slot >= self.m_free.size()) { self.m_failed.store(true); return; }
            self.m_completed.fetch_add(1);
            self.m_free[slot].store(true, std::memory_order_release);
        }
        void run() noexcept
        {
            unsigned next = 0;
            while(!m_exit.load() && !m_failed.load())
            {
                // Submission order is fixed even when callbacks complete in a
                // different order. Never overwrite a buffer still owned by IO.
                if(!m_free[next].load(std::memory_order_acquire))
                { std::this_thread::sleep_for(std::chrono::milliseconds(1)); continue; }
                m_free[next].store(false, std::memory_order_release);
                bool accepted = false;
                try
                {
                    accepted = wp_audio_render(m_context, m_blocks[next].data(), m_frames) == WP_AUDIO_OK &&
                        m_sink->submit(m_blocks[next].data(), m_frames, next);
                }
                catch(...) { accepted = false; }
                if(!accepted) { m_failed.store(true); break; }
                m_submitted.fetch_add(1);
                next = (next + 1) % unsigned(m_blocks.size());
            }
            if(m_failed.load()) wp_audio_context_fail(m_context);
        }
        Sink *m_sink = nullptr;
        wp_audio_context *m_context = nullptr;
        std::array<std::vector<float>, 3> m_blocks;
        std::array<std::atomic<bool>, 3> m_free{};
        std::atomic<bool> m_exit{true}, m_failed{false};
        std::atomic<uint64_t> m_submitted{0}, m_completed{0};
        unsigned m_frames = 0;
        std::thread m_worker;
    };
}
