#ifndef ComponentHelper_h__
#define ComponentHelper_h__

#include <Workphone/Workphone.hpp>
#include <lua.hpp>

namespace workphone
{
    class ComponentHelper
    {
    public:
        static Parameter handleEvent( scene::IComponent *component, lua_Integer eventType,
                                      lua_Integer eventValue, const Array<Parameter> &arguments,
                                      SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                                      SmartPtr<IEvent> event );
        static lua_Integer getState( const scene::IComponent *component );
        static void setState( scene::IComponent *component, lua_Integer state );
        static SmartPtr<ISharedObject> getStateByTypeId( scene::IComponent *component, u32 typeId );

        static lua_Integer getCastShadows( const scene::Renderer *renderer );
        static void setCastShadows( scene::Renderer *renderer, lua_Integer value );
        static lua_Integer getRecieveShadows( const scene::Renderer *renderer );
        static void setRecieveShadows( scene::Renderer *renderer, lua_Integer value );
        static lua_Integer getReflections( const scene::Renderer *renderer );
        static void setReflections( scene::Renderer *renderer, lua_Integer value );
        static lua_Integer getOcculsion( const scene::Renderer *renderer );
        static void setOcculsion( scene::Renderer *renderer, lua_Integer value );

        static lua_Integer getTireModel( const scene::WheelController *controller );
        static void setTireModel( scene::WheelController *controller, lua_Integer value );
        static u32 getActorFlags( scene::CharacterController *controller );
        static void setActorFlag( scene::CharacterController *controller, u32 flag, bool value );

        static u32 getConstraintType( scene::Constraint *constraint );
        static void setConstraintType( scene::Constraint *constraint, u32 type );
        static u32 getAxisX( scene::Constraint *constraint );
        static void setAxisX( scene::Constraint *constraint, u32 axisX );
        static u32 getAxisY( scene::Constraint *constraint );
        static void setAxisY( scene::Constraint *constraint, u32 axisY );
        static u32 getAxisZ( scene::Constraint *constraint );
        static void setAxisZ( scene::Constraint *constraint, u32 axisZ );
        static u32 getSwing1( scene::Constraint *constraint );
        static void setSwing1( scene::Constraint *constraint, u32 swing1 );
        static u32 getSwing2( scene::Constraint *constraint );
        static void setSwing2( scene::Constraint *constraint, u32 swing2 );
        static u32 getTwist( scene::Constraint *constraint );
        static void setTwist( scene::Constraint *constraint, u32 twist );
    };
}  // namespace workphone

#endif  // ComponentHelper_h__
