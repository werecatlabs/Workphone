#ifndef _CMLP_
#define _CMLP_

#include <Workphone/Interface/Ai/INeuralNetwork.hpp>

namespace workphone
{

    /** QLearning class slightly modified implementation from John Manslow's
        QLearning class in the article "Using Reinforcement Learning to Solve AI Control Problems"
        which came with the "AI Programming Wisdom 2" book.
        Most comments have been left unchanged. Properties controlling learning are displayed
        in the console for debugging.
        Slightly modified for VC7.
        Included fstream, declared that std namespace is being used,
        and edited the load and save functions.
        The Multilayer Perceptron (MLP) neural network class.
        The neural network relates state-action pairs (a, s) to expected long-term discounted rewards,
        playing the role of the Q-function Q(a, s).
     */
    class WPCore_API MLP : public INeuralNetwork
    {
    public:
        /**
         * Constructor for the Multi-Layer Perceptron (MLP) network.
         * @param numInputs Number of input neurons.
         * @param numHidden Number of hidden neurons.
         * @param numOutputs Number of output neurons.
         */
        MLP( unsigned long numInputs, unsigned long numHidden, unsigned long numOutputs );

        /** Destructor for the MLP network. */
        ~MLP();

        /** Reset the network's weights to random values. */
        void Reset( void );

        /**
         * Perform one step of the gradient descent training algorithm.
         * @param numSamples Number of samples in the exemplar data.
         * @param inputs Pointer to an array of input samples.
         * @param targets Pointer to an array of target outputs.
         */
        void dGradientBasedTrainingStep( unsigned long numSamples, double *inputs, double *targets );

        /**
         * Get the network's outputs in response to the provided inputs.
         * @param inputArray Pointer to an array of input values.
         * @return Pointer to an array containing the computed network outputs.
         */
        double *pdGetOutputs( const double *inputArray );

        /** Allocate memory for the network structure. */
        void AllocateMemory( void );

        /** Deallocate memory used for the network structure. */
        void DeallocateMemory( void );

        /**
         * Save the network to a file.
         * @param filename Name of the file to save to.
         * @return 0 if successful, -1 if an error occurred.
         */
        int Save( const char *filename );

        /**
         * Load the network from a file.
         * @param filename Name of the file to load from.
         * @return 0 if successful, -1 if an error occurred.
         */
        int Load( const char *filename );

        /** Get the learning rate of the network. */
        double getLearningRate() const;

        /** Set the learning rate of the network. */
        void setLearningRate( double learningRate );

    private:
        unsigned long m_numInputs;       /**< Number of input neurons. */
        unsigned long m_numHiddenNodes;  /**< Number of hidden neurons. */
        unsigned long ulNumberOfOutputs; /**< Number of output neurons. */

        double **ppdwih; /**< Pointer to the weights to the hidden layer nodes. */
        double **ppdwho; /**< Pointer to the weights to the output nodes. */

        double *pdah; /**< Hidden layer activation values. */
        double *pdao; /**< Output layer activation values. */

        double m_learningRate; /**< Learning rate of the network. */
    };

}  // namespace workphone

#endif
