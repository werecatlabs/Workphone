#ifndef _CDynamicLines_H_
#define _CDynamicLines_H_

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Graphics/DynamicLines.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsObjectOgreNext.hpp>

namespace workphone
{
    namespace render
    {
        class CDynamicLines : public CGraphicsObjectOgreNext<DynamicLines>
        {
        public:
            CDynamicLines( SmartPtr<IGraphicsScene> creator );
            ~CDynamicLines() override;

            void initialise();

            void update() override;

            //
            // IGraphicsObject functions
            //
            void setRenderQueueGroup( u8 renderQueue );

            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

            void _getObject( void **ppObject ) const override;

            //
            // IDynamicLines functions
            //
            void addPoint( const Vector3F &point ) override;
            void setPoint( u32 index, const Vector3F &point ) override;
            Vector3F getPoint( u32 index ) const override;
            u32 getNumPoints() const override;
            void clear() override;
            void setDirty() override;

            void setOperationType( u32 opType ) override;
            u32 getOperationType() const override;

            Ogre::ObjectMemoryManager *getMemoryManager() const;
            void setMemoryManager( Ogre::ObjectMemoryManager *memoryManager );

        protected:
            Ogre::ObjectMemoryManager *m_memoryManager = nullptr;
            DynamicLinesOgreNext *m_dynamicLines = nullptr;

            SmartPtr<IGraphicsScene> m_creator;
        };
    }  // end namespace render
}  // namespace workphone

#endif
