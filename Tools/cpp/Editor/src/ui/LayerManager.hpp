#ifndef LayerManager_h__
#define LayerManager_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace editor
    {
        class LayerManager : public ISharedObject
        {
        public:
            LayerManager();
            ~LayerManager() override;

            Array<String> getLayers() const;
            void setLayers( const Array<String> &layers );

            String getDefaultLayer() const;
            String normaliseLayerName( const String &layer ) const;

            bool hasLayer( const String &layer ) const;
            bool addLayer( const String &layer );
            bool removeLayer( const String &layer );

            u32 getLayerIndex( const String &layer ) const;
            String getLayerByIndex( u32 index ) const;

            void refreshFromScene();

            WP_CLASS_REGISTER_DECL;

        protected:
            void addLayerRecursive( SmartPtr<scene::IGameActor> actor );
            void replaceLayerRecursive( SmartPtr<scene::IGameActor> actor, const String &oldLayer,
                                        const String &newLayer );

            Array<String> m_layers;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // LayerManager_h__
