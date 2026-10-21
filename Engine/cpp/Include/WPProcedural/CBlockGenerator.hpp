#ifndef CBlockGenerator_H
#define CBlockGenerator_H

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Procedural/IBlockGenerator.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <WPProcedural/CProceduralGenerator.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace procedural
    {

        class WPProcedural_API CBlockGenerator : public CProceduralGenerator<IBlockGenerator>
        {
        public:
            struct BlockInfo
            {
                BlockInfo( bool isClosedBlock = false );

                bool IsClosedBlock;
            };

            CBlockGenerator();
            ~CBlockGenerator() override;

            void load( SmartPtr<ISharedObject> object ) override;
            void unload( SmartPtr<ISharedObject> object ) override;

            /// Generates simple road-centerline polygons in XZ, preserving node elevations.
            /// Input must have explicit junctions at crossings; malformed embeddings throw.
            /// Replaces this generator's previous blocks; other city blocks are preserved.
            void generate() override;

            SmartPtr<IProceduralCity> getCity() const override;
            void setCity( SmartPtr<IProceduralCity> city ) override;

            void generateBlocks( SmartPtr<scene::IGameActor> cityLayer );

            /// Exercises temporary block allocations without modifying the city.
            void testMemoryUsage();

            WP_CLASS_REGISTER_DECL;

        private:
            Array<SmartPtr<ICityBlock>> m_generatedBlocks;

            SmartPtr<IProceduralCity> m_city;
        };
    }  // namespace procedural
}  // namespace workphone

#endif
