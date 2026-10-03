#ifndef Sound2Mock_h__
#define Sound2Mock_h__

#include <Workphone/Interface/Sound/ISound.hpp>

namespace workphone
{
    namespace test
    {

        class Sound2Mock : public ISound
        {
        public:
            Sound2Mock();
            ~Sound2Mock();

            void setPan( f32 pan ) override;
        };

    }  // end namespace test
}  // namespace workphone

#endif  // Sound2Mock_h__
