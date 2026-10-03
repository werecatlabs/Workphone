#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Test/Fakes/SoundFake.hpp>

namespace workphone
{
    namespace test
    {
        SoundFake::SoundFake()
        {
        }

        SoundFake::~SoundFake()
        {
        }

        void SoundFake::getSpectrum( Array<f32> &spectrum, u32 numValues ) const
        {
        }

        void SoundFake::setLoop( bool loop )
        {
        }

        bool SoundFake::isPlaying() const
        {
            return false;
        }

        void SoundFake::stop()
        {
        }

        void SoundFake::setVolume( f32 volume )
        {
        }

        f32 SoundFake::getVolume() const
        {
            return 0.0f;
        }

        bool SoundFake::getLoop() const
        {
            return false;
        }

        void SoundFake::play()
        {
        }
    }  // end namespace test
}  // namespace workphone
