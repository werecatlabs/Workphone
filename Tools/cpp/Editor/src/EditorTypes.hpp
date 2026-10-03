#ifndef GameEditorTypes_h__
#define GameEditorTypes_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace editor
    {
        enum class ObjectType
        {
            None,
            Actor,
            Button,
            Resource,
            Mesh,
            Material,
            MaterialNode,
            Particle,
            Sound,
            SharedObject,
            Terrain,
            Texture,
            FileUnknown,

            Count
        };

        class GameEditorTypes : public ISharedObject
        {
        public:
            static const hash_type SCENE_VIEW_ID_TERRAIN;
            static const hash_type SCENE_VIEW_ID_MESH;
            static const hash_type SCENE_VIEW_ID_PARTICLE;

            static const hash_type CAMERA_TYPE_VIEWPORT_0;
            static const hash_type CAMERA_TYPE_VIEWPORT_1;
            static const hash_type CAMERA_TYPE_VIEWPORT_2;
            static const hash_type CAMERA_TYPE_VIEWPORT_3;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // GameEditorTypes_h__
