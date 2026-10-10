#include <WPAudio/WPAudioPCH.hpp>
#include "WPAudio/WPAudioEffectVolume.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    CAudioEffectVolume::CAudioEffectVolume() = default;

    CAudioEffectVolume::~CAudioEffectVolume() = default;

    void CAudioEffectVolume::process()
    {
        WP_ASSERT( m_input );
        WP_ASSERT( m_output );

        for( size_t i = 0; i < m_numSamples; ++i )
        {
            m_output[i] = m_input[i] * ( m_bypass ? 1.0f : m_volume );
        }
    }

    f32 *CAudioEffectVolume::getInput()
    {
        return m_input;
    }

    void CAudioEffectVolume::setInput( f32 *input )
    {
        m_input = input;
    }

    f32 *CAudioEffectVolume::getOutput()
    {
        return m_output;
    }

    void CAudioEffectVolume::setOutput( f32 *output )
    {
        m_output = output;
    }

    u32 CAudioEffectVolume::getNumSamples() const
    {
        return m_numSamples;
    }

    void CAudioEffectVolume::setNumSamples( u32 numSamples )
    {
        m_numSamples = numSamples;
    }

    bool CAudioEffectVolume::getBypass() const
    {
        return m_bypass;
    }

    void CAudioEffectVolume::setBypass( bool bypass )
    {
        m_bypass = bypass;
    }

    f32 CAudioEffectVolume::getVolume() const
    {
        return m_volume;
    }

    void CAudioEffectVolume::setVolume( f32 volume )
    {
        m_volume = volume;
    }

    u32 CAudioEffectVolume::getSampleRate() const
    {
        return m_sampleRate;
    }

    void CAudioEffectVolume::setSampleRate( u32 sampleRate )
    {
        m_sampleRate = sampleRate;
    }
}  // namespace workphone
