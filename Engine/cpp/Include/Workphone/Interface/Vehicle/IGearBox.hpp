#ifndef IGearBox_h__
#define IGearBox_h__

#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace vehicle
    {
        /**
         * Interface for a vehicle gear box.
         */
        class WPCore_API IGearBox : public IVehicleComponent
        {
        public:
            /** Virtual destructor. */
            ~IGearBox() override;

            /**
             * Sets the gear ratios.
             *
             * @param ratios An array containing the gear ratios.
             */
            virtual void setRatios( const Array<f32> &ratios ) = 0;

            /**
             * Gets the gear ratios.
             *
             * @return An array containing the gear ratios.
             */
            virtual Array<f32> getRatios() const = 0;

            /**
             * Returns the ratio of a gear in the gear box.
             *
             * @param gear The index of the gear.
             * @return The ratio of the specified gear.
             */
            virtual f32 getRatio( u32 gear ) const = 0;

            /**
             * Returns the number of gears in the gear box.
             *
             * @return The number of gears.
             */
            virtual u32 getNumGears() const = 0;

            /**
             * Decreases the currently selected gear to the next gear down.
             */
            virtual void decreamentSelectedGear() = 0;

            /**
             * Increases the currently selected gear to the next gear up.
             */
            virtual void increamentSelectedGear() = 0;

            /**
             * Returns the currently selected gear.
             *
             * @return The currently selected gear.
             */
            virtual u32 getCurrentGear() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IGearBox_h__
