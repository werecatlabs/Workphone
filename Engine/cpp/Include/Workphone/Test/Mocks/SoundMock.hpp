#ifndef SoundMock_h__
#define SoundMock_h__

#include <Workphone/Interface/Sound/ISound.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace test
    {

        class SoundMock : public ISound
        {
        public:
            SoundMock();
            ~SoundMock() override;

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

#endif  // SoundMock_h__
