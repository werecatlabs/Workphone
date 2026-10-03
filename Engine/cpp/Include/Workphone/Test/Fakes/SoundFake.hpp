#ifndef SoundFake_h__
#define SoundFake_h__

#include <Workphone/Interface/Sound/ISound.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace test
    {
        class SoundFake : public ISound
        {
        public:
            SoundFake();
            ~SoundFake() override;

            void getSpectrum( Array<f32> &spectrum, u32 numValues ) const override;
            void setLoop( bool loop ) override;
            bool isPlaying() const override;
            void stop() override;
            void setVolume( f32 volume ) override;
            f32 getVolume() const override;
            bool getLoop() const override;

            void play() override;
        };
    }  // end namespace test
}  // namespace workphone

#endif  // SoundFake_h__
