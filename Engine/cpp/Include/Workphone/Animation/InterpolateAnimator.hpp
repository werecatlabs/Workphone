#ifndef _InterpolateAnimator_H
#define _InterpolateAnimator_H

#include <Workphone/Animation/Animator.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>

namespace workphone
{
    /** An animator to interpolate a value.
     */
    template <class T>
    class InterpolateAnimator : public Animator
    {
    public:
        /** Default Constructor. */
        InterpolateAnimator() : Animator()
        {
        }

        /** Constructor.
        @param
            animationLength The animation length in seconds.
        @param
            start The start value.
        @param
            end The end value.
        */
        InterpolateAnimator( f32 animationLength, const T &start, const T &end ) :
            m_start( start ),
            m_end( end )
        {
            setAnimationLength( animationLength );
            setAnimationTime( 0.0f );
        }

        /** Destructor. */
        ~InterpolateAnimator() override
        {
        }

        /** */
        void update() override
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto timer = applicationManager->getTimer();

            auto task = Thread::getCurrentTask();
            auto t = timer->getTime();
            auto dt = timer->getDeltaTime();

            if( m_isPlaying )
            {
                m_animationTime += static_cast<f32>( dt ) * ( 1.0f / m_animationLength );
                m_animationTime = MathF::clamp( m_animationTime, 0.0f, 1.0f );

                if( !m_reverse )
                    m_value = m_start + ( m_end - m_start ) * m_animationTime;
                else
                    m_value = m_end + ( m_start - m_end ) * m_animationTime;
            }
        }

        void start() override
        {
            Animator::start();
            m_animationTime = 0.0f;
        }

        bool isFinished() const override
        {
            return m_animationTime >= ( 1.0 - MathF::epsilon() );
        }

        void setStartValue( const T &start )
        {
            m_start = start;
        }

        T getStartValue() const
        {
            return m_start;
        }

        void setEndValue( const T &end )
        {
            m_end = end;
        }

        T getEndValue() const
        {
            return m_end;
        }

        T getValue() const
        {
            return m_value;
        }

        void setReverse( bool reverse ) override
        {
            m_reverse = reverse;
            m_animationTime = 0.0;
        }

    protected:
        T m_start;
        T m_end;
        T m_value;
    };

    using InterpolateFloat = InterpolateAnimator<f32>;
    using InterpolateVector2f = InterpolateAnimator<Vector2<real_Num>>;
    using InterpolateVector3f = InterpolateAnimator<Vector3<real_Num>>;
}  // namespace workphone

#endif
