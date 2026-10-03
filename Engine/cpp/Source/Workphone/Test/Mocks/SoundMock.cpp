#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Test/Mocks/SoundMock.hpp>

namespace workphone
{
    namespace test
    {
        SoundMock::SoundMock()
        {
        }

        SoundMock::~SoundMock()
        {
        }

        void SoundMock::getSpectrum( Array<f32> &spectrum, u32 numValues ) const
        {
        }

        void SoundMock::setLoop( bool loop )
        {
        }

        bool SoundMock::isPlaying() const
        {
            return false;
        }

        void SoundMock::stop()
        {
        }

        void SoundMock::setVolume( f32 volume )
        {
        }

        f32 SoundMock::getVolume() const
        {
            return 0.0f;
        }

        bool SoundMock::getLoop() const
        {
            return false;
        }

        void SoundMock::play()
        {
        }
    }  // end namespace test
}  // namespace workphone
