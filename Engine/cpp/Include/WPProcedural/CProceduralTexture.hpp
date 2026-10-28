#ifndef CProceduralTexture_h__
#define CProceduralTexture_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <WPProcedural/ProceduralTexture.hpp>
#include <Workphone/Interface/Procedural/IProceduralTexture.hpp>

namespace workphone
{
    namespace procedural
    {
        /// Admitted CPU authoring storage. Native texture upload is not provided by this owner.
        /// Use TextureBaker to sample a TextureRule, then import matching raw ColourF pixels.
        class WPProcedural_API CProceduralTexture : public IProceduralTexture, public SourceBudgetObject
        {
        public:
            using SourceBudgetObject::operator new;
            using SourceBudgetObject::operator delete;

            CProceduralTexture();
            ~CProceduralTexture() override;

            /// Reports unsupported generation: this storage owner has no configured sampling rule.
            void generate() override;
            void unload( SmartPtr<ISharedObject> data ) override;

            /// Resize CPU storage atomically. (0, 0) retires it; mixed/negative dimensions are invalid.
            void setSize( int width, int height ) override;
            int getWidth() const override;
            int getHeight() const override;

            /// Import exactly width*height pixels, preserving admitted spare capacity.
            void setData( const Array<ColourF> &data ) override;
            Array<ColourF> getData() const override;

        protected:
            int m_width = 0;
            int m_height = 0;
            Array<ColourF> m_data{ Allocator<ColourF>( sourceBudget ) };
        };
    }
}

#endif
