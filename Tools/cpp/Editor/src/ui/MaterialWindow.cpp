#include <EditorPCH.hpp>
#include <ui/MaterialWindow.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <ui/ProjectTreeData.hpp>
#include <ui/PropertiesWindow.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Graphics/MaterialShader.hpp>
#include <Workphone/Graphics/MaterialShaderParameters.hpp>
#include <Workphone/Interface/UI/IUIDropdown.hpp>
#include <Workphone/Interface/UI/IUILabelSliderPair.hpp>
#include <Workphone/Interface/UI/IUIColourPicker.hpp>
#include <Workphone/Interface/UI/IUIText.hpp>
#include <Workphone/Interface/UI/IUIVector2.hpp>
#include <Workphone/Interface/UI/IUIVector4.hpp>
#include <Workphone/Interface/UI/IUILabelTextInputPair.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>

namespace workphone::editor
{

    WP_CLASS_REGISTER_DERIVED( workphone, MaterialWindow, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone, MaterialWindow::DropdownListener, IEventListener );
    WP_CLASS_REGISTER_DERIVED( workphone, MaterialWindow::TreeCtrlListener, IEventListener );

    MaterialWindow::MaterialWindow( SmartPtr<ui::IUIWindow> parent )
    {
        static const auto className = String( "MaterialEditor" );
        setClassName( className );

        setParent( parent );

        m_changeLoadingState = false;
    }

    MaterialWindow::~MaterialWindow()
    {
        unload( nullptr );
    }

    void MaterialWindow::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            ScriptWindow::load( data );

            auto applicationManager = core::IApplicationManager::instance();
            auto factoryManager = applicationManager->getFactoryManager();
            auto ui = applicationManager->getUI();

            auto parent = getParent();

            auto parentWindow = ui->addElementByType<ui::IUIWindow>();
            setParentWindow( parentWindow );

            if( parent )
            {
                parent->addChild( parentWindow );
            }

            auto debugWindow = ui->addElementByType<ui::IUIWindow>();
            setDebugWindow( debugWindow );
            debugWindow->setVisible( false, false );

            if( parent )
            {
                parent->addChild( debugWindow );
            }

            m_dropdown = ui->addElementByType<ui::IUIDropdown>();
            WP_ASSERT( m_dropdown );

            auto dropdownListener = workphone::make_ptr<DropdownListener>();
            dropdownListener->setOwner( this );
            m_dropdown->addObjectListener( dropdownListener );

            auto materialTypes = render::GraphicsUtil::getMaterialTypes();
            m_dropdown->setOptions( materialTypes );

            debugWindow->addChild( m_dropdown );

            auto treeCtrl = ui->addElementByType<ui::IUITreeCtrl>();
            debugWindow->addChild( treeCtrl );
            m_tree = treeCtrl;

            auto treeListener = workphone::make_ptr<TreeCtrlListener>();
            treeListener->setOwner( this );
            m_treeListener = treeListener;

            m_tree->addObjectListener( treeListener );

            m_propertiesWindow = factoryManager->make_ptr<PropertiesWindow>();
            m_propertiesWindow->setParent( debugWindow );
            m_propertiesWindow->load( data );

            // Data-driven shader picker + auto-generated parameter controls (step 4).
            buildShaderUI();

            if( auto invoker = getInvoker() )
            {
                invoker->callObjectMember( "load" );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MaterialWindow::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                destroyScriptObject();

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto ui = applicationManager->getUI();
                WP_ASSERT( ui );

                if( m_dropdown )
                {
                    ui->removeElement( m_dropdown );
                    m_dropdown = nullptr;
                }

                if( m_tree )
                {
                    m_tree->removeObjectListener( m_treeListener );

                    ui->removeElement( m_tree );
                    m_tree = nullptr;
                }

                if( m_treeListener )
                {
                    m_treeListener->unload( nullptr );
                    m_treeListener = nullptr;
                }

                if( m_propertiesWindow )
                {
                    m_propertiesWindow->unload( nullptr );
                    m_propertiesWindow = nullptr;
                }

                if( m_shaderDropdown )
                {
                    if( m_shaderDropdownListener ) m_shaderDropdown->removeObjectListener( m_shaderDropdownListener );
                    ui->removeElement( m_shaderDropdown );
                    m_shaderDropdown = nullptr;
                }
                if( m_shaderParamContainer )
                {
                    ui->removeElement( m_shaderParamContainer );
                    m_shaderParamContainer = nullptr;
                }
                if( m_shaderDropdownListener )
                {
                    m_shaderDropdownListener->unload( nullptr );
                    m_shaderDropdownListener = nullptr;
                }
                if( m_paramValueListener )
                {
                    m_paramValueListener->unload( nullptr );
                    m_paramValueListener = nullptr;
                }

                if( auto parentWindow = getParentWindow() )
                {
                    ui->removeElement( parentWindow );
                    setParentWindow( nullptr );
                }

                for( auto data : m_dataArray )
                {
                    data->unload( nullptr );
                }

                m_dataArray.clear();

                EditorWindow::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MaterialWindow::updateSelection()
    {
        buildTree();
        EditorWindow::updateSelection();
    }

    SmartPtr<render::IMaterial> MaterialWindow::getMaterial() const
    {
        return m_material;
    }

    void MaterialWindow::setMaterial( SmartPtr<render::IMaterial> material )
    {
        m_material = material;
    }

    SmartPtr<ui::IUITreeCtrl> MaterialWindow::getTree() const
    {
        return m_tree;
    }

    void MaterialWindow::setTree( SmartPtr<ui::IUITreeCtrl> tree )
    {
        m_tree = tree;
    }

    void MaterialWindow::buildTree()
    {
        try
        {
            auto tree = getTree();
            if( !tree )
            {
                return;
            }

            if( tree )
            {
                tree->clear();
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto resourceDatabase = applicationManager->getResourceDatabasePtr();

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );
            WP_ASSERT( graphicsSystem->isValid() );

            auto materialManager = graphicsSystem->getMaterialManager();
            auto meshManager = applicationManager->getMeshManager();

            WP_ASSERT( materialManager );
            WP_ASSERT( meshManager );

            auto editorManager = EditorManager::getSingletonPtr();
            auto selectionManager = applicationManager->getSelectionManager();
            WP_ASSERT( selectionManager );

            auto sceneManager = applicationManager->getGameManager();
            WP_ASSERT( sceneManager );

            auto currentScene = sceneManager->getCurrentScene();
            WP_ASSERT( currentScene );

            if( currentScene )
            {
                auto project = editorManager->getProject();
                WP_ASSERT( project );

                auto sceneName = currentScene->getLabel();

                auto selection = selectionManager->getSelection();
                if( selection.size() == 1 )
                {
                    auto selectedObject = selection.front();

                    if( selectedObject->isDerived<FileSelection>() )
                    {
                        auto fileSelection =
                            workphone::dynamic_pointer_cast<FileSelection>( selectedObject );

                        auto filePath = fileSelection->getFilePath();
                        auto ext = Path::getFileExtension( filePath );

                        auto fileInfo = fileSelection->getFileInfo();

                        static const auto materialExt = String( ".mat" );

                        if( ext == materialExt )
                        {
                            auto material =
                                resourceDatabase->loadResourceByType<render::IMaterial>( filePath );
                            if( !material )
                            {
                                auto materialFilePath = fileInfo.absolutePath;
                                material = resourceDatabase->loadResourceByType<render::IMaterial>(
                                    materialFilePath.c_str() );
                            }

                            if( material )
                            {
                                setMaterial( material );

                                auto materialType = material->getMaterialType();
                                m_dropdown->setSelectedOption( static_cast<u32>( materialType ) );

                                auto rootNode = tree->addRoot();
                                WP_ASSERT( rootNode );
                                rootNode->setExpanded( true );

                                addMaterialToTree( material, rootNode );
                            }
                        }
                    }
                    else if( selectedObject->isDerived<scene::Material>() )
                    {
                        auto materialComponent =
                            workphone::dynamic_pointer_cast<scene::Material>( selectedObject );
                        auto material = materialComponent->getMaterial();
                        if( material )
                        {
                            setMaterial( material );

                            auto materialType = material->getMaterialType();
                            m_dropdown->setSelectedOption( static_cast<u32>( materialType ) );

                            auto rootNode = tree->addRoot();
                            WP_ASSERT( rootNode );
                            rootNode->setExpanded( true );

                            addMaterialToTree( material, rootNode );
                        }
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MaterialWindow::addMaterialToTree( SmartPtr<render::IMaterial> material,
                                            SmartPtr<ui::IUITreeNode> node )
    {
        try
        {
            auto editorManager = EditorManager::getSingletonPtr();

            auto project = editorManager->getProject();
            WP_ASSERT( project );

            auto data =
                workphone::make_ptr<ProjectTreeData>( "material", "material", material, material );

            auto actorName = material->getName();
            if( StringUtil::isNullOrEmpty( actorName ) )
            {
                actorName = "Untitled";
            }

            auto treeNode = m_tree->addNode();
            treeNode->setExpanded( true );

            WP_ASSERT( treeNode );
            Util::setText( treeNode, actorName );

            treeNode->setNodeUserData( data );

            if( node )
            {
                node->addChild( treeNode );
            }

            addObjectToTree( material, treeNode );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MaterialWindow::addObjectToTree( SmartPtr<ISharedObject> object,
                                          SmartPtr<ui::IUITreeNode> node )
    {
        try
        {
            if( object )
            {
                auto editorManager = EditorManager::getSingletonPtr();

                auto project = editorManager->getProject();
                WP_ASSERT( project );

                auto data = workphone::make_ptr<ProjectTreeData>( "object", "object", object, object );

                auto typeinfo = object->getTypeInfo();
                WP_ASSERT( typeinfo != 0 );

                auto typeManager = TypeManager::instance();
                WP_ASSERT( typeManager );

                auto className = String( typeManager->getName( typeinfo ) );
                if( StringUtil::isNullOrEmpty( className ) )
                {
                    className = "Untitled";
                }

                if( object->isDerived<IStateContext>() )
                {
                    className = String( "StateObject" );
                }
                else if( object->isDerived<IStateListener>() )
                {
                    className = String( "StateListener" );
                }
                else if( object->isDerived<render::IGraphicsScene>() )
                {
                    className = String( "SceneManager" );
                }
                else if( object->isDerived<render::IGraphicsSceneNode>() )
                {
                    className = String( "SceneNode" );
                }
                else if( object->isDerived<render::IGraphicsMesh>() )
                {
                    className = String( "Mesh" );
                }
                else if( object->isDerived<render::IMaterial>() )
                {
                    className = String( "Material" );
                }
                else if( object->isDerived<render::IMaterialTechnique>() )
                {
                    className = String( "Technique" );
                }
                else if( object->isDerived<render::IMaterialPass>() )
                {
                    className = String( "Pass" );
                }
                else if( object->isDerived<render::IMaterialTexture>() )
                {
                    className = object->getName();
                }

                auto treeNode = m_tree->addNode();

                WP_ASSERT( treeNode );
                Util::setText( treeNode, className );

                treeNode->setNodeUserData( data );

                if( node )
                {
                    node->addChild( treeNode );
                }

                auto children = object->getChildObjects();
                for( auto child : children )
                {
                    if( child )
                    {
                        addObjectToTree( child, treeNode );
                    }
                }

                if( !children.empty() )
                {
                    treeNode->setExpanded( true );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MaterialWindow::handleDropdownSelection()
    {
        auto selectedOption = m_dropdown->getSelectedOption();
        auto materialType = static_cast<MaterialType>( selectedOption );

        if( auto material = getMaterial() )
        {
            material->setMaterialType( materialType );
            material->save();
        }
    }

    void MaterialWindow::setPropertiesWindow( SmartPtr<PropertiesWindow> propertiesWindow )
    {
        m_propertiesWindow = propertiesWindow;
    }

    SmartPtr<PropertiesWindow> MaterialWindow::getPropertiesWindow() const
    {
        return m_propertiesWindow;
    }

    MaterialWindow::TreeCtrlListener::TreeCtrlListener() = default;

    MaterialWindow::TreeCtrlListener::~TreeCtrlListener() = default;

    Parameter MaterialWindow::TreeCtrlListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::handleTreeSelectionActivated )
        {
            if( auto node = workphone::dynamic_pointer_cast<ui::IUITreeNode>( object ) )
            {
                auto data = workphone::dynamic_pointer_cast<ProjectTreeData>( node->getNodeUserData() );
                if( data )
                {
                    auto applicationManager = core::IApplicationManager::instance();
                    auto selectionManager = applicationManager->getSelectionManager();

                    selectionManager->clearSelection();

                    auto component = data->getObjectData();
                    selectionManager->addSelectedObject( component );

                    if( auto propertiesWindow = m_owner->getPropertiesWindow() )
                    {
                        propertiesWindow->updateSelection();
                    }
                }
            }
        }

        return {};
    }

    MaterialWindow *MaterialWindow::TreeCtrlListener::getOwner() const
    {
        return m_owner;
    }

    void MaterialWindow::TreeCtrlListener::setOwner( MaterialWindow *owner )
    {
        m_owner = owner;
    }

    Parameter MaterialWindow::DropdownListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::handleSelection )
        {
            WP_ASSERT( m_owner );
            m_owner->handleDropdownSelection();
        }

        return {};
    }

    MaterialWindow *MaterialWindow::DropdownListener::getOwner() const
    {
        return m_owner;
    }

    void MaterialWindow::DropdownListener::setOwner( MaterialWindow *owner )
    {
        m_owner = owner;
    }

    MaterialWindow::DropdownListener::DropdownListener() = default;

    MaterialWindow::DropdownListener::~DropdownListener() = default;

    //---------------------------------------------------------------------------------------
    // Data-driven shader UI (Esoterica integration, step 4)
    //
    // A shader picker lists every registered MaterialShader (DefaultPBR / Unlit / ColorOnly / ...).
    // Picking one binds the material to that shader (builds a MaterialShaderParametersInstance)
    // and, for the PBR shader, syncs the legacy PBR values into the data-driven buffer. The
    // parameter container is then auto-populated with an editor control per declared parameter:
    //   - Scalar  -> IUILabelSliderPair (0..1)
    //   - Colour  -> IUIColourPicker
    //   - other   -> read-only IUIText label (step 4b will add vector / texture-slot editors)
    // A single ParamValueListener owns the control->parameter bindings and writes edited values
    // straight back into the material's MaterialShaderParametersInstance.
    //---------------------------------------------------------------------------------------

    namespace
    {
        /// Maps one per-parameter editor control back to the named slot it edits.
        struct ShaderParamBinding
        {
            SmartPtr<ui::IUIElement>                   control;
            String                                     name;
            render::MaterialShaderParameterType        type = render::MaterialShaderParameterType::Scalar;
        };
    }

    class MaterialWindow::ShaderDropdownListener : public IEventListener
    {
    public:
        WP_CLASS_REGISTER_DECL;

        ShaderDropdownListener() = default;
        ~ShaderDropdownListener() override = default;

        Parameter handleEvent( EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
                               SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                               SmartPtr<IEvent> event ) override
        {
            if( eventValue == IEvent::handleSelection )
            {
                WP_ASSERT( m_owner );
                if( m_owner ) m_owner->handleShaderDropdownSelection();
            }
            return {};
        }

        MaterialWindow *getOwner() const { return m_owner; }
        void setOwner( MaterialWindow *owner ) { m_owner = owner; }

    private:
        MaterialWindow *m_owner = nullptr;
    };

    class MaterialWindow::ParamValueListener : public IEventListener
    {
    public:
        WP_CLASS_REGISTER_DECL;

        ParamValueListener() = default;
        ~ParamValueListener() override = default;

        Parameter handleEvent( EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
                               SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                               SmartPtr<IEvent> event ) override
        {
            if( eventValue == IEvent::handleValueChanged && object && m_owner )
            {
                for( auto const &b : m_bindings )
                {
                    if( b.control == object )
                    {
                        auto material = workphone::dynamic_pointer_cast<render::Material>( m_owner->getMaterial() );
                        if( material )
                        {
                            auto handle = material->getShaderParameters().FindParameter( b.name );
                            if( handle.IsValid() )
                            {
                                if( b.type == render::MaterialShaderParameterType::Scalar )
                                {
                                    auto slider = workphone::dynamic_pointer_cast<ui::IUILabelSliderPair>( object );
                                    if( slider ) material->getShaderParameters().SetScalar( handle, slider->getValue() );
                                }
                                else if( b.type == render::MaterialShaderParameterType::Colour )
                                {
                                    auto picker = workphone::dynamic_pointer_cast<ui::IUIColourPicker>( object );
                                    if( picker ) material->getShaderParameters().SetColour( handle, picker->getColour() );
                                }
                                else if( b.type == render::MaterialShaderParameterType::Vector2 )
                                {
                                    auto vec = workphone::dynamic_pointer_cast<ui::IUIVector2>( object );
                                    if( vec )
                                    {
                                        auto rv = vec->getValue();
                                        material->getShaderParameters().SetVector2( handle, Vector2<f32>( static_cast<f32>( rv.x ), static_cast<f32>( rv.y ) ) );
                                    }
                                }
                                else if( b.type == render::MaterialShaderParameterType::Vector4 )
                                {
                                    auto vec = workphone::dynamic_pointer_cast<ui::IUIVector4>( object );
                                    if( vec )
                                    {
                                        auto rv = vec->getValue();
                                        material->getShaderParameters().SetVector4( handle, Vector4<f32>( static_cast<f32>( rv.x ), static_cast<f32>( rv.y ), static_cast<f32>( rv.z ), static_cast<f32>( rv.w ) ) );
                                    }
                                }
                                else if( b.type == render::MaterialShaderParameterType::Texture )
                                {
                                    auto pathInput = workphone::dynamic_pointer_cast<ui::IUILabelTextInputPair>( object );
                                    if( pathInput )
                                    {
                                        material->setShaderTexture( b.name, pathInput->getValue() );
                                        material->applyDataDrivenTextures();
                                    }
                                }
                            }
                        }
                        break;
                    }
                }

                // Push the edited value into the legacy renderer path so it renders immediately.
                auto editedMaterial = workphone::dynamic_pointer_cast<render::Material>( m_owner->getMaterial() );
                if( editedMaterial )
                {
                    editedMaterial->applyShaderParametersToRenderer();
                }
            }
            return {};
        }

        void addBinding( SmartPtr<ui::IUIElement> control, const String &name, render::MaterialShaderParameterType type )
        {
            ShaderParamBinding b{ control, name, type };
            m_bindings.push_back( b );
        }

        void clearBindings() { m_bindings.clear(); }

        MaterialWindow *getOwner() const { return m_owner; }
        void setOwner( MaterialWindow *owner ) { m_owner = owner; }

    private:
        MaterialWindow *m_owner = nullptr;
        Array<ShaderParamBinding> m_bindings;
    };

    WP_CLASS_REGISTER_DERIVED( workphone, MaterialWindow::ShaderDropdownListener, IEventListener );
    WP_CLASS_REGISTER_DERIVED( workphone, MaterialWindow::ParamValueListener, IEventListener );

    void MaterialWindow::buildShaderUI()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto ui = applicationManager->getUI();

            render::MaterialShaderRegistry::instance().registerBuiltinShaders();

            m_shaderDropdown = ui->addElementByType<ui::IUIDropdown>();
            WP_ASSERT( m_shaderDropdown );

            auto shaderListener = workphone::make_ptr<ShaderDropdownListener>();
            shaderListener->setOwner( this );
            m_shaderDropdownListener = shaderListener;
            m_shaderDropdown->addObjectListener( shaderListener );

            Array<String> options;
            options.push_back( String( "(legacy PBR)" ) );
            u32 selectedIndex = 0;
            auto & shaders = render::MaterialShaderRegistry::instance().getShaders();
            auto material = workphone::dynamic_pointer_cast<render::Material>( getMaterial() );
            for( size_t i = 0; i < shaders.size(); ++i )
            {
                options.push_back( shaders[i].getShaderID() );
                if( material && shaders[i].getShaderID() == material->getShaderID() )
                {
                    selectedIndex = static_cast<u32>( i + 1 );
                }
            }
            m_shaderDropdown->setOptions( options );
            m_shaderDropdown->setSelectedOption( selectedIndex );

            if( auto dbg = getDebugWindow() ) dbg->addChild( m_shaderDropdown );

            m_shaderParamContainer = ui->addElementByType<ui::IUIWindow>();
            if( auto dbg = getDebugWindow() ) dbg->addChild( m_shaderParamContainer );

            auto paramListener = workphone::make_ptr<ParamValueListener>();
            paramListener->setOwner( this );
            m_paramValueListener = paramListener;

            refreshShaderParameterControls();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MaterialWindow::handleShaderDropdownSelection()
    {
        if( !m_shaderDropdown ) return;
        auto idx = m_shaderDropdown->getSelectedOption();
        auto material = workphone::dynamic_pointer_cast<render::Material>( getMaterial() );
        if( !material ) return;

        if( idx == 0 )
        {
            // "(legacy PBR)" selected: leave the material on its non-data-driven path.
            return;
        }

        auto & shaders = render::MaterialShaderRegistry::instance().getShaders();
        if( idx - 1 >= shaders.size() ) return;
        auto const & shader = shaders[ idx - 1 ];
        material->setShaderParameters( shader.getParameterInfo(), shader.getShaderID() );
        if( shader.getShaderID() == "DefaultPBR" )
        {
            material->syncPBRToShaderParameters();
        }
        material->applyDataDrivenShaderToRenderer();
        refreshShaderParameterControls();
    }

    void MaterialWindow::refreshShaderParameterControls()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto ui = applicationManager->getUI();

        if( m_shaderParamContainer )
        {
            ui->removeElement( m_shaderParamContainer );
            m_shaderParamContainer = ui->addElementByType<ui::IUIWindow>();
            if( auto dbg = getDebugWindow() ) dbg->addChild( m_shaderParamContainer );
        }

        if( auto paramListener = workphone::dynamic_pointer_cast<ParamValueListener>( m_paramValueListener ) )
        {
            paramListener->clearBindings();
        }

        auto material = workphone::dynamic_pointer_cast<render::Material>( getMaterial() );
        if( !material || !material->isDataDriven() ) return;

        auto const & info = material->getShaderParameters().GetParameterInfo();
        for( auto const &p : info )
        {
            auto handle = material->getShaderParameters().FindParameter( p.m_name );
            if( !handle.IsValid() ) continue;

            if( p.m_type == render::MaterialShaderParameterType::Scalar )
            {
                auto slider = ui->addElementByType<ui::IUILabelSliderPair>();
                slider->setLabel( p.m_name );
                slider->setMinValue( 0.0f );
                slider->setMaxValue( 1.0f );
                slider->setValue( material->getShaderParameters().GetScalar( handle ) );
                if( m_paramValueListener ) slider->addObjectListener( m_paramValueListener );
                if( m_shaderParamContainer ) m_shaderParamContainer->addChild( slider );
                if( auto paramListener = workphone::dynamic_pointer_cast<ParamValueListener>( m_paramValueListener ) )
                {
                    paramListener->addBinding( slider, p.m_name, p.m_type );
                }
            }
            else if( p.m_type == render::MaterialShaderParameterType::Colour )
            {
                auto picker = ui->addElementByType<ui::IUIColourPicker>();
                picker->setLabel( p.m_name );
                picker->setColour( material->getShaderParameters().GetColour( handle ) );
                if( m_paramValueListener ) picker->addObjectListener( m_paramValueListener );
                if( m_shaderParamContainer ) m_shaderParamContainer->addChild( picker );
                if( auto paramListener = workphone::dynamic_pointer_cast<ParamValueListener>( m_paramValueListener ) )
                {
                    paramListener->addBinding( picker, p.m_name, p.m_type );
                }
            }
            else if( p.m_type == render::MaterialShaderParameterType::Vector2 )
            {
                auto vec = ui->addElementByType<ui::IUIVector2>();
                vec->setLabel( p.m_name );
                auto v = material->getShaderParameters().GetVector2f( handle );
                vec->setValue( Vector2<real_Num>( static_cast<real_Num>( v.x ), static_cast<real_Num>( v.y ) ) );
                if( m_paramValueListener ) vec->addObjectListener( m_paramValueListener );
                if( m_shaderParamContainer ) m_shaderParamContainer->addChild( vec );
                if( auto paramListener = workphone::dynamic_pointer_cast<ParamValueListener>( m_paramValueListener ) )
                {
                    paramListener->addBinding( vec, p.m_name, p.m_type );
                }
            }
            else if( p.m_type == render::MaterialShaderParameterType::Vector4 )
            {
                auto vec = ui->addElementByType<ui::IUIVector4>();
                vec->setLabel( p.m_name );
                auto v = material->getShaderParameters().GetVector4f( handle );
                vec->setValue( Vector4<real_Num>( static_cast<real_Num>( v.x ), static_cast<real_Num>( v.y ), static_cast<real_Num>( v.z ), static_cast<real_Num>( v.w ) ) );
                if( m_paramValueListener ) vec->addObjectListener( m_paramValueListener );
                if( m_shaderParamContainer ) m_shaderParamContainer->addChild( vec );
                if( auto paramListener = workphone::dynamic_pointer_cast<ParamValueListener>( m_paramValueListener ) )
                {
                    paramListener->addBinding( vec, p.m_name, p.m_type );
                }
            }
            else if( p.m_type == render::MaterialShaderParameterType::Texture )
            {
                auto pathInput = ui->addElementByType<ui::IUILabelTextInputPair>();
                pathInput->setLabel( p.m_name );
                pathInput->setValue( material->getShaderTexture( p.m_name ) );
                if( m_paramValueListener ) pathInput->addObjectListener( m_paramValueListener );
                if( m_shaderParamContainer ) m_shaderParamContainer->addChild( pathInput );
                if( auto paramListener = workphone::dynamic_pointer_cast<ParamValueListener>( m_paramValueListener ) )
                {
                    paramListener->addBinding( pathInput, p.m_name, p.m_type );
                }
            }
            else
            {
                // Matrices / buffers / samplers: read-only label for now (step 4b-ext).
                auto label = ui->addElementByType<ui::IUIText>();
                label->setText( p.m_name + String( "  (matrix/buffer/sampler)" ) );
                if( m_shaderParamContainer ) m_shaderParamContainer->addChild( label );
            }
        }
    }
}  // namespace workphone::editor
