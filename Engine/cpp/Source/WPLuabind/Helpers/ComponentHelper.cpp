#include <WPLuabind/WPLuabindPCH.hpp>
#include <WPLuabind/Helpers/ComponentHelper.hpp>

namespace workphone
{
    Parameter ComponentHelper::handleEvent( scene::IComponent *component, lua_Integer eventType,
                                            lua_Integer eventValue, const Array<Parameter> &arguments,
                                            SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                                            SmartPtr<IEvent> event )
    {
        return component->handleEvent( static_cast<EventType>( eventType ), eventValue, arguments,
                                       sender, object, event );
    }

    lua_Integer ComponentHelper::getState( const scene::IComponent *component )
    {
        return static_cast<lua_Integer>( component->getState() );
    }

    void ComponentHelper::setState( scene::IComponent *component, lua_Integer state )
    {
        component->setState( static_cast<scene::IComponent::State>( state ) );
    }

    SmartPtr<ISharedObject> ComponentHelper::getStateByTypeId( scene::IComponent *component, u32 typeId )
    {
        return component->getStateByTypeId( typeId );
    }

    lua_Integer ComponentHelper::getCastShadows( const scene::Renderer *renderer )
    {
        return static_cast<lua_Integer>( renderer->getCastShadows() );
    }

    void ComponentHelper::setCastShadows( scene::Renderer *renderer, lua_Integer value )
    {
        renderer->setCastShadows( static_cast<scene::Renderer::CastShadows>( value ) );
    }

    lua_Integer ComponentHelper::getRecieveShadows( const scene::Renderer *renderer )
    {
        return static_cast<lua_Integer>( renderer->getRecieveShadows() );
    }

    void ComponentHelper::setRecieveShadows( scene::Renderer *renderer, lua_Integer value )
    {
        renderer->setRecieveShadows( static_cast<scene::Renderer::RecieveShadows>( value ) );
    }

    lua_Integer ComponentHelper::getReflections( const scene::Renderer *renderer )
    {
        return static_cast<lua_Integer>( renderer->getReflections() );
    }

    void ComponentHelper::setReflections( scene::Renderer *renderer, lua_Integer value )
    {
        renderer->setReflections( static_cast<scene::Renderer::Reflections>( value ) );
    }

    lua_Integer ComponentHelper::getOcculsion( const scene::Renderer *renderer )
    {
        return static_cast<lua_Integer>( renderer->getOcculsion() );
    }

    void ComponentHelper::setOcculsion( scene::Renderer *renderer, lua_Integer value )
    {
        renderer->setOcculsion( static_cast<scene::Renderer::Occulsion>( value ) );
    }

    lua_Integer ComponentHelper::getTireModel( const scene::WheelController *controller )
    {
        return static_cast<lua_Integer>( controller->getTireModel() );
    }

    void ComponentHelper::setTireModel( scene::WheelController *controller, lua_Integer value )
    {
        controller->setTireModel( static_cast<TireModel>( value ) );
    }

    u32 ComponentHelper::getActorFlags( scene::CharacterController *controller )
    {
        return static_cast<u32>( controller->getActorFlags() );
    }

    void ComponentHelper::setActorFlag( scene::CharacterController *controller, u32 flag, bool value )
    {
        controller->setActorFlag( static_cast<physics::ActorFlagEnum>( flag ), value );
    }

    u32 ComponentHelper::getConstraintType( scene::Constraint *constraint )
    {
        return static_cast<u32>( constraint->getType() );
    }

    void ComponentHelper::setConstraintType( scene::Constraint *constraint, u32 type )
    {
        constraint->setType( static_cast<scene::Constraint::Type>( type ) );
    }

    u32 ComponentHelper::getAxisX( scene::Constraint *constraint )
    {
        return static_cast<u32>( constraint->getAxisX() );
    }

    void ComponentHelper::setAxisX( scene::Constraint *constraint, u32 axisX )
    {
        constraint->setAxisX( static_cast<physics::D6MotionEnum>( axisX ) );
    }

    u32 ComponentHelper::getAxisY( scene::Constraint *constraint )
    {
        return static_cast<u32>( constraint->getAxisY() );
    }

    void ComponentHelper::setAxisY( scene::Constraint *constraint, u32 axisY )
    {
        constraint->setAxisY( static_cast<physics::D6MotionEnum>( axisY ) );
    }

    u32 ComponentHelper::getAxisZ( scene::Constraint *constraint )
    {
        return static_cast<u32>( constraint->getAxisZ() );
    }

    void ComponentHelper::setAxisZ( scene::Constraint *constraint, u32 axisZ )
    {
        constraint->setAxisZ( static_cast<physics::D6MotionEnum>( axisZ ) );
    }

    u32 ComponentHelper::getSwing1( scene::Constraint *constraint )
    {
        return static_cast<u32>( constraint->getSwing1() );
    }

    void ComponentHelper::setSwing1( scene::Constraint *constraint, u32 swing1 )
    {
        constraint->setSwing1( static_cast<physics::D6MotionEnum>( swing1 ) );
    }

    u32 ComponentHelper::getSwing2( scene::Constraint *constraint )
    {
        return static_cast<u32>( constraint->getSwing2() );
    }

    void ComponentHelper::setSwing2( scene::Constraint *constraint, u32 swing2 )
    {
        constraint->setSwing2( static_cast<physics::D6MotionEnum>( swing2 ) );
    }

    u32 ComponentHelper::getTwist( scene::Constraint *constraint )
    {
        return static_cast<u32>( constraint->getTwist() );
    }

    void ComponentHelper::setTwist( scene::Constraint *constraint, u32 twist )
    {
        constraint->setTwist( static_cast<physics::D6MotionEnum>( twist ) );
    }

}  // namespace workphone
