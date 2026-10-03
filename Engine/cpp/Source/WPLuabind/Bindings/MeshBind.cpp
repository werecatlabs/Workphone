#include <WPLuabind/WPLuabindPCH.hpp>
#include <WPLuabind/Bindings/MeshBind.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <WPLuabind/ParamConverter.hpp>
#include <Workphone/Workphone.hpp>
#include <luabind/luabind.hpp>
#include <luabind/class.hpp>

namespace workphone
{
    static int getRenderOp( ISubMesh *s )
    {
        return static_cast<int>( s->getRenderOperationType() );
    }

    static void setRenderOp( ISubMesh *s, int type )
    {
        s->setRenderOperationType( static_cast<RenderOperationType>( type ) );
    }

    static int getIndexType( IIndexBuffer *buffer )
    {
        return static_cast<int>( buffer->getIndexType() );
    }

    static void setIndexType( IIndexBuffer *buffer, int type )
    {
        buffer->setIndexType( static_cast<IIndexBuffer::Type>( type ) );
    }

    static int getVertexElementType( IVertexElement *element )
    {
        return static_cast<int>( element->getType() );
    }

    static void setVertexElementType( IVertexElement *element, int type )
    {
        element->setType( static_cast<VertexElementType>( type ) );
    }

    static int getMaterialNaming( IMeshResource *resource )
    {
        return static_cast<int>( resource->getMaterialNaming() );
    }

    static void setMaterialNaming( IMeshResource *resource, int naming )
    {
        resource->setMaterialNaming( static_cast<IMeshResource::MaterialNaming>( naming ) );
    }

    static int getSkeletonBlendMode( ISkeleton *skeleton )
    {
        return static_cast<int>( skeleton->getBlendMode() );
    }

    static void setSkeletonBlendMode( ISkeleton *skeleton, int mode )
    {
        skeleton->setBlendMode( static_cast<SkeletonAnimationBlendMode>( mode ) );
    }

    static SmartPtr<IBone> createBoneChild( IBone *bone, u16 handle )
    {
        return bone->createChild( handle );
    }

    void bindMesh( lua_State *L )
    {
        using namespace luabind;

        module( L )[class_<ISubMesh, ISharedObject, SmartPtr<ISubMesh>>( "ISubMesh" )
                        .def( "setMaterialName", &ISubMesh::setMaterialName )
                        .def( "getMaterialName", &ISubMesh::getMaterialName )
                        .def( "setVertexBuffer", &ISubMesh::setVertexBuffer )
                        .def( "getVertexBuffer", &ISubMesh::getVertexBuffer )
                        .def( "setIndexBuffer", &ISubMesh::setIndexBuffer )
                        .def( "getIndexBuffer", &ISubMesh::getIndexBuffer )
                        .def( "updateAABB", &ISubMesh::updateAABB )
                        .def( "getAABB", &ISubMesh::getAABB )
                        .def( "setAABB", &ISubMesh::setAABB )
                        .def( "clone", &ISubMesh::clone )
                        .def( "getUseSharedVertices", &ISubMesh::getUseSharedVertices )
                        .def( "setUseSharedVertices", &ISubMesh::setUseSharedVertices )
                        .def( "compare", &ISubMesh::compare )
                        .def( "getRenderOperationType", &getRenderOp )
                        .def( "setRenderOperationType", &setRenderOp )
                        .def( "addBoneAssignment", &ISubMesh::addBoneAssignment )
                        .def( "removeBoneAssignment", &ISubMesh::removeBoneAssignment )
                        .def( "getBoneAssignments", &ISubMesh::getBoneAssignments )
                        .scope[def( "typeInfo", ISubMesh::typeInfo )]];

        module( L )[class_<IMesh, ISharedObject, SmartPtr<IMesh>>( "IMesh" )
                        .def( "addSubMesh", &IMesh::addSubMesh )
                        .def( "removeSubMesh", &IMesh::removeSubMesh )
                        .def( "removeAllSubMeshes", &IMesh::removeAllSubMeshes )
                        .def( "getSubMeshes", &IMesh::getSubMeshes )
                        .def( "getSubMesh", &IMesh::getSubMesh )
                        .def( "getNumSubMeshes", &IMesh::getNumSubMeshes )
                        .def( "updateAABB", &IMesh::updateAABB )
                        .def( "getAABB", &IMesh::getAABB )
                        .def( "setAABB", &IMesh::setAABB )
                        .def( "setBoundingSphereRadius", &IMesh::setBoundingSphereRadius )
                        .def( "getBoundingSphereRadius", &IMesh::getBoundingSphereRadius )
                        .def( "clone", &IMesh::clone )
                        .def( "getAnimationInterface", &IMesh::getAnimationInterface )
                        .def( "setAnimationInterface", &IMesh::setAnimationInterface )
                        .def( "getHasSharedVertexData", &IMesh::getHasSharedVertexData )
                        .def( "setHasSharedVertexData", &IMesh::setHasSharedVertexData )
                        .def( "getSharedVertexBuffer", &IMesh::getSharedVertexBuffer )
                        .def( "setSharedVertexBuffer", &IMesh::setSharedVertexBuffer )
                        .def( "hasSkeleton", &IMesh::hasSkeleton )
                        .def( "getSkeletonName", &IMesh::getSkeletonName )
                        .def( "getSkeleton", &IMesh::getSkeleton )
                        .def( "setSkeleton", &IMesh::setSkeleton )
                        .def( "addBoneAssignment", &IMesh::addBoneAssignment )
                        .def( "removeBoneAssignment", &IMesh::removeBoneAssignment )
                        .def( "getBoneAssignments", &IMesh::getBoneAssignments )
                        .def( "getNumLodLevels", &IMesh::getNumLodLevels )
                        .def( "isEdgeListBuilt", &IMesh::isEdgeListBuilt )
                        .def( "hasVertexAnimation", &IMesh::hasVertexAnimation )
                        .def( "getNumAnimations", &IMesh::getNumAnimations )
                        .def( "getAnimation", &IMesh::getAnimation )
                        .def( "getAnimationByName", &IMesh::getAnimationByName )
                        .def( "createAnimation", &IMesh::createAnimation )
                        .def( "removeAnimation", &IMesh::removeAnimation )
                        .def( "removeAllAnimations", &IMesh::removeAllAnimations )
                        .def( "createPose", &IMesh::createPose )
                        .def( "getNumPoses", &IMesh::getNumPoses )
                        .def( "getPose", &IMesh::getPose )
                        .def( "getPoseByName", &IMesh::getPoseByName )
                        .def( "addPose", &IMesh::addPose )
                        .def( "removePose", &IMesh::removePose )
                        .def( "removeAllPoses", &IMesh::removeAllPoses )
                        .def( "compare", &IMesh::compare )
                        .scope[def( "typeInfo", IMesh::typeInfo )]];

        module(
            L )[class_<ISkeleton, ISharedObject, SmartPtr<ISkeleton>>( "ISkeleton" )
                    .def( "createAnimation", &ISkeleton::createAnimation )
                    .def( "getAnimation", static_cast<SmartPtr<IAnimation> ( ISkeleton::* )(
                                              const String & ) const>( &ISkeleton::getAnimation ) )
                    .def( "hasAnimation", &ISkeleton::hasAnimation )
                    .def( "createBone",
                          static_cast<SmartPtr<IBone> ( ISkeleton::* )()>( &ISkeleton::createBone ) )
                    .def( "createBoneWithHandle", static_cast<SmartPtr<IBone> ( ISkeleton::* )( u32 )>(
                                                      &ISkeleton::createBone ) )
                    .def( "createBoneWithName", static_cast<SmartPtr<IBone> ( ISkeleton::* )(
                                                    const String & )>( &ISkeleton::createBone ) )
                    .def( "createBoneWithNameAndHandle",
                          static_cast<SmartPtr<IBone> ( ISkeleton::* )( const String &, u32 )>(
                              &ISkeleton::createBone ) )
                    .def( "getBone", &ISkeleton::getBone )
                    .def( "hasBone", &ISkeleton::hasBone )
                    .def( "getBlendMode", getSkeletonBlendMode )
                    .def( "setBlendMode", setSkeletonBlendMode )
                    .scope[def( "typeInfo", ISkeleton::typeInfo )]];

        module( L )[class_<IBone, ISharedObject, SmartPtr<IBone>>( "IBone" )
                        .def( "getPosition", &IBone::getPosition )
                        .def( "setPosition", &IBone::setPosition )
                        .def( "getOrientation", &IBone::getOrientation )
                        .def( "setOrientation", &IBone::setOrientation )
                        .def( "getBoneHandle", &IBone::getBoneHandle )
                        .def( "setBoneHandle", &IBone::setBoneHandle )
                        .def( "getParent", &IBone::getParent )
                        .def( "getChildren", &IBone::getChildren )
                        .def( "createChild", createBoneChild )
                        .def( "createChildWithTransform", &IBone::createChild )
                        .def( "setBindingPose", &IBone::setBindingPose )
                        .def( "reset", &IBone::reset )
                        .def( "setManuallyControlled", &IBone::setManuallyControlled )
                        .def( "isManuallyControlled", &IBone::isManuallyControlled )
                        .scope[def( "typeInfo", IBone::typeInfo )]];

        module( L )[class_<IIndexBuffer, ISharedObject, SmartPtr<IIndexBuffer>>( "IIndexBuffer" )
                        .def( "setIndexType", setIndexType )
                        .def( "getIndexType", getIndexType )
                        .def( "setNumIndices", &IIndexBuffer::setNumIndices )
                        .def( "getNumIndices", &IIndexBuffer::getNumIndices )
                        .def( "getIndexSize", &IIndexBuffer::getIndexSize )
                        .def( "setIndexSize", &IIndexBuffer::setIndexSize )
                        .def( "clone", &IIndexBuffer::clone )
                        .def( "compare", &IIndexBuffer::compare )
                        .scope[def( "typeInfo", IIndexBuffer::typeInfo )]];

        module( L )[class_<IVertexBoneAssignment, ISharedObject, SmartPtr<IVertexBoneAssignment>>(
                        "IVertexBoneAssignment" )
                        .def( "getVertexIndex", &IVertexBoneAssignment::getVertexIndex )
                        .def( "setVertexIndex", &IVertexBoneAssignment::setVertexIndex )
                        .def( "getBoneIndex", &IVertexBoneAssignment::getBoneIndex )
                        .def( "setBoneIndex", &IVertexBoneAssignment::setBoneIndex )
                        .def( "getWeight", &IVertexBoneAssignment::getWeight )
                        .def( "setWeight", &IVertexBoneAssignment::setWeight )
                        .scope[def( "typeInfo", IVertexBoneAssignment::typeInfo )]];

        module( L )[class_<IVertexBuffer, ISharedObject, SmartPtr<IVertexBuffer>>( "IVertexBuffer" )
                        .def( "setVertexDeclaration", &IVertexBuffer::setVertexDeclaration )
                        .def( "getVertexDeclaration", &IVertexBuffer::getVertexDeclaration )
                        .def( "setNumVertices", &IVertexBuffer::setNumVertices )
                        .def( "getNumVertices", &IVertexBuffer::getNumVertices )
                        .def( "clone", &IVertexBuffer::clone )
                        .def( "compare", &IVertexBuffer::compare )
                        .scope[def( "typeInfo", IVertexBuffer::typeInfo )]];

        module( L )[class_<IVertexDeclaration, ISharedObject, SmartPtr<IVertexDeclaration>>(
                        "IVertexDeclaration" )
                        .def( "getSize", &IVertexDeclaration::getSize )
                        .def( "findElementsBySource", &IVertexDeclaration::findElementsBySource )
                        .def( "clone", &IVertexDeclaration::clone )
                        .def( "compare", &IVertexDeclaration::compare )
                        .scope[def( "typeInfo", IVertexDeclaration::typeInfo )]];

        module( L )[class_<IVertexElement, ISharedObject, SmartPtr<IVertexElement>>( "IVertexElement" )
                        .def( "getSource", &IVertexElement::getSource )
                        .def( "setSource", &IVertexElement::setSource )
                        .def( "getSize", &IVertexElement::getSize )
                        .def( "setSize", &IVertexElement::setSize )
                        .def( "getOffset", &IVertexElement::getOffset )
                        .def( "setOffset", &IVertexElement::setOffset )
                        .def( "getSemantic", &IVertexElement::getSemantic )
                        .def( "setSemantic", &IVertexElement::setSemantic )
                        .def( "getType", getVertexElementType )
                        .def( "setType", setVertexElementType )
                        .def( "getIndex", &IVertexElement::getIndex )
                        .def( "setIndex", &IVertexElement::setIndex )
                        .def( "compare", &IVertexElement::compare )
                        .scope[def( "typeInfo", IVertexElement::typeInfo )]];

        module(
            L )[class_<IMeshLoader, ISharedObject, SmartPtr<IMeshLoader>>( "IMeshLoader" )
                    .def( "loadActor", static_cast<SmartPtr<scene::IGameActor> ( IMeshLoader::* )(
                                           SmartPtr<IMeshResource> )>( &IMeshLoader::loadActor ) )
                    .def( "loadActor", static_cast<SmartPtr<scene::IGameActor> ( IMeshLoader::* )(
                                           const String & )>( &IMeshLoader::loadActor ) )
                    .def( "loadMesh", static_cast<SmartPtr<IMesh> ( IMeshLoader::* )(
                                          SmartPtr<IMeshResource> )>( &IMeshLoader::loadMesh ) )
                    .def( "loadMesh", static_cast<SmartPtr<IMesh> ( IMeshLoader::* )( const String & )>(
                                          &IMeshLoader::loadMesh ) )
                    .def( "getUseSingleMesh", &IMeshLoader::getUseSingleMesh )
                    .def( "setUseSingleMesh", &IMeshLoader::setUseSingleMesh )
                    .def( "getOverwrite", &IMeshLoader::getOverwrite )
                    .def( "setOverwrite", &IMeshLoader::setOverwrite )
                    .scope[def( "typeInfo", IMeshLoader::typeInfo )]];

        module(
            L )[class_<IMeshPose, ISharedObject, SmartPtr<IMeshPose>>( "IMeshPose" )
                    .def( "getTarget", &IMeshPose::getTarget )
                    .def( "setTarget", &IMeshPose::setTarget )
                    .def( "getIncludesNormals", &IMeshPose::getIncludesNormals )
                    .def( "setIncludesNormals", &IMeshPose::setIncludesNormals )
                    .def( "addVertex", static_cast<void ( IMeshPose::* )( u32, const Vector3F & )>(
                                           &IMeshPose::addVertex ) )
                    .def( "addVertexWithNormal",
                          static_cast<void ( IMeshPose::* )( u32, const Vector3F &, const Vector3F & )>(
                              &IMeshPose::addVertex ) )
                    .def( "getNumVertexOffsets", &IMeshPose::getNumVertexOffsets )
                    .def( "clone", &IMeshPose::clone )
                    .scope[def( "typeInfo", IMeshPose::typeInfo )]];

        module( L )[class_<IMeshResource, IResource, SmartPtr<IMeshResource>>( "IMeshResource" )
                        .def( "getScale", &IMeshResource::getScale )
                        .def( "setScale", &IMeshResource::setScale )
                        .def( "getMaterialNaming", getMaterialNaming )
                        .def( "setMaterialNaming", setMaterialNaming )
                        .def( "getConstraints", &IMeshResource::getConstraints )
                        .def( "setConstraints", &IMeshResource::setConstraints )
                        .def( "getAnimation", &IMeshResource::getAnimation )
                        .def( "setAnimation", &IMeshResource::setAnimation )
                        .def( "getVisibility", &IMeshResource::getVisibility )
                        .def( "setVisibility", &IMeshResource::setVisibility )
                        .def( "getCameras", &IMeshResource::getCameras )
                        .def( "setCameras", &IMeshResource::setCameras )
                        .def( "getLights", &IMeshResource::getLights )
                        .def( "setLights", &IMeshResource::setLights )
                        .def( "getLightmapUVs", &IMeshResource::getLightmapUVs )
                        .def( "setLightmapUVs", &IMeshResource::setLightmapUVs )
                        .def( "getUseMeshInstancing", &IMeshResource::getUseMeshInstancing )
                        .def( "setUseMeshInstancing", &IMeshResource::setUseMeshInstancing )
                        .def( "getMesh", &IMeshResource::getMesh )
                        .def( "setMesh", &IMeshResource::setMesh )
                        .scope[def( "typeInfo", IMeshResource::typeInfo )]];
    }
} // namespace workphone
