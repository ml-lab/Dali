#ifndef CORE_STACKED_MODEL_H
#define CORE_STACKED_MODEL_H

#include <fstream>
#include <gflags/gflags.h>
#include <initializer_list>
#include <iostream>
#include <map>
#include <sstream>
#include <unordered_map>

#include "dali/mat/CrossEntropy.h"
#include "dali/mat/Softmax.h"
#include "dali/models/RecurrentEmbeddingModel.h"
#include "dali/core.h"
#include "dali/utils.h"

/**
StackedModel
-----------------

A Model for making sequence predictions using stacked LSTM cells.

The network uses an embedding layer, and can reconstruct a sequence.

The objective function is built using masked cross entropy (only certain
input channels collect error over small intervals).

**/
DECLARE_int32(stack_size);
DECLARE_int32(input_size);
DECLARE_int32(hidden);
DECLARE_double(decay_rate);
DECLARE_double(rho);
DECLARE_bool(shortcut);
DECLARE_bool(memory_feeds_gates);

template<typename Z>
class StackedModel : public RecurrentEmbeddingModel<Z>  {
    typedef LSTM<Z>                    lstm;
    typedef Layer<Z>           classifier_t;
    typedef std::map<std::string, std::vector<std::string>> config_t;

    inline void name_parameters();

    public:
        typedef Mat<Z> mat;

        typedef std::vector< typename LSTM<Z>::State> state_type;
        typedef std::tuple<state_type, mat, mat> activation_t;
        typedef Z value_t;
        const bool use_shortcut;
        const bool memory_feeds_gates;

        std::shared_ptr<AbstractStackedLSTM<Z>> stacked_lstm;

        typedef Eigen::Matrix<uint, Eigen::Dynamic, Eigen::Dynamic> index_mat;
        typedef std::shared_ptr< index_mat > shared_index_mat;
        std::shared_ptr<AbstractMultiInputLayer<Z>> decoder;
        virtual std::vector<mat> parameters() const;
        /**
        Load
        ----

        Load a saved copy of this model from a directory containing the
        configuration file named "config.md", and from ".npy" saves of
        the model parameters in the same directory.

        Inputs
        ------

        std::string dirname : directory where the model is currently saved

        Outputs
        -------

        StackedModel<Z> model : the saved model

        **/
        /**
        StackedModel Constructor from configuration map
        ----------------------------------------------------

        Construct a model from a map of configuration parameters.
        Useful for reinitializing a model that was saved to a file
        using the `utils::file_to_map` function to obtain a map of
        configurations.

        **/
        static StackedModel<Z> load(std::string);

        static StackedModel<Z> build_from_CLI(std::string load_location,
                                              int vocab_size,
                                              int output_size,
                                              bool verbose = true);

        StackedModel(int, int, int, int, int, bool use_shortcut = false, bool memory_feeds_gates = false);
        StackedModel(int, int, int, std::vector<int>&, bool use_shortcut = false, bool memory_feeds_gates = false);
        /**StackedModel Constructor from configuration map
        ----------------------------------------------------

        Construct a model from a map of configuration parameters.
        Useful for reinitializing a model that was saved to a file
        using the `utils::file_to_map` function to obtain a map of
        configurations.

        Inputs
        ------

        std::map<std::string, std::vector<std::string>& config : model hyperparameters

        **/
        virtual config_t configuration() const;
        StackedModel(const config_t&);
        /**
        StackedModel<Z>::StackedModel
        -----------------------------

        Copy constructor with option to make a shallow
        or deep copy of the underlying parameters.

        If the copy is shallow then the parameters are shared
        but separate gradients `dw` are used for each of
        thread StackedModel<Z>.

        Shallow copies are useful for Hogwild and multithreaded
        training

        See `Mat<Z>::shallow_copy`, `examples/character_prediction.cpp`,
        `StackedModel<Z>::shallow_copy`

        Inputs
        ------

              StackedModel<Z> l : StackedModel from which to source parameters and dw
                    bool copy_w : whether parameters for new StackedModel should be copies
                                  or shared
                   bool copy_dw : whether gradients for new StackedModel should be copies
                                  shared (Note: sharing `dw` should be used with
                                  caution and can lead to unpredictable behavior
                                  during optimization).

        Outputs
        -------

        StackedModel<Z> out : the copied StackedModel with deep or shallow copy of parameters

        **/
        StackedModel(const StackedModel<Z>&, bool, bool);
        Z masked_predict_cost(
            shared_index_mat,
            shared_index_mat,
            shared_eigen_index_vector,
            shared_eigen_index_vector,
            uint offset=0,
            Z drop_prob = 0.0);
        Z masked_predict_cost(
            shared_index_mat,
            shared_index_mat,
            uint,
            shared_eigen_index_vector,
            uint offset=0,
            Z drop_prob = 0.0);

        virtual std::vector<int> reconstruct(
            Indexing::Index,
            int,
            int symbol_offset = 0) const;

        state_type get_final_activation(
            Indexing::Index,
            Z drop_prob=0.0) const;
        /**
        Activate
        --------

        Run Stacked Model by 1 timestep by observing
        the element from embedding with index `index`
        and report the activation, cell, and hidden
        states

        Inputs
        ------

        std::pair<std::vector<Mat<Z>>, std::vector<Mat<Z>>>& : previous state
        uint index : embedding observation

        Outputs
        -------

        std::pair<std::pair<vector<Mat<Z>>, vector<Mat<Z>>>, Mat<Z> > out :
            pair of LSTM hidden and cell states, and probabilities from the decoder.

        **/
        activation_t activate(state_type&, const uint& ) const;
        activation_t activate(state_type&, const eigen_index_block ) const;

        virtual std::vector<utils::OntologyBranch::shared_branch> reconstruct_lattice(
            Indexing::Index,
            utils::OntologyBranch::shared_branch,
            int) const;

        /**
        Shallow Copy
        ------------

        Perform a shallow copy of a StackedModel<Z> that has
        the same parameters but separate gradients `dw`
        for each of its parameters.

        Shallow copies are useful for Hogwild and multithreaded
        training

        See `StackedModel<Z>::shallow_copy`, `examples/character_prediction.cpp`.

        Outputs
        -------

        StackedModel<Z> out : the copied layer with sharing parameters,
                                   but with separate gradients `dw`

        **/
        StackedModel<Z> shallow_copy() const;

        /**
        Decoder initialization
        ----------------------

        Prepare sequence of input sizes to
        parametrize the decoder for this shorcut
        stacked LSTM model.

        Inputs
        ------

           int input_size : size of input embedding
          int hidden_size : size of internal layers
           int stack_size : how many stacked layers
                            are used.

        Outputs
        -------

        std::vector<int> init_list : sizes needed for decoder init.

        **/
        static std::vector<int> decoder_initialization(int, int, int);


        /**
        Decoder initialization
        ----------------------

        Prepare sequence of input sizes to
        parametrize the decoder for this shorcut
        stacked LSTM model.

        Inputs
        ------

               int input_size : size of input embedding
std::vector<int> hidden_sizes : size of internal layers

        Outputs
        -------

        std::vector<int> init_list : sizes needed for decoder init.

        **/
        static std::vector<int> decoder_initialization(int, std::vector<int>);
        static std::vector<int> decoder_initialization(int, const std::vector<std::string>&);
};

#endif