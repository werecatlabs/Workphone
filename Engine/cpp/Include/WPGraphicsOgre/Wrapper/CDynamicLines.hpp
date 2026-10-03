#ifndef _CDynamicLines_H_
#define _CDynamicLines_H_

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IDynamicLines.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsObjectOgre.hpp>

namespace workphone
{
    namespace render
    {

        class CDynamicLines : public CGraphicsObjectOgre<IDynamicLines>
        {
        public:
            CDynamicLines() = default;
            CDynamicLines( SmartPtr<IGraphicsScene> creator );
            ~CDynamicLines() override;

            void initialise();

            void update() override;

            void setMaterialName( const String &materialName, s32 index = -1 ) override;

            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

            void _getObject( void **ppObject ) const override;

            void addPoint( const Vector3F &point ) override;
            void setPoint( u32 index, const Vector3F &point ) override;
            Vector3F getPoint( u32 index ) const override;
            u32 getNumPoints() const override;
            void clear() override;
            void setDirty() override;

            void setOperationType( u32 opType ) override;
            u32 getOperationType() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            DynamicLinesOgre *m_dynamicLines = nullptr;

            SmartPtr<IGraphicsScene> m_creator;
        };
    }  // end namespace render
}  // namespace workphone

#endif
