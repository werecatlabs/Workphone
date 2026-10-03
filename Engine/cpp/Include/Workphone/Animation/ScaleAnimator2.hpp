#ifndef _ScaleAnimator2_H
#define _ScaleAnimator2_H

#include <Workphone/Animation/Animator.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    /**
     * @brief Interpolates the scale of a target object over a specified duration.
     *
     * @tparam T The type of the object to be animated. The object must implement a setScale(const
     * Vector2<real_Num>&) method.
     */
    template <class T>
    class ScaleAnimator2 : public Animator
    {
    public:
        /**
         * @brief Default constructor. Initializes with unit scales and no target object.
         */
        ScaleAnimator2();

        /**
         * @brief Constructs a ScaleAnimator2 for a specific object.
         *
         * @param object Pointer to the object to be animated.
         * @param animationLength Duration of the animation in seconds.
         * @param startScale The scale at the beginning of the animation. Defaults to Vector2::UNIT.
         * @param endScale The scale at the end of the animation. Defaults to Vector2::UNIT.
         */
        ScaleAnimator2( T *object, f32 animationLength,
                        const Vector2<real_Num> &startScale = Vector2<real_Num>::UNIT,
                        const Vector2<real_Num> &endScale = Vector2<real_Num>::UNIT );

        /**
         * @brief Destructor.
         */
        ~ScaleAnimator2() override;

        /**
         * @brief Updates the animation progress and applies the current scale to the target object.
         */
        void update() override;

    protected:
        /// Starting scale of the animation.
        Vector2<real_Num> m_start;
        /// Ending scale of the animation.
        Vector2<real_Num> m_end;

        /// Pointer to the object being animated.
        T *m_object;
    };

    template <class T>
    ScaleAnimator2<T>::ScaleAnimator2() :
        m_start( Vector2<real_Num>::UNIT ),
        m_end( Vector2<real_Num>::UNIT ),
        m_object( nullptr )
    {
    }

    template <class T>
    ScaleAnimator2<T>::ScaleAnimator2( T *object, f32 animationLength,
                                       const Vector2<real_Num> &startScale = Vector2<real_Num>::UNIT,
                                       const Vector2<real_Num> &endScale = Vector2<real_Num>::UNIT ) :
        m_animationLength( animationLength ),
        m_animationTime( 0.0f ),
        m_start( startScale ),
        m_end( endScale ),
        m_object( object )
    {
    }

    template <class T>
    ScaleAnimator2<T>::~ScaleAnimator2()
    {
    }

    template <class T>
    void ScaleAnimator2<T>::update()
    {
        m_animationTime += dt;
        MathF::clamp( m_animationTime, 0.0f, 1.0f );
        Vector2<real_Num> scale =
            m_start + ( m_end - m_start ) * ( m_animationTime * 1.0f / m_animationLength );
        m_object->setScale( scale );
    }
}  // namespace workphone

#endif
