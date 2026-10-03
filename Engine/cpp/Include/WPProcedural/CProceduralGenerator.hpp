//
// Created by fluff on 09/03/2020.
//

#ifndef FBENGINE_CPROCEDURALGENERATOR_H
#define FBENGINE_CPROCEDURALGENERATOR_H

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Memory/IData.hpp>
#include <Workphone/Interface/IBuildDirector.hpp>

namespace workphone
{
    namespace procedural
    {
        template <class T>
        class CProceduralGenerator : public T
        {
        public:
            CProceduralGenerator() = default;
            ~CProceduralGenerator() override = default;

            virtual String getFilePath() const
            {
                return m_filePath;
            }

            virtual void setFilePath( const String &filePath )
            {
                m_filePath = filePath;
            }

            void load( const String &filePath )
            {
            }

            void load( SmartPtr<IData> data )
            {
            }

            void unload( u32 flags )
            {
                m_input = nullptr;
                m_output = nullptr;
                m_parent = nullptr;
            }

            void generate()
            {
            }

            SmartPtr<IBuildDirector> getInput() const override
            {
                return m_input;
            }

            void setInput( SmartPtr<IBuildDirector> input ) override
            {
                m_input = input;
            }

            SmartPtr<IBuildDirector> getOutput() const override
            {
                return m_output;
            }

            void setOutput( SmartPtr<IBuildDirector> output ) override
            {
                m_output = output;
            }

            SmartPtr<IProceduralGenerator> getParent() const override
            {
                return m_parent;
            }

            void setParent( SmartPtr<IProceduralGenerator> parent ) override
            {
                m_parent = parent;
            }

            bool isFinished() const override
            {
                return true;
            }

            WP_CLASS_REGISTER_TEMPLATE_DECL( CProceduralGenerator, T );

        protected:
            SmartPtr<IBuildDirector> m_input;
            SmartPtr<IBuildDirector> m_output;
            SmartPtr<IProceduralGenerator> m_parent;
            String m_filePath;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, CProceduralGenerator, T, T );

    }  // namespace procedural
}  // namespace workphone

#endif  // FBENGINE_CPROCEDURALGENERATOR_H
