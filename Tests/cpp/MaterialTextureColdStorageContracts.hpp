#ifndef WP_MATERIAL_TEXTURE_COLD_STORAGE_CONTRACTS_HPP
#define WP_MATERIAL_TEXTURE_COLD_STORAGE_CONTRACTS_HPP

#include <Workphone/Animation/Animator.hpp>
#include <Workphone/Graphics/MaterialTexture.hpp>
#include <WPGraphics/ClawMaterialTexture.hpp>
#include <WPGraphics/ClawMaterialPass.hpp>
#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <Workphone/State/States/MaterialPassStateData.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/Handle.hpp>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <iostream>

namespace material_texture_cold_contracts
{
    inline void check( bool result, const char *message )
    {
        if( !result ) throw std::runtime_error( message );
    }

    inline void run()
    {
        using namespace workphone;
        using namespace workphone::render;
        class Probe : public ClawMaterialTexture
        {
        public:
            const void *storage() const { return m_textureAuthoring.get(); }
            SmartPtr<ITexture> detachedTexture() const { return m_texture.load(); }
        };
        auto unit = make_ptr<Probe>();
        const auto defaultBytes = unit->getTextureAuthoringStorageBytes();
        check( unit->getTextureAuthoringStorageBytes() == 0 &&
               unit->getScale() == Vector3<real_Num>::unit() && unit->getTint() == ColourF::White &&
               !unit->getAnimator() && unit->getTextureType() == 0,
               "texture unit metadata reads shared defaults without allocation" );
        unit->setScale( Vector3<real_Num>::unit() );
        unit->setTint( ColourF::White );
        unit->setAnimator( nullptr );
        unit->setTextureType( 0 );
        auto defaults = unit->getProperties();
        unit->setProperties( defaults );
        check( !unit->storage(), "default unit setters and property round-trip retain zero cold bytes" );
        unit->setTextureType( 23 );
        check( !unit->storage() && unit->getTextureType() == 23,
               "ordinary nonzero texture slot ordinals require no optional metadata" );
        unit->setTextureType( 0 );
        auto nearWhite = ColourF::White;
        nearWhite.r = std::nextafter( nearWhite.r, 0.0f );
        unit->setTint( nearWhite );
        check( unit->storage() && unit->getTint().r == nearWhite.r,
               "cold texture metadata retains exact nondefault values below approximate colour equality" );
        unit->setTint( ColourF::White );
        check( !unit->storage(), "exact default reset releases a near-default colour override" );

        const Vector3<real_Num> scale( real_Num( 2 ), real_Num( 3 ), real_Num( 4 ) );
        const ColourF tint( 0.25f, 0.5f, 0.75f, 0.625f );
        unit->setScale( scale );
        const auto storage = unit->storage();
        const auto bytes = unit->getTextureAuthoringStorageBytes();
        unit->setScale( scale );
        unit->MaterialTexture::setTint( tint );
        unit->setTextureType( 7 );
        auto animator = make_ptr<Animator>();
        unit->setAnimator( animator );
        check( storage && unit->storage() == storage && bytes == unit->getTextureAuthoringStorageBytes() &&
               unit->getScale() == scale && unit->getTint() == tint &&
               unit->MaterialTexture::getAnimator() == animator &&
               unit->MaterialTexture::getTextureType() == 7,
               "Claw and base APIs share one stable optional metadata owner" );

        auto partial = make_ptr<Properties>();
        partial->setProperty( IMaterialTexture::textureTypeStr, u32( 9 ) );
        unit->setProperties( partial );
        check( unit->getScale() == scale && unit->getTint() == tint && unit->getAnimator() == animator &&
               unit->getTextureType() == 9 && unit->storage() == storage,
               "partial texture-unit edits retain all unspecified metadata" );
        auto restored = make_ptr<ClawMaterialTexture>();
        restored->fromData( unit->toData() );
        check( restored->getScale() == scale && restored->getTint() == tint &&
               restored->getTextureType() == 9 && !restored->getAnimator(),
               "saved texture-unit scale tint and type round-trip without serializing animator ownership" );

        auto texture = make_ptr<ClawTexture>();
        unit->setTexture( texture );
        auto basePartial = make_ptr<Properties>();
        basePartial->setProperty( IMaterialTexture::tintStr, ColourF::White );
        unit->MaterialTexture::setProperties( basePartial );
        check( unit->getTexture() == texture && unit->getScale() == scale &&
               unit->getTint() == ColourF::White && unit->getTextureType() == 9 &&
               unit->getAnimator() == animator,
               "base partial property import retains binding and unspecified cold metadata" );
        unit->setTexture( nullptr );
        unit->setLoadingState( LoadingState::Loaded );
        unit->unload( nullptr );
        check( !unit->getAnimator() && unit->getScale() == scale && unit->getTextureType() == 9,
               "unload releases animator but retains authored unit metadata" );
        unit->setScale( Vector3<real_Num>::unit() );
        check( !unit->storage() && unit->getTextureType() == 9,
               "resetting final cold metadata frees storage while preserving inline slot type" );
        unit->MaterialTexture::setTextureType( 0 );
        check( !unit->storage() && unit->getTextureAuthoringStorageBytes() == 0,
               "clearing inline slot type retains zero optional storage" );
        unit->setAnimator( animator );
        unit->unload( nullptr );
        check( !unit->storage() && !unit->getAnimator(),
               "unloading animator-only metadata releases its entire optional block" );
        std::cout << "texture_cold_storage base_unit_bytes=" << sizeof( MaterialTexture )
                  << " claw_unit_bytes=" << sizeof( ClawMaterialTexture )
                  << " default_authoring_bytes=" << defaultBytes
                  << " allocated_authoring_bytes=" << bytes << '\n';

        const auto assignBinding = []( const SmartPtr<ClawMaterialPass> &pass,
                                       SmartPtr<ITexture> texture ) {
            auto context = pass->getStateContext();
            check( bool( context ), "attached texture test has a pass context" );
            auto state = context->invalidateStateDataById<MaterialPassStateData>( pass->getId() );
            check( bool( state ), "attached texture test has canonical pass state" );
            state->setTextureBinding( 0, texture );
        };
        auto textureA = make_ptr<ClawTexture>();
        auto textureB = make_ptr<ClawTexture>();
        textureA->setFilePath( "cold-owner-a.png" );
        textureB->setFilePath( "cold-owner-b.png" );
        textureA->getHandle()->setUUID( String( "159b94aa-0658-4a3c-8ae5-118b35071d0a" ) );
        textureB->getHandle()->setUUID( String( "159b94aa-0658-4a3c-8ae5-118b35071d0b" ) );
        textureA->setLoadingState( LoadingState::Loaded );
        textureB->setLoadingState( LoadingState::Loaded );
        auto attached = make_ptr<Probe>();
        attached->setTexture( textureA );
        check( attached->detachedTexture() == textureA,
               "detached unit owns its resource before canonical state exists" );
        auto firstPass = make_ptr<ClawMaterialPass>();
        firstPass->addTextureUnit( attached );
        check( attached->getTexture() == textureA && !attached->detachedTexture(),
               "attached unit retires its local resolved resource mirror" );
        assignBinding( firstPass, textureB );
        check( attached->getTexture() == textureB && attached->getTextureName() == "cold-owner-b.png" &&
               !attached->detachedTexture(), "direct canonical binding edits immediately reach materialized facades" );
        auto serialized = workphone::static_pointer_cast<Properties>( attached->toData() );
        check( serialized->getProperty( MaterialTexture::textureStr ) == textureB->getHandle()->getUUIDAsString(),
               "materialized facade serialization uses its canonical resource UUID" );
        auto baseSerialized = workphone::static_pointer_cast<Properties>( attached->MaterialTexture::toData() );
        check( baseSerialized->getProperty( "texturePath" ) == "cold-owner-b.png",
               "base texture serialization uses the canonical resource path fallback" );
        auto passData = workphone::static_pointer_cast<Properties>( firstPass->toData() );
        auto savedUnits = passData->getChildrenByName( "textures" );
        check( savedUnits.size() == 1 &&
               savedUnits[0]->getProperty( MaterialTexture::textureStr ) == textureB->getHandle()->getUUIDAsString(),
               "pass serialization cannot replay a stale facade binding" );
        const auto children = attached->getChildObjects();
        check( std::any_of( children.begin(), children.end(), [&]( const auto &child ) {
                   return child.get() == textureB.get();
               } ), "unit child ownership reports canonical resource identity" );

        attached->setTextureName( "m4-unresolved-owner-7b4633.png" );
        check( !attached->getTexture() && attached->getUnresolvedTextureNameObjectBytes() != 0,
               "attached unresolved names retain optional authoring storage" );
        {
            auto state = firstPass->getStateContext()->invalidateStateDataById<MaterialPassStateData>( firstPass->getId() );
            state->setSurfaceValue( &MaterialRenderState::roughness, 0.81f );
        }
        check( attached->getTextureName() == "m4-unresolved-owner-7b4633.png",
               "unrelated canonical scalar edits preserve an unresolved authoring name" );
        auto emptyBinding = make_ptr<Properties>();
        emptyBinding->setProperty( IMaterialTexture::texturePathStr, SmartPtr<ITexture>() );
        attached->MaterialTexture::setProperties( emptyBinding );
        check( attached->getTextureName().empty() && !attached->getUnresolvedTextureNameObjectBytes(),
               "explicit empty base texture property clears a pending name even without a bound resource" );
        attached->setTextureName( "m4-unresolved-owner-7b4633.png" );
        assignBinding( firstPass, textureB );
        check( !attached->getUnresolvedTextureNameObjectBytes(),
               "effective canonical binding changes retire obsolete unresolved names" );
        assignBinding( firstPass, nullptr );
        check( !attached->getTexture() && attached->getTextureName().empty(),
               "clearing a direct canonical binding cannot resurrect an obsolete name" );
        assignBinding( firstPass, textureB );
        firstPass->removeTextureUnit( attached );
        check( !attached->getParent() && attached->getTexture() == textureB &&
               attached->detachedTexture() == textureB,
               "unit removal transfers its latest canonical binding to detached ownership" );
        auto secondPass = make_ptr<ClawMaterialPass>();
        secondPass->addTextureUnit( attached );
        check( attached->getTexture() == textureB && !attached->detachedTexture(),
               "reattaching a unit seeds its resource and retires detached ownership" );
        attached->setParent( nullptr );
        check( attached->getTexture() == textureB && attached->detachedTexture() == textureB,
               "explicit parent detach captures the latest canonical resource" );
        secondPass->removeTextureUnit( attached );
        secondPass->addTextureUnit( attached );
        attached->setLoadingState( LoadingState::Loaded );
        secondPass->unload( nullptr );
        check( !attached->getParent() && !attached->getTexture() &&
               attached->getTextureName() == "cold-owner-b.png",
               "pass unload captures canonical resource path before context and binding retirement" );

        // Adding directly to another pass transfers ownership without requiring
        // callers to remove the old membership first.
        auto oldMaterial = make_ptr<ClawMaterial>();
        oldMaterial->load( nullptr );
        auto oldPass = workphone::dynamic_pointer_cast<MaterialPass>( oldMaterial->getTechnique( 0 )->getPass( 0 ) );
        check( bool( oldPass ), "cross-pass transfer fixture has a concrete pass" );
        oldPass->load( nullptr );
        check( oldPass->getNumTexturesNodes() > 1,
               "cross-pass transfer fixture begins with standard texture slots" );
        auto moved = workphone::dynamic_pointer_cast<MaterialTexture>( oldPass->getTextureUnit( 1 ) );
        check( bool( moved ), "cross-pass fixture materializes the transferred facade" );
        moved->setTexture( textureB );
        auto newPass = make_ptr<ClawMaterialPass>();
        newPass->addTextureUnit( moved );
        check( moved->getParent().get() == newPass.get() && moved->getTexture() == textureB,
               "cross-pass add retains the previous owner's canonical binding" );
        {
            auto state = oldPass->getStateContext()->invalidateStateDataById<MaterialPassStateData>( oldPass->getId() );
            state->setSurfaceValue( &MaterialRenderState::materialType, MaterialType::UI );
        }
        oldPass->load( nullptr );
        check( oldPass->getNumTexturesNodes() == 1 && moved->getParent().get() == newPass.get() &&
               moved->getTexture() == textureB,
               "old pass shrink and reload cannot reclaim a transferred facade" );
        oldPass->unload( nullptr );
        check( moved->getParent().get() == newPass.get() && moved->getTexture() == textureB,
               "old pass unload cannot clear the new pass's transferred binding" );

        // Parent association without slot membership remains a detached/local
        // binding and cannot be reclaimed by the former array owner.
        auto formerPass = make_ptr<ClawMaterialPass>();
        auto foreignParent = make_ptr<ClawMaterialPass>();
        auto foreign = make_ptr<Probe>();
        foreign->setTexture( textureB );
        foreign->setLoadingState( LoadingState::Loaded );
        formerPass->addTextureUnit( foreign );
        foreign->setParent( foreignParent );
        SmartPtr<ITexture> foreignBinding;
        check( !foreignParent->getRenderTextureBinding( foreign.get(), foreignBinding ) &&
               foreign->getTexture() == textureB && foreign->detachedTexture() == textureB,
               "parent association outside verified slot membership retains local ownership" );
        formerPass->load( nullptr );
        check( foreign->getParent().get() == foreignParent.get() && foreign->getTexture() == textureB,
               "former pass reload cannot steal a facade associated with another parent" );
        formerPass->unload( nullptr );
        check( foreign->getParent().get() == foreignParent.get() && foreign->getTexture() == textureB,
               "former pass unload cannot detach or clear a foreign facade" );
        foreign->setParent( nullptr );
        foreign->unload( nullptr );
    }
}
#endif
