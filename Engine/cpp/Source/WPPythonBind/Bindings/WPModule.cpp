#include <WPPythonBind/WPPythonBindPCH.hpp>
#include <WPPythonBind/Bindings/WPModule.hpp>
#include <boost/python.hpp>

#include <WPPythonBind/Helpers/EngineHelper.hpp>
#include <WPPythonBind/Helpers/FactoryHelper.hpp>
#include <WPPythonBind/Helpers/GraphicsSystemHelper.hpp>
#include <WPPythonBind/Helpers/ParamHelper.hpp>
#include <WPPythonBind/Helpers/EntityManagerHelper.hpp>
#include <WPPythonBind/Helpers/EntityHelper.hpp>
#include <WPPythonBind/Helpers/FSMHelper.hpp>
#include <WPPythonBind/Helpers/SceneManagerHelper.hpp>
#include <WPPythonBind/Helpers/PythonHelper.hpp>
#include <WPPythonBind/Bindings/BindComponents.hpp>
#include <WPPythonBind/Bindings/BindCore.hpp>
#include <WPPythonBind/Bindings/BindGraphics.hpp>
#include <WPPythonBind/Bindings/BindBaseObjects.hpp>
#include <WPPythonBind/Bindings/BindPhysics.hpp>
#include <WPPythonBind/Converters/smart_ptr_from_python.hpp>
#include <WPPythonBind/Converters/smart_ptr_to_python.hpp>
#include <WPPythonBind/Converters/StringConverter.hpp>
#include <Workphone/Workphone.hpp>
#include <WPApplication/WPApplication.hpp>

namespace fb
{
    static bool initialised = false;

    void bindGame()
    {
        using namespace fb::core;
        using namespace boost::python;

        //class_<IApplication, ApplicationPtr, bases<IScriptObject>, boost::noncopyable>( "IApplication", no_init )
        //	.def("initialise", pure_virtual(&IApplication::initialise))
        //	.def("shutdown", pure_virtual(&IApplication::shutdown))
        //	.def("run", pure_virtual(&IApplication::run))
        //	.def("getEngine", pure_virtual(&IApplication::getEngine))
        //	.def("setEngine", pure_virtual(&IApplication::setEngine))
        //	;

        //class_<GameApplication, GameApplicationPtr, bases<IApplication>>("GameApplication")
        //	;

        //class_<GameApp, GameAppPtr, bases<GameApplication>>("GameApp")
        //	;

        /*
        class_<IMap, MapPtr, bases<IObject>, boost::noncopyable>( "IMap", no_init )
            .def( "load", &IMap::load );

        class_<IMapManager, MapManagerPtr, bases<IObject>, boost::noncopyable>( "IMapManager", no_init )
            .def( "addMap", &IMapManager::addMap );
        */
    }

    void bindEntity()
    {
        using namespace boost::python;

        /*
        class_<IFSMContainer, FSMContainerPtr, bases<IObject>, boost::noncopyable>( "IFSMContainer",
                                                                                    no_init )
            //.def("addFSM", &IFSMContainer::addFSM )
            .def( "removeFSM", &IFSMContainer::removeFSM )
            .def( "getFSM", FSMHelper::_getFSM )
            .def( "getFSM", FSMHelper::_getFSMByName );

        class_<IFSM, SmartPtr<IFSM>, bases<IObject>, boost::noncopyable>( "IFSM", no_init )
            .def( "setState", FSMHelper::setFSMState )
            .def( "getState", FSMHelper::getFSMState )
            .def( "getPreviousState", &IFSM::getPreviousState )
            .def( "getNewState", &IFSM::getNewState )
            .def( "setInitialState", FSMHelper::setFSMInitialState )
            .def( "getStateTime", &IFSM::getStateTime );

        class_<IEntityManager, EntityManagerPtr, bases<IObject>, boost::noncopyable>( "IEntityManager",
                                                                                      no_init )
            //.def("handleMessage", &IEntityManager::handleMessage )
            .def( "addEntity", &IEntityManager::addEntity )
            .def( "removeEntity", &IEntityManager::removeEntity )
            .def( "findEntity", &IEntityManager::findEntity )
            .def( "findEntityById", &IEntityManager::findEntityById )

            //.def("getEntities", _getEntities )
            //.def("getEntities", _getEntitiesSphere )
            //.def("getEntities", _getEntitiesByType )
            ;

        class_<IEntity, SmartPtr<scene::IActor>, bases<IObject>, boost::noncopyable>( "IEntity",
                                                                                      no_init )
            .def( "getComponents", EntityHelper::getComponents )
            .def( "getFSMs", EntityHelper::getFSMs )

            .def( "getId", EntityHelper::getEntityId )
            .def( "setId", EntityHelper::setEntityId )

            .def( "getType", EntityHelper::getEntityTypeId )
            .def( "setType", EntityHelper::setEntityTypeId )

            .def( "getFactoryType", EntityHelper::getFactoryType )
            .def( "setFactoryType", EntityHelper::setFactoryType );

        class_<StandardGameObject, StandardGameSmartPtr<ISharedObject>, bases<IEntity>>(
            "StandardGameObject", no_init );

        class_<GameObject, GameSmartPtr<ISharedObject>, bases<IEntity>, boost::noncopyable>(
            "GameObject", no_init );

        //smart_ptr_to_python<SmartPtr<scene::IActor>>();
        converter::smart_ptr_from_python<IEntity>();
        converter::smart_ptr_from_python<StandardGameObject>();
        converter::smart_ptr_from_python<GameObject>();
        */
    }

    void bindEngine()
    {
        using namespace fb::core;
        using namespace boost::python;

        class_<scene::ISceneManager, SmartPtr<scene::ISceneManager>, bases<ISharedObject>,
               boost::noncopyable>( "ISceneManager", no_init )
            .def( "loadScene", &scene::ISceneManager::loadScene );

        //class_<Engine, Engine*, bases<FBObject, Singleton<Engine>>>( "Engine" )
        //	;

        class_<IApplicationManager, SmartPtr<IApplicationManager>, bases<ISharedObject>,
               boost::noncopyable>( "IApplicationManager", no_init )
            .def( "instance", EngineHelper::instance )
            .staticmethod( "instance" )
            .def( "getTimer", EngineHelper::getTimer )
            .def( "getSceneManager", &core::IApplicationManager::getSceneManager )

            //.def("getStateManager", EngineHelper::getStateManager )

            //.def("getFactory", EngineHelper::getFactory )
            //.def("getFileSystem", EngineHelper::getFileSystem )
            .def( "getGraphicsSystem", EngineHelper::getGraphicsSystem )
            //.def("getInputManager", EngineHelper::getInputManager )
            //.def("getEntityManager", EngineHelper::getEntityManager )
            //.def("getPhysicsManager2", EngineHelper::getPhysicsManager2 )
            //.def("getPhysicsManager3", EngineHelper::getPhysicsManager3 )
            //.def("getSoundManager", &Engine::getSoundManager )
            ////.def("getFlashControlManager", &Engine::getFlashControlManager )
            //.def("getVideoManager", &Engine::getVideoManager )
            //.def("getCombatManager2", &Engine::getCombatManager2 )
            //.def("getEntityMessageDispatcher", &Engine::getEntityMessageDispatcher )
            //.def("getProjectileManager2", &Engine::getProjectileManager2 )
            //.def("getGameManager", &Engine::getGameManager )
            //.def("getGUIManager", &Engine::getGUIManager )
            //.def("getAiManager", &Engine::getAiManager )
            ////.def("getCameraControllerMgr", &Engine::getCameraControllerMgr )
            //.def("getSpecialFxManager", &Engine::getSpecialFxManager )
            //.def("getMapManager", EngineHelper::getMapManager )

            //.def("hasFocus", &Engine::hasFocus )
            //.def("setFocus", &Engine::setHasFocus )
            //.
            ;

        class_<ITimer, SmartPtr<ITimer>, bases<ISharedObject>, boost::noncopyable>( "ITimer", no_init )
            //.def( "getTime", &ITimer::getTime )
            .def( "getTimeSinceLevelLoad", &ITimer::getTimeSinceLevelLoad )
            .def( "getTimeInterval", &ITimer::getTimeInterval )
            .def( "getTimeMilliseconds", &ITimer::getTimeMilliseconds )
            .def( "getRealTime", &ITimer::getRealTime );
    }

    void bindParam()
    {
        using namespace boost::python;

        class_<Parameter>( "Param" )
            //.def(constructor<>())
            //.def(constructor<Param&>())
            //.def(constructor<const c8*>())
            //.def(constructor<bool>())
            //.def(constructor<s32>())
            //.def(constructor<u32>())
            //.def(constructor<f32>())
            //.def(constructor<f64>())
            //.def(constructor<void*>())
            .def( "setBool", &Parameter::setBool )
            .def( "setInt", &Parameter::setS64 )
            .def( "setNumber", &Parameter::setF64 )
            .def( "getBool", ParamHelper::getBool )
            .def( "getInt", ParamHelper::getInt )
            .def( "getNumber", ParamHelper::getNumber )
            .def( "getScriptObject", ParamHelper::getScriptObject );

        class_<Parameters>( "ParamList" )
            .def( "addAsBool", ParamHelper::addParamAsBool )
            .def( "addAsInt", ParamHelper::addParamAsInt )
            .def( "addAsNumber", ParamHelper::addParamAsNumber )
            .def( "get", ParamHelper::getParam )
            .def( "getAsBool", ParamHelper::getParamAsBool )
            .def( "getAsInt", ParamHelper::getParamAsInt )
            .def( "getAsNumber", ParamHelper::getParamAsNumber )
            .def( "getAsObject", ParamHelper::getParamAsObject )
            .def( "getAsEntity", ParamHelper::getParamAsEntity )
            .def( "getAsStateMessage", ParamHelper::getParamAsStateMessage )

            .def( "size", ParamHelper::getListSize );
    }

    void initTemplates()
    {
        using namespace boost::python;

        /*
        class_<ITemplate, TemplatePtr, bases<IObject>, boost::noncopyable>( "ITemplate", no_init );

        class_<TerrainTemplate, TerrainTemplatePtr, bases<IObject>, boost::noncopyable>(
            "TerrainTemplate", no_init )
            .def( "setHeightData", &TerrainTemplate::setHeightData )
            .def( "getHeightData", &TerrainTemplate::getHeightData );

        class_<CarTemplate, CarTemplatePtr, bases<IObject>, boost::noncopyable>( "CarTemplate", no_init )
            .def( "getSceneFileName", &CarTemplate::getSceneFileName )
            .def( "setSceneFileName", &CarTemplate::setSceneFileName );

        class_<VehicleTemplate, VehicleTemplatePtr, bases<IObject>, boost::noncopyable>(
            "VehicleTemplate", no_init );

        class_<WaterTemplate, WaterTemplatePtr, bases<IObject>, boost::noncopyable>( "WaterTemplate",
                                                                                     no_init );

        PythonHelper::registerPointer<ITemplate>();
        PythonHelper::registerPointer<TerrainTemplate>();
        PythonHelper::registerPointer<CarTemplate>();
        PythonHelper::registerPointer<WaterTemplate>();
        */
    }

    void bindMath()
    {
        using namespace boost::python;

        class_<Vector2F>( "Vector2F", init<f32, f32>() )
            .def_readwrite( "x", &Vector2F::x )
            .def_readwrite( "y", &Vector2F::y )

            .def( "normalise", &Vector2F::normalise )
            .def( "getLength", &Vector2F::length )
            .def( "getLengthSquared", &Vector2F::squaredLength )

            // Operators
            .def( self + other<Vector2F>() )
            .def( self - other<Vector2F>() )
            .def( self * other<Vector2F>() )
            .def( self * f32() );

        class_<Vector2I>( "Vector2I", init<s32, s32>() )
            .def_readwrite( "x", &Vector2I::x )
            .def_readwrite( "y", &Vector2I::y )

            .def( "normalise", &Vector2I::normalise )
            .def( "getLength", &Vector2I::length )
            .def( "getLengthSquared", &Vector2I::squaredLength )

            // Operators
            .def( self + other<Vector2I>() )
            .def( self - other<Vector2I>() )
            .def( self * other<Vector2I>() )
            .def( self * f32() );

        class_<Vector3F>( "Vector3F", init<f32, f32, f32>() )
            .def_readwrite( "x", &Vector3F::x )
            .def_readwrite( "y", &Vector3F::y )
            .def_readwrite( "z", &Vector3F::z )

            .def( "crossProduct", &Vector3F::crossProduct )
            .def( "dotProduct", &Vector3F::dotProduct )
            .def( "dotProductABS", &Vector3F::dotProductABS )
            .def( "isZeroLength", &Vector3F::isZeroLength )
            .def( "length", &Vector3F::length )
            .def( "makeCeil", &Vector3F::makeCeil )
            .def( "makeFloor", &Vector3F::makeFloor )
            .def( "midPoint", &Vector3F::midPoint )
            .def( "nornaliseCopy", &Vector3F::normaliseCopy )
            .def( "perpendicular", &Vector3F::perpendicular )

            .def( "normalise", &Vector3F::normalise )
            .def( "getLength", &Vector3F::length )
            .def( "getLengthSquared", &Vector3F::squaredLength )

            // Operators
            .def( self + other<Vector3F>() )
            .def( self - other<Vector3F>() )
            .def( self * other<Vector3F>() )
            .def( self * f32() );

        class_<Vector3I>( "Vector3I", init<s32, s32, s32>() )
            .def_readwrite( "x", &Vector3I::x )
            .def_readwrite( "y", &Vector3I::y )
            .def_readwrite( "z", &Vector3I::z )

            .def( "crossProduct", &Vector3I::crossProduct )
            .def( "dotProduct", &Vector3I::dotProduct )
            .def( "dotProductABS", &Vector3I::dotProductABS )
            .def( "isZeroLength", &Vector3I::isZeroLength )
            .def( "length", &Vector3I::length )
            .def( "makeCeil", &Vector3I::makeCeil )
            .def( "makeFloor", &Vector3I::makeFloor )
            .def( "midPoint", &Vector3I::midPoint )
            .def( "nornaliseCopy", &Vector3I::normaliseCopy )
            .def( "perpendicular", &Vector3I::perpendicular )

            .def( "normalise", &Vector3I::normalise )
            .def( "getLength", &Vector3I::length )
            .def( "getLengthSquared", &Vector3I::squaredLength )

            // Operators
            .def( self + other<Vector3I>() )
            .def( self - other<Vector3I>() )
            .def( self * other<Vector3I>() )
            .def( self * f32() );
    }

    void _setKeyboardAction( IGameInput *gameInput, u32 id, const char *key0, const char *key1 )
    {
        gameInput->getGameInputMap()->setKeyboardAction( id, key0, key1 );
    }

    bool _isPressedDown( const IKeyboardState *keyboardState )
    {
        return keyboardState->isPressedDown();
    }

    python_Integer _getJoystickEventType( const IJoystickState *joystickState )
    {
        return joystickState->getEventType();
    }

    python_Integer _getGameEventType( const IGameInputState *gameInputEvent )
    {
        return gameInputEvent->getEventType();
    }

    python_Integer _getAction( const IGameInputState *gameInputEvent )
    {
        return gameInputEvent->getAction();
    }

    SmartPtr<IGameInputMap> _getGameInputMap( IGameInput *input )
    {
        return input->getGameInputMap();
    }

    void _setKeyboardActionMap( IGameInputMap *map, python_Integer id, const String &key0,
                                const String &key1 )
    {
        map->setKeyboardAction( id, key0, key1 );
    }

    void _getKeyboardAction( IGameInputMap *map, python_Integer id, String &key0, String &key1 )
    {
        map->getKeyboardAction( id, key0, key1 );
    }

    void _setJoystickAction( IGameInputMap *map, python_Integer id, python_Integer button )
    {
        map->setJoystickAction( *reinterpret_cast<u32 *>( &id ), *reinterpret_cast<u32 *>( &button ),
                                IGameInput::UNASSIGNED );
    }

    void _setJoystickAction2( IGameInputMap *map, python_Integer id, python_Integer button0,
                              python_Integer button1 )
    {
        map->setJoystickAction( *reinterpret_cast<u32 *>( &id ), *reinterpret_cast<u32 *>( &button0 ),
                                *reinterpret_cast<u32 *>( &button1 ) );
    }

    void _getJoystickAction( IGameInputMap *map, python_Integer id, python_Integer &button0,
                             python_Integer &button1 )
    {
    }

    u32 _getActionFromButton( IGameInputMap *map, python_Integer button )
    {
        return 0;
    }

    u32 _getActionFromKey( IGameInputMap *map, python_Integer key )
    {
        return 0;
    }

    SmartPtr<IMouseState> _getMouseState( IInputEvent *event )
    {
        return event->getMouseState();
    }

    SmartPtr<IKeyboardState> _getKeyboardState( IInputEvent *event )
    {
        return event->getKeyboardState();
    }

    SmartPtr<IJoystickState> _getJoystickState( IInputEvent *event )
    {
        return event->getJoystickState();
    }

    SmartPtr<IGameInputState> _getGameInputState( IInputEvent *event )
    {
        return event->getGameInputState();
    }

    void bindInput()
    {
        using namespace boost::python;

        /*
        class_<IInputManager, InputManagerPtr, bases<IObject>, boost::noncopyable>( "IInputManager",
                                                                                    no_init )
            .def( "addGameInput", &IInputManager::addGameInput )
            .def( "findGameInput", &IInputManager::findGameInput );

        class_<IGameInputMap, GameInputMapPtr, bases<IObject>, boost::noncopyable>( "IGameInputMap",
                                                                                    no_init )
            .def( "setKeyboardAction", _setKeyboardActionMap )
            .def( "getKeyboardAction", _getKeyboardAction )

            .def( "setJoystickAction", _setJoystickAction )
            .def( "getJoystickAction", _getJoystickAction )

            .def( "getActionFromButton", &IGameInputMap::getActionFromButton )
            .def( "getActionFromKey", &IGameInputMap::getActionFromKey );

        class_<IGameInput, GameInputPtr, bases<IObject>, boost::noncopyable>( "IGameInput", no_init )
            .def( "setId", &IGameInput::setId )
            .def( "getId", &IGameInput::getId )
            .def( "setKeyboardAction", _setKeyboardAction )

            .def( "getPlayerIndex", &IGameInput::getPlayerIndex )
            .def( "setPlayerIndex", &IGameInput::setPlayerIndex )

            .def( "getJoystickId", &IGameInput::getJoystickId )
            .def( "setJoystickId", &IGameInput::setJoystickId )
            .def( "getGameInputMap", _getGameInputMap );

        class_<IMouseState, MouseStatePtr, bases<IObject>, boost::noncopyable>( "IMouseState", no_init );

        class_<IKeyboardState, KeyboardStatePtr, bases<IObject>, boost::noncopyable>( "IKeyboardState",
                                                                                      no_init )
            .def( "isPressedDown", &IKeyboardState::isPressedDown );

        class_<IJoystickState, JoystickStatePtr, bases<IObject>, boost::noncopyable>( "IJoystickState",
                                                                                      no_init )
            .def( "getEventType", _getJoystickEventType );
        class_<IGameInputState, GameInputStatePtr, bases<IObject>, boost::noncopyable>(
            "IGameInputState", no_init )
            .def( "getEventType", _getGameEventType )
            .def( "getAction", _getAction );

        class_<IInputEvent, InputEventPtr, bases<IObject>, boost::noncopyable>( "IInputEvent", no_init )
            .def( "getMouseState", _getMouseState )
            .def( "getKeyboardState", _getKeyboardState )
            .def( "getJoystickState", _getJoystickState )
            .def( "getGameInputState", _getGameInputState )

            .def( "getGameInputId", &IInputEvent::getGameInputId )
            .def( "setGameInputId", &IInputEvent::setGameInputId )
            .def( "getEventType", &IInputEvent::getEventType )
            .def( "setEventType", &IInputEvent::setEventType );
            */
    }

    void initializeConverters()
    {
        using namespace boost::python;

        to_python_converter<String, String_to_python_str>();
        // String_from_python_str();
    }

    //-----------------------------------------------------------------------
    BOOST_PYTHON_MODULE( fb )
    {
        try
        {
            if( !initialised )
            {
                initializeConverters();

                bindBaseObjects();
                bindCore();
                bindEntity();
                bindParam();
                bindEngine();
                bindGame();
                bindGraphics();
                bindComponents();
                bindPhysics();
                initTemplates();
                bindMath();
                bindInput();

                initialised = true;
            }
        }
        catch( boost::python::error_already_set &e )
        {
            auto applicationManager = core::IApplicationManager::instance();
            FB_ASSERT( applicationManager );

            auto scriptManager = applicationManager->getScriptManager();
            if( scriptManager )
            {
                auto debugInfo = scriptManager->getDebugInfo();
                FB_LOG_ERROR( debugInfo );
            }
        }
    }

    // A friendly class.
    class hello
    {
    public:
        hello( const std::string &country )
        {
            this->country = country;
        }
        std::string greet() const
        {
            return "Hello from " + country;
        }

    private:
        std::string country;
    };

    // A function taking a hello object as an argument.
    std::string invite( const hello &w )
    {
        return w.greet() + "! Please come soon!";
    }

    BOOST_PYTHON_MODULE( extending )
    {
        using namespace boost::python;
        class_<hello>( "hello", init<std::string>() )
            // Add a regular member function.
            .def( "greet", &hello::greet )
            // Add invite() as a member of hello!
            .def( "invite", invite );

        // Also add invite() as a regular function to the module.
        //def( "invite", invite );
    }

    void initPythonBinding()
    {
        //init_module_extending();
        //init_module_fb();

        if( PyImport_AppendInittab( "fb", PyInit_fb ) == -1 )
        {
            throw std::runtime_error(
                "Failed to add embedded_hello to the interpreter's "
                "builtin modules" );
        }
    }

}  // namespace fb
