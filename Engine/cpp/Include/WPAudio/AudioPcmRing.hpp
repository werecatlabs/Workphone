#pragma once
#include <array>
#include <cstddef>
#include <atomic>
#include <algorithm>
#include <cstdint>
#include <cstring>

namespace workphone::audio
{
    /** One control producer, one native voice callback consumer. Storage is
     * fixed and preallocated; callback methods never allocate, lock or do IO.
     * reset() requires that the native voice/callback has been quiesced. */
    class AudioPcmRing final
    {
    public:
        static constexpr size_t blockBytes=65536,slotCount=8;
        enum State : uint8_t { Free,Filling,Ready,Queued };
        struct Slot {
            std::atomic<uint8_t> state{Free};
            uint64_t sequence=0,offset=0;
            uint32_t size=0;
            bool dataEnd=false;
            std::array<uint8_t,blockBytes> bytes{};
        };
        struct Buffer {size_t slot=slotCount;const uint8_t *bytes=nullptr;uint32_t size=0;bool dataEnd=false;};
        struct Stats {uint64_t submitted=0,completed=0,underruns=0,silenceBytes=0,position=0;size_t ready=0,pending=0,queued=0;bool finished=false;};
        AudioPcmRing()=default;
        AudioPcmRing(const AudioPcmRing&)=delete;
        AudioPcmRing &operator=(const AudioPcmRing&)=delete;
        void reset(uint64_t position=0) noexcept {
            for(auto &slot:m_slots)slot.state.store(Free,std::memory_order_relaxed);
            m_expected.store(0,std::memory_order_relaxed);m_position.store(position,std::memory_order_relaxed);m_finished.store(false,std::memory_order_relaxed);
        }
        void finish() noexcept {m_finished.store(true,std::memory_order_release);}
        size_t book(uint64_t sequence,uint64_t offset,uint32_t size,bool dataEnd) noexcept {
            if(!size||size>blockBytes)return slotCount;
            for(size_t i=0;i<slotCount;++i){uint8_t expected=Free;if(m_slots[i].state.compare_exchange_strong(expected,Filling,std::memory_order_acquire)){
                auto &slot=m_slots[i];slot.sequence=sequence;slot.offset=offset;slot.size=size;slot.dataEnd=dataEnd;return i;}}
            return slotCount;
        }
        bool fill(size_t index,const void *bytes,size_t size) noexcept {
            if(index>=slotCount||!bytes)return false;auto &slot=m_slots[index];
            if(slot.state.load(std::memory_order_relaxed)!=Filling||size!=slot.size)return false;
            std::memcpy(slot.bytes.data(),bytes,size);slot.state.store(Ready,std::memory_order_release);return true;
        }
        void cancelFill(size_t index) noexcept {if(index<slotCount&&m_slots[index].state.load()==Filling)m_slots[index].state.store(Free,std::memory_order_release);}
        const Slot &slot(size_t index) const noexcept {return m_slots[index];}
        Buffer acquire(uint32_t silenceBytes) noexcept {
            if(m_finished.load(std::memory_order_acquire))return {};
            const auto sequence=m_expected.load(std::memory_order_relaxed);
            for(size_t i=0;i<slotCount;++i){auto &slot=m_slots[i];if(slot.state.load(std::memory_order_acquire)==Ready&&slot.sequence==sequence){
                uint8_t expected=Ready;if(slot.state.compare_exchange_strong(expected,Queued,std::memory_order_acq_rel))return {i,slot.bytes.data(),slot.size,slot.dataEnd};}}
            const auto count=std::min<uint32_t>(silenceBytes,blockBytes);if(!count)return {};
            m_underruns.fetch_add(1,std::memory_order_relaxed);m_silenceBytes.fetch_add(count,std::memory_order_relaxed);return {slotCount,m_silence.data(),count,false};
        }
        void submitted(const Buffer &buffer,bool success) noexcept {
            if(buffer.slot>=slotCount)return;
            if(success){m_expected.fetch_add(1,std::memory_order_relaxed);m_submitted.fetch_add(buffer.size,std::memory_order_relaxed);}
            else m_slots[buffer.slot].state.store(Ready,std::memory_order_release);
        }
        void complete(size_t index,bool final) noexcept {
            if(index>=slotCount)return;auto &slot=m_slots[index];
            m_completed.fetch_add(slot.size,std::memory_order_relaxed);m_position.store(slot.offset+slot.size,std::memory_order_relaxed);
            if(final)m_finished.store(true,std::memory_order_release);
            slot.state.store(Free,std::memory_order_release);
        }
        Stats stats() const noexcept {
            Stats out;out.submitted=m_submitted.load();out.completed=m_completed.load();out.underruns=m_underruns.load();out.silenceBytes=m_silenceBytes.load();out.position=m_position.load();out.finished=m_finished.load();
            for(const auto &slot:m_slots){const auto state=slot.state.load();out.ready+=state==Ready;out.pending+=state==Filling;out.queued+=state==Queued;}return out;
        }
    private:
        std::array<Slot,slotCount> m_slots;
        std::array<uint8_t,blockBytes> m_silence{};
        std::atomic<uint64_t> m_expected{0},m_submitted{0},m_completed{0},m_underruns{0},m_silenceBytes{0},m_position{0};
        std::atomic<bool> m_finished{false};
    };
}
