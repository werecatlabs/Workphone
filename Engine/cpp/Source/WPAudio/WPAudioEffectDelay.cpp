#include "WPAudio/WPAudioEffectDelay.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    CAudioEffectDelay::CAudioEffectDelay()
    {
    }

    CAudioEffectDelay::~CAudioEffectDelay()
    {
    }

    void CAudioEffectDelay::process()
    {
    }

    f32 *CAudioEffectDelay::getInput()
    {
        return m_input;
    }

    void CAudioEffectDelay::setInput( f32 *input )
    {
        m_input = input;
    }

    f32 *CAudioEffectDelay::getOutput()
    {
        return m_output;
    }

    void CAudioEffectDelay::setOutput( f32 *output )
    {
        m_output = output;
    }

    u32 CAudioEffectDelay::getNumSamples() const
    {
        return m_numSamples;
    }

    void CAudioEffectDelay::setNumSamples( u32 numSamples )
    {
        m_numSamples = numSamples;
    }

    bool CAudioEffectDelay::getBypass() const
    {
        return m_bBypass;
    }

    void CAudioEffectDelay::setBypass( bool bypass )
    {
        m_bBypass = bypass;
    }

    u32 CAudioEffectDelay::getNumChannels() const
    {
        return m_numChannels;
    }

    void CAudioEffectDelay::setNumChannels( u32 numChannels )
    {
        m_numChannels = numChannels;
    }

    u32 CAudioEffectDelay::getSampleRate() const
    {
        return m_sampleRate;
    }

    void CAudioEffectDelay::setSampleRate( u32 sampleRate )
    {
        m_sampleRate = sampleRate;
    }
}  // namespace workphone
