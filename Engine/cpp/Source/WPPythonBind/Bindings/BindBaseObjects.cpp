#include <WPPythonBind/WPPythonBindPCH.hpp>
#include <WPPythonBind/Bindings/BindBaseObjects.hpp>
#include <WPPythonBind/Helpers/ScriptObjectHelper.hpp>
#include <Workphone/Workphone.hpp>
#include <boost/python.hpp>

namespace fb
{
    void bindBaseObjects()
    {
        using namespace boost::python;
        typedef ScriptObjectFunc<IObject> Functions;

        //class_<IObject, SmartPtr<IObject>, boost::noncopyable>( "IObject", no_init );

        class_<ISharedObject, SmartPtr<ISharedObject>, boost::noncopyable>(
            "ISharedObject", no_init )
            .def( "getReferences", &ISharedObject::getReferences );

        /*
        class_<IObject, SmartPtr<ISharedObject>, boost::noncopyable>( "ScriptObject" )
            .def( "initialise", ScriptObjectHelper::_initialise )
            .def( "initialise", ScriptObjectHelper::_initialiseTemplate<ITemplate> )
            .def( "initialise", ScriptObjectHelper::_initialiseTemplate<CarTemplate> )
            .def( "initialise", ScriptObjectHelper::_initialiseTemplate<CharacterController3Template> )
            .def( "initialise", ScriptObjectHelper::_initialiseTemplate<CollisionShapeTemplate> )
            .def( "initialise", ScriptObjectHelper::_initialiseTemplate<TerrainTemplate> )
            .def( "initialise", ScriptObjectHelper::_initialiseTemplate<VehicleTemplate> )
            .def( "initialise", ScriptObjectHelper::_initialiseTemplate<WaterTemplate> )

            .def( "initialise", ScriptObjectHelper::_initialiseTemplate<ParticleSystemTemplate> )
            //.def("initialise", ScriptObjectHelper::_initialiseTemplateProperties<ParticleTemplate> )

            .def( "update", Functions::scriptObjectUpdate )

            //.def("setFSMContainer", &IObject::setFSMs )
            //.def("getFSMContainer", ScriptObjectHelper::_getFSMContainer )

            //.def( "getFSM", Functions::getFSMByName )
            .def( "getFSM", Functions::getFSM )
            .def( "setProperty", Functions::ScriptObject_setProperty )
            .def( "setProperty", Functions::ScriptObject_setPropertyHash )
            .def( "setAsString", Functions::ScriptObject_setProperty )
            .def( "setAsString", Functions::ScriptObject_setPropertyHash )
            .def( "setAsBool", Functions::setPropertyAsBool )
            .def( "setAsBool", Functions::setPropertyAsBoolHash )
            .def( "setAsInt", Functions::setPropertyAsInt )
            .def( "setAsInt", Functions::setPropertyAsIntHash )
            .def( "setAsUInt", Functions::setPropertyAsUInt )
            .def( "setAsUInt", Functions::setPropertyAsUIntHash )
            .def( "setAsNumber", Functions::setPropertyAsNumber )
            .def( "setAsNumber", Functions::setPropertyAsNumberHash )

            .def( "setAsVector2i", Functions::setPropertyAsVector2i )
            .def( "setAsVector2i", Functions::setPropertyAsVector2iHash )
            .def( "setAsVector2", Functions::setPropertyAsVector2 )
            .def( "setAsVector2", Functions::setPropertyAsVector2Hash )

            .def( "setAsVector3", Functions::setPropertyAsVector3 )
            .def( "setAsVector3", Functions::setPropertyAsVector3Hash )
            .def( "setAsQuaternion", Functions::setPropertyAsQuaternion )
            .def( "setAsQuaternion", Functions::setPropertyAsQuaternionHash )
            .def( "setAsColor", Functions::setPropertyAsColour )
            .def( "setAsColor", Functions::setPropertyAsColourHash )
            .def( "setAsColour", Functions::setPropertyAsColour )
            .def( "setAsColour", Functions::setPropertyAsColourHash )
            .def( "getAsBool", Functions::getPropertyAsBool )
            .def( "getAsBool", Functions::getPropertyAsBoolHash )
            .def( "getAsInt", Functions::getPropertyAsInt )
            .def( "getAsInt", Functions::getPropertyAsIntHash )
            .def( "getAsUInt", Functions::getPropertyAsUInt )
            .def( "getAsUInt", Functions::getPropertyAsUIntHash )
            .def( "getAsNumber", Functions::getPropertyAsNumber )
            .def( "getAsNumber", Functions::getPropertyAsNumberHash )
            .def( "getAsString", Functions::getPropertyAsString )
            .def( "getAsHash", Functions::getPropertyAsHash )

            .def( "getAsVector2i", Functions::getPropertyAsVector2i )
            .def( "getAsVector2i", Functions::getPropertyAsVector2iHash )
            .def( "getAsVector2", Functions::getPropertyAsVector2 )
            .def( "getAsVector2", Functions::getPropertyAsVector2Hash )

            .def( "getAsVector3", Functions::getPropertyAsVector3 )
            .def( "getAsVector3", Functions::getPropertyAsVector3Hash )
            .def( "getAsQuaternion", Functions::getPropertyAsQuaternion )
            .def( "getAsQuaternion", Functions::getPropertyAsQuaternionHash )
            .def( "getAsColor", Functions::getPropertyAsColour )
            .def( "getAsColor", Functions::getPropertyAsColourHash )
            .def( "getAsColour", Functions::getPropertyAsColour )
            .def( "getAsColour", Functions::getPropertyAsColourHash )
            .def( "getAsArray", Functions::getPropertyAsArray )
            .def( "getAsArray", Functions::getPropertyAsArrayHash )

            .def( "callFunction", Functions::callObjectFunction )
            .def( "callFunction", Functions::callObjectFunctionHash )
            .def( "callFunction", Functions::callObjectFunctionAny )
            .def( "callFunction", Functions::callObjectFunctionAnyHash )
            .def( "callFunction", Functions::callObjectFunctionAny2 )
            .def( "callFunction", Functions::callObjectFunctionAny2Hash )
            .def( "callFunction", Functions::callObjectFunctionAny3 )
            .def( "callFunction", Functions::callObjectFunctionAny3Hash )
            .def( "callFunction", Functions::callObjectFunctionParams )
            .def( "callFunction", Functions::callObjectFunctionParamsHash )
            .def( "callFunction", Functions::callObjectFunctionParamsResults )
            .def( "callFunction", Functions::callObjectFunctionParamsResultsHash )
            .def( "callFunction", Functions::callObjectFunctionObj )
            .def( "callFunction", Functions::callObjectFunctionObjHash )

            .def( "setObject", ScriptObjectHelper::setObject )
            .def( "setObject", ScriptObjectHelper::setObjectFromHash )
            //.def("setObject", &IScriptObject::setObject )

            .def( "getObject", Functions::getObject )
            .def( "getObject", Functions::getObjectFromHash )
            .def( "getObjectAsEntity", Functions::getObjectAsEntity )
            .def( "getObjectAsEntity", Functions::getObjectAsEntityFromHash )

            //.def("getStateObject", _getStateObject )
            //.def("setStateObject", &IScriptObject::setStateObject )

            .def( "setCallback", Functions::setCallback )

            //.def("getScriptInstance", ScriptObjectHelper::_getInstance )

            .def( "setUserData", ScriptObjectHelper::setUserDataMap )
            .def( "getUserData", ScriptObjectHelper::getUserDataMap );
            */
    }

}  // namespace fb
