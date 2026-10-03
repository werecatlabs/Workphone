//QLearing class slighty modified implementation in John Manslow
//qlearning class in the article Using Reinforcement Learning to Solve Ai Control Problems
//which came with the Ai Programming Wisdom 2
//most of the comment were left modified the properties that control learing and displayed out in the
//console for debugging

//slightly modified for VC7
//inlcuded fstream, decleared std is being used
//and edited the load and save functions
#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/AI/MLP.hpp"
#include "stdlib.h"
#include "math.h"
#include "stdio.h"
#include "assert.h"
#include <fstream>
#include "time.h"

namespace workphone
{

    using namespace std;

    MLP::MLP( const unsigned long ulNewNumberOfInputs, const unsigned long ulNewNumberOfHiddenNodes,
              const unsigned long ulNewNumberOfOutputs )
    {
        //Record the structure (number of inputs, hidden neurons, and outputs) of the network
        m_numInputs = ulNewNumberOfInputs;
        m_numHiddenNodes = ulNewNumberOfHiddenNodes;
        ulNumberOfOutputs = ulNewNumberOfOutputs;

        //Allocate memory to store the network's "current" weights and the best weights.
        AllocateMemory();

        //Set the network's weights to random values and reset all variables used in training.
        Reset();

        //Choose some nominal value for the learning rate
        m_learningRate = 0.1;
    }

    void MLP::AllocateMemory()
    {
        unsigned long i;
        pdah = new double[m_numHiddenNodes];
        pdao = new double[ulNumberOfOutputs];

        //Allocate memory to store the current values of the input to hidden layer weights and
        //their best values
        ppdwih = new double *[m_numHiddenNodes];
        for( i = 0; i < m_numHiddenNodes; i++ )
        {
            ppdwih[i] = new double[m_numInputs + 1];
        }

        //Do the same for the hidden to output layer weights
        ppdwho = new double *[ulNumberOfOutputs];
        for( i = 0; i < ulNumberOfOutputs; i++ )
        {
            ppdwho[i] = new double[m_numHiddenNodes + 1];
        }
    }

    void MLP::DeallocateMemory()
    {
        //Deallocate the storage used for current and best weight values.
        unsigned long i;
        for( i = 0; i < m_numHiddenNodes; i++ )
        {
            delete[] ppdwih[i];
        }
        delete[] ppdwih;
        for( i = 0; i < ulNumberOfOutputs; i++ )
        {
            delete[] ppdwho[i];
        }
        delete[] ppdwho;

        delete[] pdah;
        delete[] pdao;
    }

    MLP::~MLP()
    {
        DeallocateMemory();
    }

    void MLP::Reset()
    {
        unsigned long i, j;

        //Give the network weights random values between -1 and +1. Since this effectively resets
        //training the best recorded weights (stored in ppdBest...) are set to the new random values.
        for( i = 0; i < m_numHiddenNodes; i++ )
        {
            for( j = 0; j < m_numInputs + 1; j++ )
            {
                ppdwih[i][j] =
                    0.5 * ( static_cast<double>( rand() ) / static_cast<double>( RAND_MAX ) - 0.5 );
            }
        }

        //Do the same for the hidden to output layer weights
        for( i = 0; i < ulNumberOfOutputs; i++ )
        {
            for( j = 0; j < m_numHiddenNodes + 1; j++ )
            {
                ppdwho[i][j] =
                    0.5 * ( static_cast<double>( rand() ) / static_cast<double>( RAND_MAX ) - 0.5 );
            }
        }
    }

    //This is where the neural network calculates its outputs (Q(a,s) estimates). It is passed a vector containing
    //a and s and returns its estimate of Q(a,s) in the first element of the Array pdao
    auto MLP::pdGetOutputs( const double *const pdInputs ) -> double *
    {
        unsigned long i, j;
        double dStimulus;

        for( i = 0; i < m_numHiddenNodes; i++ )
        {
            dStimulus = ppdwih[i][0];
            for( j = 1; j < m_numInputs + 1; j++ )
            {
                dStimulus += ppdwih[i][j] * pdInputs[j - 1];
            }
            pdah[i] = 1.0 / ( 1.0 + exp( -dStimulus ) );
        }

        for( i = 0; i < ulNumberOfOutputs; i++ )
        {
            dStimulus = ppdwho[i][0];
            for( j = 1; j < m_numHiddenNodes + 1; j++ )
            {
                dStimulus += ppdwho[i][j] * pdah[j - 1];
            }

            //Here are three ways of calculating the network's output (i.e. the activation function of the output
            //neuron). The first and third should work with the demonstration application. Note that when changing
            //these lines, you also need to change the training function immeditaely below

            //The first allows the network to model any size reward, positive or negative
            //		pdao[i]=dStimulus;

            //The second only allows the network to approximate rewards that lie in the range 0.0 to 1.0. In other
            //words, with this output, the network cannot model negative rewards (punishments)
            //		pdao[i]=1.0/(1.0+exp(-dStimulus));

            //The third allows the network to approximate rewards that lie in the range -1.0 to 1.0
            pdao[i] = 2.0 / ( 1.0 + exp( -dStimulus ) ) - 1.0;
        }
        return pdao;
    }

    void MLP::dGradientBasedTrainingStep( unsigned long numSamples, double *inputs, double *targets )
    {
        unsigned long i, j;

        //Allocate memory to store derivatives
        auto pddao = new double[ulNumberOfOutputs];
        auto pddah = new double[m_numHiddenNodes];

        //Make sure you select the line here that corresponds to the activation of the output neuron of the network
        //(see above)
        //for( i = 0; i < ulNumberOfOutputs; i++ )
        //{
        //    //		pddao[i]=-dLearningRate*(pdao[i]-dTargets);
        //    //		pddao[i]=-dLearningRate*(pdao[i]-dTargets)*pdao[i]*(1-pdao[i]);
        //    pddao[i] = -m_learningRate * ( pdao[i] - dTargets ) * ( 1 + pdao[i] ) * ( 1 - pdao[i] );
        //}

        //Update the hidden to output layer weights
        for( i = 0; i < ulNumberOfOutputs; i++ )
        {
            ppdwho[i][0] += pddao[i];
            for( j = 1; j < m_numHiddenNodes + 1; j++ )
            {
                ppdwho[i][j] += pddao[i] * pdah[j - 1];
            }
        }

        //Update the input to hidden layer weights
        for( i = 0; i < m_numHiddenNodes; i++ )
        {
            pddah[i] = 0.0;

            for( j = 0; j < ulNumberOfOutputs; j++ )
            {
                pddah[i] += ppdwho[j][i + 1] * pddao[j];
            }
            pddah[i] *= pdah[i] * ( 1.0 - pdah[i] );

            ppdwih[i][0] += pddah[i];
            //for( j = 1; j < m_numInputs + 1; j++ )
            //{
            //    ppdwih[i][j] += pddah[i] * pdInputs[j - 1];
            //}
        }

        //Clean up memory
        delete[] pddao;
        delete[] pddah;
    }

    auto MLP::Save( const char *const pFileName ) -> int
    {
        unsigned long i, j;
        assert( pFileName );

        //Create the output stream to save the network to.
        auto pOut = new ofstream( pFileName );

        //Make sure it was created and opened successfully.
        if( !pOut )
        {
            assert( false );
            return 0;
        }
        if( !pOut->is_open() )
        {
            assert( false );
            delete pOut;
            return 0;
        }

        //Make sure we don't lose information.
        pOut->precision( 15 );

        //Save all the network info:
        //Its structure...
        *pOut << m_numInputs;
        *pOut << "\n";
        *pOut << m_numHiddenNodes;
        *pOut << "\n";
        *pOut << ulNumberOfOutputs;
        *pOut << "\n";
        *pOut << m_learningRate;
        *pOut << "\n";
        //Its weights
        for( i = 0; i < m_numHiddenNodes; i++ )
        {
            for( j = 0; j < m_numInputs + 1; j++ )
            {
                *pOut << ppdwih[i][j];
                *pOut << "\t";
            }
            *pOut << "\n";
        }
        for( i = 0; i < ulNumberOfOutputs; i++ )
        {
            for( j = 0; j < m_numHiddenNodes + 1; j++ )
            {
                *pOut << ppdwho[i][j];
                *pOut << "\t";
            }
            *pOut << "\n";
        }

        //Close the file and delete the stream.
        pOut->close();
        delete pOut;

        //Return that the save was successful.*/
        return 1;
    }

    auto MLP::Load( const char *const pFileName ) -> int
    {
        unsigned long i, j;
        //assert(pFileName);

        //Create a stream to load the network from
        auto pIn = new ifstream( pFileName );

        //Check to make sure that it was created and could be opened.
        if( !pIn )
        {
            //assert(false);
            return 0;
        }
        if( !pIn->is_open() )
        {
            //assert(false);
            delete pIn;
            return 0;
        }

        //Since we're about to load a new network, we should delete the storage used by the
        //current one to prevent memory leaks.
        DeallocateMemory();

        //Load the structure of the new network
        *pIn >> m_numInputs;
        *pIn >> m_numHiddenNodes;
        *pIn >> ulNumberOfOutputs;
        *pIn >> m_learningRate;
        //Allocate memory to store its weights
        AllocateMemory();

        //Reset its status so that it can be trained if necessary
        Reset();

        //Load in its weights
        for( i = 0; i < m_numHiddenNodes; i++ )
        {
            for( j = 0; j < m_numInputs + 1; j++ )
            {
                *pIn >> ppdwih[i][j];
            }
        }
        for( i = 0; i < ulNumberOfOutputs; i++ )
        {
            for( j = 0; j < m_numHiddenNodes + 1; j++ )
            {
                *pIn >> ppdwho[i][j];
            }
        }

        //Close and delete the stream.
        pIn->close();
        delete pIn;

        //Indicate that we've been successful.
        return 1;
    }

    auto MLP::getLearningRate() const -> double
    {
        return m_learningRate;
    }

    void MLP::setLearningRate( double learningRate )
    {
        m_learningRate = learningRate;
    }

}  // namespace workphone
