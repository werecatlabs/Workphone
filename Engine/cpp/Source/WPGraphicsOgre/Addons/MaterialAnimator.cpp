#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include "WPGraphicsOgre/Addons/MaterialAnimator.hpp"

namespace workphone
{
    namespace render
    {

        MaterialAnimator::MaterialAnimator()
        {
        }

        MaterialAnimator::~MaterialAnimator()
        {
        }

        void MaterialAnimator::setId( u32 id )
        {
        }

        u32 MaterialAnimator::getId() const
        {
            return 0;
        }

        void MaterialAnimator::start()
        {
        }

        void MaterialAnimator::stop()
        {
        }

        void MaterialAnimator::setLoop( bool loop )
        {
        }

        bool MaterialAnimator::isLoop() const
        {
            return false;
        }

        void MaterialAnimator::setReverse( bool reverse )
        {
        }

        bool MaterialAnimator::isReverse() const
        {
            return false;
        }

        bool MaterialAnimator::isFinished() const
        {
            return false;
        }

        void MaterialAnimator::setAnimationLength( f32 animationLength )
        {
        }

        workphone::render::IMaterial *MaterialAnimator::getOwner() const
        {
            return m_owner;
        }

        void MaterialAnimator::setOwner( IMaterial *owner )
        {
            m_owner = owner;
        }

    }  // namespace render
}  // namespace workphone
