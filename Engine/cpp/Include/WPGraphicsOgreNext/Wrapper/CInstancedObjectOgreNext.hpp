#ifndef CInstancedObjectOgreNext_h__
#define CInstancedObjectOgreNext_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsObjectOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CInstanceManagerOgreNext.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>
#include <Workphone/Graphics/GraphicsObject.hpp>
#include <Workphone/Interface/Graphics/IInstancedObject.hpp>

namespace Ogre
{
    class Item;
    class SceneNode;
}  // namespace Ogre

namespace workphone::render
{
    class CInstancedObjectOgreNext : public CGraphicsObjectOgreNext<GraphicsObject<IInstancedObject>>
    {
    public:
        using Base = CGraphicsObjectOgreNext<GraphicsObject<IInstancedObject>>;

        CInstancedObjectOgreNext( SmartPtr<IGraphicsScene> creator,
                                  SmartPtr<CInstanceManagerOgreNext> manager,
                                  const String &materialName, const String &managerName );

        ~CInstancedObjectOgreNext() override;

        void load( SmartPtr<ISharedObject> data ) override;

        void unload( SmartPtr<ISharedObject> data ) override;

        void setPosition( const Vector3F &position ) override;

        Vector3F getPosition() const override;

        void setOrientation( const QuaternionF &orientation ) override;

        QuaternionF getOrientation() const override;

        void setScale( const Vector3F &scale ) override;

        Vector3F getScale() const override;

        void setCustomParam( u8 idx, const Vector4F &newParam ) override;

        Vector4F getCustomParam( u8 idx ) override;

        void attachToParent( SmartPtr<IGraphicsSceneNode> parent ) override;

        void detachFromParent( SmartPtr<IGraphicsSceneNode> parent ) override;

        void setVisible( bool visible ) override;

        void setCastShadows( bool castShadows ) override;

        void setVisibilityFlags( u32 flags ) override;

        void setRenderQueueGroup( u32 queueID ) override;

        SmartPtr<IGraphicsObject> clone( const String &name = StringUtil::EmptyString ) const override;

        void _getObject( void **ppObject ) const override;

        SmartPtr<Properties> getProperties() const override;

        void setProperties( SmartPtr<Properties> properties ) override;

    protected:
        void setupStateObject() override;

    private:
        static constexpr size_t MaxCustomParams = 8;

        void applyMaterial();

        void applyRenderState();

        void applyCustomParams();

        void updateInstanceNodeTransform();

        AtomicSmartPtr<CInstanceManagerOgreNext> m_manager;
        AtomicValue<Ogre::Item *> m_item = nullptr;
        Ogre::SceneNode *m_instanceNode = nullptr;
        String m_materialName;
        String m_managerName;
        Vector3F m_position = Vector3F::ZERO;
        QuaternionF m_orientation = QuaternionF();
        Vector3F m_scale = Vector3F::UNIT;
        Vector4F m_customParams[MaxCustomParams] = {};
        bool m_customParamSet[MaxCustomParams] = {};
    };
}  // namespace workphone::render

#endif  // CInstancedObjectOgreNext_h__
