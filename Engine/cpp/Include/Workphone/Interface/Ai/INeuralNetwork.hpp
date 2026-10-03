#ifndef INeuralNetwork_h__
#define INeuralNetwork_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** Interface for a neural network.
     */
    class WPCore_API INeuralNetwork : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~INeuralNetwork() override;

        /** Train the neural network with the given data. */
        virtual void train( const std::vector<float> &inputs, const std::vector<float> &targets ) = 0;

        /** Predict outputs for the given inputs. */
        virtual std::vector<float> predict( const std::vector<float> &inputs ) const = 0;

        /** Save the neural network to a file. */
        virtual bool save( const std::string &filename ) const = 0;

        /** Load the neural network from a file. */
        virtual bool load( const std::string &filename ) = 0;

        /** Set the input values for the network. */
        virtual void setInputs( const std::vector<float> &inputs ) = 0;

        /** Get the output values from the network. */
        virtual std::vector<float> getOutputs() const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // INeuralNetwork_h__
