#ifndef WP_MATERIAL_SURFACE_HANDOFF_CONTRACTS_HPP
#define WP_MATERIAL_SURFACE_HANDOFF_CONTRACTS_HPP

// Included inside MaterialLoadSequenceTests' fixture namespace.
void runMaterialSurfaceHandoff( Fixture &fixture )
{
    using namespace render;
    MaterialShader full( "SurfaceHandoffPBR" );
    full.addParameter( MaterialShaderParameterType::Scalar, "Metalness" )
        .addParameter( MaterialShaderParameterType::Scalar, "Roughness" )
        .addParameter( MaterialShaderParameterType::Scalar, "Opacity" )
        .addParameter( MaterialShaderParameterType::Colour, "Albedo" );
    MaterialShader absent( "SurfaceHandoffAbsent" );
    absent.addParameter( MaterialShaderParameterType::Int, "Custom" );
    MaterialShader wrong( "SurfaceHandoffWrongTypes" );
    wrong.addParameter( MaterialShaderParameterType::Int, "Metalness" )
        .addParameter( MaterialShaderParameterType::Int, "Roughness" )
        .addParameter( MaterialShaderParameterType::Int, "Opacity" );
    const auto author = []( SmartPtr<ClawMaterial> material ) {
        material->batch( []( Material &value ) {
            value.setMetalness( 0.31f ); value.setRoughness( 0.67f ); value.setOpacity( 0.42f );
        } );
    };
    u32 authoredChecks = 0;
    const auto expectAuthored = [&]( const SmartPtr<IMaterialPass> &pass ) {
        auto data = pass->getStateContext()->getStateDataById<MaterialPassStateData>( pass->getId() );
        ++authoredChecks;
        const bool matches = pass->getMetalness() == 0.31f && pass->getRoughness() == 0.67f &&
                             data->getSurfaceState().opacity == 0.42f && pass->getDiffuse().a == 0.42f;
        if( !matches ) std::cerr << "surface_handoff_check=" << authoredChecks
            << " metalness=" << pass->getMetalness() << " roughness=" << pass->getRoughness()
            << " opacity=" << data->getSurfaceState().opacity << " diffuse_alpha=" << pass->getDiffuse().a << '\n';
        require( matches,
                 "new passes inherit the old shader's authored scalar values after ownership handoff" );
    };
    for( u32 mode = 0; mode < 4; ++mode )
    {
        auto material = make_ptr<ClawMaterial>();
        material->setShaderParameters( full.getParameterInfo(), full.getShaderID() );
        material->load( nullptr ); author( material );
        require( material->getLegacySurfaceStorageBytes() == 0, "declared scalars have no fallback mirror" );
        auto primary = material->getTechnique( 0 )->getPass( 0 );
        primary->setMetalness( 0.91f ); primary->setRoughness( 0.12f );
        primary->setDiffuse( ColourF( 0.8f, 0.6f, 0.4f, 0.22f ) );
        const auto local = material->getRenderState();
        if( mode == 0 ) material->setShaderParameters( absent.getParameterInfo(), absent.getShaderID() );
        if( mode == 1 ) material->setShaderParameters( wrong.getParameterInfo(), wrong.getShaderID() );
        if( mode == 2 ) material->setShaderID( String() );
        if( mode == 3 ) material->setShaderID( Material::unlitShaderIDStr );
        require( material->getLegacySurfaceStorageBytes() > 0 && material->getRenderState() == local,
                 "schema removal, wrong types, clear and inactive workflows hand off without replaying primary overrides" );

        // A detached, already initialized pass exercises the addTechnique path,
        // whose retained fallback replay must not reset existing local passes.
        auto technique = make_ptr<ClawMaterialTechnique>();
        auto added = make_ptr<ClawMaterialPass>();
        technique->addPass( added ); added->load( nullptr );
        material->addTechnique( technique ); added->load( nullptr );
        expectAuthored( added );
        require( material->getRenderState() == local && primary->getMetalness() == 0.91f &&
                 primary->getRoughness() == 0.12f && primary->getDiffuse().a == 0.22f,
                 "attaching a fallback-seeded graph preserves existing pass-local values and snapshot identity" );
        if( mode == 1 )
        {
            const auto &parameters = material->getShaderParameters();
            for( const auto semantic : { MaterialBuiltinParameter::Metalness, MaterialBuiltinParameter::Roughness,
                                        MaterialBuiltinParameter::Opacity } )
                require( parameters.GetInt( parameters.GetBuiltin( semantic ) ) == 0,
                         "surface handoff leaves wrong-type custom values untouched" );
        }
        material->unload( nullptr ); material->load( nullptr );
        expectAuthored( material->getTechnique( 0 )->getPass( 0 ) );
        material->unload( nullptr );
    }

    auto pending = make_ptr<ClawMaterial>();
    pending->setShaderParameters( full.getParameterInfo(), full.getShaderID() );
    {
        auto edit = pending->getShaderParameters().Edit();
        author( pending );
        pending->setShaderParameters( wrong.getParameterInfo(), wrong.getShaderID() );
    }
    pending->load( nullptr ); expectAuthored( pending->getTechnique( 0 )->getPass( 0 ) );
    pending->unload( nullptr );

    MaterialShader albedoOnly( "SurfaceHandoffAlbedoOnly" );
    albedoOnly.addParameter( MaterialShaderParameterType::Colour, "Albedo" );
    for( u32 mode = 0; mode < 3; ++mode )
    {
        auto material = make_ptr<ClawMaterial>();
        material->setShaderParameters( albedoOnly.getParameterInfo(), albedoOnly.getShaderID() );
        material->setOpacity( 0.37f );
        if( mode == 0 ) material->setShaderID( String() );
        if( mode == 1 ) material->setShaderParameters( wrong.getParameterInfo(), wrong.getShaderID() );
        if( mode == 2 ) material->setShaderParameters( absent.getParameterInfo(), absent.getShaderID() );
        material->load( nullptr );
        require( material->getOpacity() == 0.37f && material->getDiffuse().a == 0.37f,
                 "Albedo-owned alpha hands off before its shader or compatible colour slot disappears" );
        material->unload( nullptr );
    }

    auto represented = make_ptr<ClawMaterial>();
    represented->setShaderParameters( full.getParameterInfo(), full.getShaderID() );
    represented->setOpacity( 0.46f );
    represented->setShaderParameters( albedoOnly.getParameterInfo(), albedoOnly.getShaderID() );
    require( represented->getShaderParameters().GetColour( represented->getShaderParameters().GetBuiltin(
                 MaterialBuiltinParameter::Albedo ) ).a == 0.46f,
             "removing scalar Opacity transfers its current value into the remaining Albedo owner" );
    represented->setOpacity( 0.58f );
    represented->setShaderParameters( full.getParameterInfo(), full.getShaderID() );
    require( represented->getShaderParameters().GetScalar( represented->getShaderParameters().GetBuiltin(
                 MaterialBuiltinParameter::Opacity ) ) == 0.58f,
             "adding scalar Opacity transfers the current Albedo alpha into its new owner" );
    represented->load( nullptr );
    require( represented->getOpacity() == 0.58f, "the transferred opacity representation seeds graph creation" );
    represented->unload( nullptr );

    for( u32 mode = 0; mode < 4; ++mode )
    {
        auto material = make_ptr<ClawMaterial>();
        material->setShaderParameters( albedoOnly.getParameterInfo(), albedoOnly.getShaderID() );
        {
            auto edit = material->getShaderParameters().Edit();
            material->setOpacity( 0.37f );
            if( mode == 0 ) material->setShaderParameters( absent.getParameterInfo(), absent.getShaderID() );
            if( mode == 1 ) material->setShaderParameters( wrong.getParameterInfo(), wrong.getShaderID() );
            if( mode == 2 ) material->setShaderID( String() );
            if( mode == 3 ) material->setShaderParameters( full.getParameterInfo(), full.getShaderID() );
        }
        material->load( nullptr );
        require( material->getOpacity() == 0.37f && material->getDiffuse().a == 0.37f,
                 "schema/workflow replacement retires obsolete alpha-only masks and preserves represented queued opacity" );
        material->unload( nullptr );
    }

    auto queued = make_ptr<ClawMaterial>();
    queued->setShaderParameters( full.getParameterInfo(), full.getShaderID() ); queued->load( nullptr );
    auto extended = full.getParameterInfo(); extended.emplace_back( MaterialShaderParameterType::Int, "Extra" );
    {
        auto edit = queued->getShaderParameters().Edit();
        queued->setMetalness( 0.47f );
        queued->setShaderParameters( extended, "SurfaceHandoffExtended" );
    }
    require( queued->getMetalness() == 0.47f && queued->getRenderState()->metalness == 0.47f,
             "compatible queued semantic edits still publish after schema replacement inside an open edit" );
    queued->unload( nullptr );

    for( u32 mode = 0; mode < 6; ++mode )
    {
        auto material = make_ptr<ClawMaterial>();
        material->setShaderParameters( full.getParameterInfo(), full.getShaderID() );
        material->load( nullptr ); author( material );
        auto primary = material->getTechnique( 0 )->getPass( 0 );
        primary->setMetalness( 0.91f ); primary->setRoughness( 0.12f );
        primary->setDiffuse( ColourF( 0.8f, 0.6f, 0.4f, 0.22f ) );
        const auto local = material->getRenderState();
        auto &parameters = material->getShaderParameters();
        MaterialShaderParametersInstance replacement;
        replacement.Reset( wrong.getParameterInfo() );
        replacement.SetScalar( replacement.GetBuiltin( MaterialBuiltinParameter::Metalness ), s32( 17 ) );
        if( mode == 0 ) parameters.Reset( absent.getParameterInfo() );
        if( mode == 1 ) parameters.Reset( replacement.GetLayout() );
        if( mode == 2 ) parameters.Clear();
        if( mode == 3 ) parameters = replacement;
        if( mode == 4 ) parameters = std::move( replacement );
        if( mode == 5 )
        {
            MaterialShaderParametersInstance moved( std::move( parameters ) );
            require( !parameters.IsValid() && moved.IsValid(), "moving from an owned instance retires only its values" );
            moved.SetScalar( moved.GetBuiltin( MaterialBuiltinParameter::Metalness ), 0.99f );
            require( material->getRenderState() == local, "a detached moved value does not steal the physical source observer" );
        }
        require( material->getLegacySurfaceStorageBytes() > 0 && material->getRenderState() == local,
                 "retained mutable Reset/Clear/copy/move adapters hand off scalars before losing their old schema" );
        if( mode == 3 || mode == 4 )
            require( parameters.GetInt( parameters.GetBuiltin( MaterialBuiltinParameter::Metalness ) ) == 17,
                     "assignment preserves the replacement's custom values" );
        auto added = material->createTechnique()->createPass(); added->load( nullptr ); expectAuthored( added );
        require( material->getRenderState() == local && primary->getMetalness() == 0.91f &&
                 primary->getRoughness() == 0.12f && primary->getDiffuse().a == 0.22f,
                 "mutable replacement notifications preserve unsupported semantics and independent primary overrides" );
        material->unload( nullptr );
    }

    for( u32 mode = 0; mode < 3; ++mode )
    {
        auto material = make_ptr<ClawMaterial>();
        material->setShaderParameters( full.getParameterInfo(), full.getShaderID() );
        {
            auto edit = material->getShaderParameters().Edit();
            author( material );
            if( mode == 0 ) material->getShaderParameters().Clear();
            if( mode == 1 ) material->getShaderParameters().Reset( wrong.getParameterInfo() );
            if( mode == 2 )
            {
                MaterialShaderParametersInstance replacement; replacement.Reset( wrong.getParameterInfo() );
                material->getShaderParameters() = replacement;
            }
        }
        material->load( nullptr ); expectAuthored( material->getTechnique( 0 )->getPass( 0 ) );
        material->unload( nullptr );
    }
    for( u32 mode = 0; mode < 2; ++mode )
    {
        auto material = make_ptr<ClawMaterial>();
        material->setShaderParameters( albedoOnly.getParameterInfo(), albedoOnly.getShaderID() );
        material->setOpacity( 0.37f );
        if( mode == 0 ) material->getShaderParameters().Clear();
        else material->getShaderParameters().Reset( wrong.getParameterInfo() );
        material->load( nullptr );
        require( material->getOpacity() == 0.37f && material->getDiffuse().a == 0.37f,
                 "mutable replacements retain pending Albedo-owned alpha without scalar Opacity" );
        material->unload( nullptr );
    }

    auto resetting = make_ptr<ClawMaterial>();
    resetting->setShaderParameters( full.getParameterInfo(), full.getShaderID() );
    resetting->load( nullptr ); author( resetting );
    auto &resetValues = resetting->getShaderParameters();
    const auto beforeReset = resetting->getRenderState();
    const auto beforeRevision = resetValues.GetRevision();
    const auto beforeStorage = resetValues.GetValueStorageIdentity();
    MaterialShaderParametersInstance identical( resetValues );
    resetValues = identical; resetValues = resetValues; resetValues = std::move( resetValues );
    auto invalid = full.getParameterInfo(); invalid.push_back( invalid.front() );
    resetValues.Reset( invalid ); resetValues.InitializeDefaultResourceHandles();
    require( resetValues.GetRevision() == beforeRevision && resetValues.GetValueStorageIdentity() == beforeStorage &&
             resetting->getRenderState() == beforeReset && resetting->getLegacySurfaceStorageBytes() == 0,
             "self/equal assignments, rejected schemas and resource initialization do not trigger scalar handoff" );
    resetValues.Reset( resetValues.GetLayout() );
    require( resetValues.GetScalar( resetValues.GetBuiltin( MaterialBuiltinParameter::Metalness ) ) == 0.0f &&
             resetting->getRenderState()->metalness == 0.0f && resetting->getLegacySurfaceStorageBytes() == 0,
             "a valid same-layout Reset intentionally selects fresh defaults without preserving a redundant mirror" );
    resetting->unload( nullptr );

    auto source = make_ptr<ClawMaterial>(), destination = make_ptr<ClawMaterial>();
    source->setShaderParameters( full.getParameterInfo(), full.getShaderID() );
    destination->setShaderParameters( full.getParameterInfo(), full.getShaderID() );
    source->load( nullptr ); destination->load( nullptr ); author( source ); author( destination );
    destination->setMetalness( 0.72f );
    auto &sourceValues = source->getShaderParameters();
    auto &destinationValues = destination->getShaderParameters();
    destinationValues = std::move( sourceValues );
    require( !sourceValues.IsValid() && destination->getMetalness() == 0.31f &&
             source->getLegacySurfaceStorageBytes() > 0 && destination->getLegacySurfaceStorageBytes() == 0,
             "moving between owned instances stages the source fallback while retaining destination assignment semantics" );
    destination->setMetalness( 0.83f );
    auto sourcePass = source->createTechnique()->createPass(); sourcePass->load( nullptr ); expectAuthored( sourcePass );
    require( source->getRenderState()->metalness == 0.31f && destination->getRenderState()->metalness == 0.83f,
             "moved instances retain their physical observers and independent future-pass values" );
    sourceValues.Reset( full.getParameterInfo() );
    require( source->getRenderState()->metalness == 0.0f && source->getLegacySurfaceStorageBytes() == 0 &&
             destination->getRenderState()->metalness == 0.83f,
             "rebinding a moved-from physical instance restores its observer and releases compatible fallbacks" );
    source->unload( nullptr ); destination->unload( nullptr );
    static_assert( std::is_nothrow_move_constructible_v<MaterialShaderParametersInstance> &&
                   std::is_nothrow_move_assignable_v<MaterialShaderParametersInstance>,
                   "owner handoff preserves noexcept parameter moves" );
    fixture.graphics->pending.clear();
    std::cout << "Surface handoff contracts: scalar/alpha owners, schema loss, mutable resets and assignments, pending edits and retained local passes passed.\n";
}

#endif
