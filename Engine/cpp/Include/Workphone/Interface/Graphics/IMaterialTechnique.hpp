#ifndef IMaterialTechnique_h__
#define IMaterialTechnique_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IMaterialNode.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief An interface for a material technique.
         */
        class WPCore_API IMaterialTechnique : public IMaterialNode
        {
        public:
            /** Virtual destructor. */
            ~IMaterialTechnique() override;

            /**
             * @brief Gets the scheme hash value.
             * @return A 32-bit unsigned integer representing the scheme hash value.
             */
            virtual hash32 getScheme() const = 0;

            /**
             * @brief Sets the scheme hash value.
             * @param scheme A 32-bit unsigned integer representing the scheme hash value.
             */
            virtual void setScheme( hash32 scheme ) = 0;

            /**
             * @brief Gets the number of material passes in the technique.
             * @return A 32-bit unsigned integer representing the number of material passes.
             */
            virtual u32 getNumPasses() const = 0;

            /**
             * @brief Creates a new material pass.
             * @return A smart pointer to the new material pass.
             */
            virtual SmartPtr<IMaterialPass> createPass() = 0;

            /**
             * @brief Adds a material pass to the technique.
             * @param pass A smart pointer to the material pass to add.
             */
            virtual void addPass( SmartPtr<IMaterialPass> pass ) = 0;

            /**
             * @brief Removes a material pass from the technique.
             * @param pass A smart pointer to the material pass to remove.
             */
            virtual void removePass( SmartPtr<IMaterialPass> pass ) = 0;

            /**
             * @brief Removes all material passes from the technique.
             */
            virtual void removePasses() = 0;

            /**
             * @brief Gets an array of all material passes in the technique.
             * @return An array of smart pointers to the material passes.
             */
            virtual Array<SmartPtr<IMaterialPass>> getPasses() const = 0;

            /**
             * @brief Sets the material passes in the technique.
             * @param passes An array of smart pointers to the material passes.
             */
            virtual void setPasses( Array<SmartPtr<IMaterialPass>> passes ) = 0;

            /**
             * @brief Gets a material pass by index.
             * @param index A 32-bit unsigned integer representing the index of the material pass.
             * @return A smart pointer to the material pass at the specified index.
             */
            virtual SmartPtr<IMaterialPass> getPass( u32 index ) const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // IMaterialTechnique_h__
