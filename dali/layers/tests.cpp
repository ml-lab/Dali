#include <chrono>
#include <vector>
#include <iomanip>
#include <gtest/gtest.h>

#include "dali/test_utils.h"
#include "dali/tensor/Index.h"
#include "dali/layers/Layers.h"
#include "dali/layers/LSTM.h"
#include "dali/layers/GRU.h"
#include "dali/tensor/Mat.h"
#include "dali/tensor/MatOps.h"
#include "dali/tensor/Tape.h"
#include "dali/tensor/CrossEntropy.h"
#include "dali/tensor/Solver.h"

using std::vector;
using std::chrono::milliseconds;



typedef MemorySafeTest LayerTests;

TEST_F(LayerTests, layer_tanh_gradient) {
    int num_examples = 7;
    int hidden_size = 10;
    int input_size = 5;

    EXPERIMENT_REPEAT {
        auto X  = Mat<R>(input_size, num_examples, weights<R>::uniform(20.0));
        auto mylayer = Layer<R>(input_size, hidden_size);
        auto params = mylayer.parameters();
        params.emplace_back(X);
        auto functor = [&mylayer](vector<Mat<R>> Xs)-> Mat<R> {
            return mylayer.activate(Xs.back()).tanh();
        };
        ASSERT_TRUE(gradient_same(functor, params, 0.0003));
    }
}



TEST_F(LayerTests, BroadcastMultiply) {

    int large_size = 7;
    int out_size   = 10;

    // different input sizes passed to a stacked input layer
    vector<int> input_sizes   = {5,  2,  5,  1, 5};

    // broadcast the 1s into the larger dimension:
    vector<int> example_sizes = {large_size, 1, large_size, 1, large_size};

    EXPERIMENT_REPEAT {
        // build layer
        auto mylayer = StackedInputLayer<R>(input_sizes, out_size);

        // build inputs
        vector<Mat<R>> inputs;
        for (int i = 0; i < input_sizes.size(); i++) {
            inputs.emplace_back(input_sizes[i], example_sizes[i], weights<R>::uniform(5.0));
        }

        // add the params
        auto params = mylayer.parameters();
        params.insert(params.end(), inputs.begin(), inputs.end());

        // project
        auto functor = [&mylayer, &inputs](vector<Mat<R>> Xs)-> Mat<R> {
            return mylayer.activate(inputs);
        };
        ASSERT_TRUE(gradient_same(functor, params, 0.0003));
    }
}

TEST_F(LayerTests, stacked_layer_tanh_gradient) {

    int num_examples = 10;
    int hidden_size  = 10;
    int input_size_1 = 5;
    int input_size_2 = 8;
    int input_size_3 = 12;

    EXPERIMENT_REPEAT {
        auto A  = Mat<R>(
            input_size_1,
            num_examples,
            weights<R>::uniform(20.0));
        auto B  = Mat<R>(
            input_size_2,
            num_examples,
            weights<R>::uniform(20.0));
        auto C  = Mat<R>(
            input_size_3,
            num_examples,
            weights<R>::uniform(20.0));
        auto mylayer = StackedInputLayer<R>({
            input_size_1,
            input_size_2,
            input_size_3}, hidden_size);
        auto params = mylayer.parameters();
        params.emplace_back(A);
        params.emplace_back(B);
        params.emplace_back(C);
        auto functor = [&mylayer, &A, &B, &C](vector<Mat<R>> Xs)-> Mat<R> {
            return mylayer.activate({A, B, C}).tanh();
        };
        ASSERT_TRUE(gradient_same(functor, params, 0.0003));
    }
}

TEST_F(LayerTests, DISABLED_LSTM_Zaremba_gradient) {

    int num_examples           = 10;
    int hidden_size            = 5;
    int input_size             = 3;

    EXPERIMENT_REPEAT {
        auto X  = Mat<R>(input_size, num_examples, weights<R>::uniform(20.0));
        auto mylayer = LSTM<R>(input_size, hidden_size, false);
        auto params = mylayer.parameters();
        params.emplace_back(X);
        auto initial_state = mylayer.initial_states();
        auto functor = [&mylayer, &X, &initial_state](vector<Mat<R>> Xs)-> Mat<R> {
            auto myout_state = mylayer.activate(X, initial_state);
            return myout_state.hidden;
        };
        ASSERT_TRUE(gradient_same(functor, params, 0.0003));
    }
}

TEST_F(LayerTests, DISABLED_LSTM_Graves_gradient) {

    int num_examples           = 10;
    int hidden_size            = 5;
    int input_size             = 3;

    EXPERIMENT_REPEAT {
        auto X  = Mat<R>(input_size, num_examples,      weights<R>::uniform(20.0));
        auto mylayer = LSTM<R>(input_size, hidden_size, true);
        auto params = mylayer.parameters();
        params.emplace_back(X);

        // In an LSTM we do not back prop through the cell activations when using
        // it in a gate, but to test it here we set this to true
        mylayer.backprop_through_gates = true;

        auto initial_state = mylayer.initial_states();
        auto functor = [&mylayer, &X, &initial_state](vector<Mat<R>> Xs)-> Mat<R> {
            auto myout_state = mylayer.activate(X, initial_state);
            return myout_state.hidden;
        };
        ASSERT_TRUE(gradient_same(functor, params, 0.0003));
    }
}

TEST_F(LayerTests, DISABLED_LSTM_Graves_shortcut_gradient) {

    int num_examples           = 10;
    int hidden_size            = 5;
    int input_size             = 3;
    int shortcut_size          = 2;

    EXPERIMENT_REPEAT {
        auto X  = Mat<R>(input_size,    num_examples, weights<R>::uniform(20.0));
        auto X_s = Mat<R>(shortcut_size, num_examples, weights<R>::uniform(20.0));
        auto mylayer = LSTM<R>({input_size, shortcut_size}, hidden_size, 1, true);
        auto params = mylayer.parameters();
        params.emplace_back(X);
        params.emplace_back(X_s);

        // In an LSTM we do not back prop through the cell activations when using
        // it in a gate:
        mylayer.backprop_through_gates = true;

        auto initial_state = mylayer.initial_states();
        auto functor = [&mylayer, &X, &X_s, &initial_state](vector<Mat<R>> Xs)-> Mat<R> {
            auto state = mylayer.activate_shortcut(X, X_s, initial_state);
            return state.hidden;
        };
        ASSERT_TRUE(gradient_same(functor, params, 0.0003));
    }
}

TEST_F(LayerTests, DISABLED_LSTM_Zaremba_shortcut_gradient) {
    int num_examples           = 10;
    int hidden_size            = 5;
    int input_size             = 3;
    int shortcut_size          = 2;

    EXPERIMENT_REPEAT {
        auto X  = Mat<R>(input_size,    num_examples, weights<R>::uniform(20.0));
        auto X_s = Mat<R>(shortcut_size, num_examples, weights<R>::uniform(20.0));
        auto mylayer = LSTM<R>({input_size, shortcut_size}, hidden_size, 1, false);
        auto params = mylayer.parameters();
        params.emplace_back(X);
        params.emplace_back(X_s);

        auto initial_state = mylayer.initial_states();
        auto functor = [&mylayer, &X, &X_s, &initial_state](vector<Mat<R>> Xs)-> Mat<R> {
            auto state = mylayer.activate_shortcut(X, X_s, initial_state);
            return state.hidden;
        };
        ASSERT_TRUE(gradient_same(functor, params, 0.0003));
    }
}

TEST_F(LayerTests, DISABLED_RNN_gradient_vs_Stacked_gradient) {
    // int num_examples           = 10;
    // int hidden_size            = 5;
    // int input_size             = 3;

    // EXPERIMENT_REPEAT {

    //     auto X  = Mat<R>(input_size, num_examples, weights<R>::uniform(20.0));
    //     auto H  = Mat<R>(hidden_size, num_examples, weights<R>::uniform(20.0));

    //     auto X_s  = Mat<R>(X, true, true); // perform full copies
    //     auto H_s  = Mat<R>(H, true, true); // perform full copies

    //     auto rnn_layer = RNN<R>(input_size, hidden_size);
    //     auto stacked_layer = StackedInputLayer<R>({input_size, hidden_size}, hidden_size);

    //     auto params = rnn_layer.parameters();
    //     auto stacked_params = stacked_layer.parameters();

    //     for (auto it1 = params.begin(),
    //               it2 = stacked_params.begin(); (it1 != params.end()) && (it2 != stacked_params.end()); it1++, it2++) {
    //         ASSERT_EQ((*it1).dims(), (*it2).dims());
    //         std::copy(it2->w()->data(), it2->w()->data() + it2->number_of_elements(),
    //                   it1->w()->data());
    //     }

    //     auto error = ((rnn_layer.activate(X, H).tanh() - 1) ^ 2).sum();
    //     error.grad();
    //     auto error2 = ((stacked_layer.activate({X_s, H_s}).tanh() - 1) ^ 2).sum();
    //     error2.grad();
    //     graph::backward();

    //     for (auto it1 = params.begin(),
    //               it2 = stacked_params.begin(); (it1 != params.end()) && (it2 != stacked_params.end()); it1++, it2++) {
    //         ASSERT_MATRIX_GRAD_CLOSE((*it1), (*it2), 1e-6);
    //     }
    //     ASSERT_MATRIX_GRAD_CLOSE(X, X_s, 1e-6);
    //     ASSERT_MATRIX_GRAD_CLOSE(H, H_s, 1e-6);
    // }
}



TEST_F(LayerTests, DISABLED_shortcut_test) {
    int input_size = 10;
    int num_examples = 2;
    auto hidden_sizes = {40, 30};//{30, 13, 20, 1, 9, 2};

    auto model = StackedLSTM<R>(input_size, hidden_sizes, true, true);
    auto X = {Mat<R>(
        input_size,
        num_examples,
        weights<R>::uniform(20.0)
    )};

    auto out_states = model.activate_sequence(model.initial_states(),
                                              X,
                                              0.2);
}

TEST_F(LayerTests, DISABLED_multi_input_lstm_test) {
    int num_children = 3;
    int input_size = 4;
    int hidden_size = 2;
    int num_examples = 3;

    EXPERIMENT_REPEAT {
        auto input = Mat<R>(input_size,    num_examples, weights<R>::uniform(20.0));
        vector<LSTM<R>::State> states;
        for (int cidx = 0 ; cidx < num_children; ++cidx) {
            states.emplace_back(
                Mat<R>(hidden_size, num_examples, weights<R>::uniform(20.0)),
                Mat<R>(hidden_size, num_examples, weights<R>::uniform(20.0))
            );
        }

        auto mylayer = LSTM<R>(input_size, hidden_size, num_children);
        auto params = mylayer.parameters();
        params.emplace_back(input);
        for(auto& state: states) {
            params.emplace_back(state.memory);
            params.emplace_back(state.hidden);
        }

        auto functor = [&mylayer, &input, &states](vector<Mat<R>> Xs)-> Mat<R> {
                auto state = mylayer.activate(input, states);
                return state.hidden;
        };
        ASSERT_TRUE(gradient_same(functor, params, 0.0003));

        utils::Timer::report();
    }
}

TEST_F(LayerTests, DISABLED_activate_sequence) {
    vector<int> hidden_sizes = {7, 10};
    int input_size = 5;
    int num_out_states = hidden_sizes.size();
    vector<Mat<R>> sequence;
    for (int i = 0; i < 10; i++) {
        sequence.emplace_back(input_size, 1);
    }
    auto model = StackedLSTM<R>(input_size, hidden_sizes, false, false);
    auto out_states = model.activate_sequence(model.initial_states(), sequence, 0.1);
    ASSERT_EQ(num_out_states, LSTM<R>::State::hiddens(out_states).size());
}

TEST_F(LayerTests, DISABLED_GRU) {
    int input_size = 3;
    int hidden_size = 5;
    int tsteps = 5;

    EXPERIMENT_REPEAT {
        auto gru = GRU<R>(input_size, hidden_size);
        auto params = gru.parameters();
        auto inputs = vector<Mat<R>>();
        for (int i = 0; i < tsteps; i++) inputs.emplace_back(Mat<R>(input_size,1, weights<R>::uniform(20.0)));
        auto functor = [&inputs, &gru,&tsteps, &hidden_size, &input_size](vector<Mat<R>> Xs)-> Mat<R> {
            auto state = Mat<R>(hidden_size, 1);
            for (int i = 0; i < tsteps; i++)
                state = gru.activate(inputs[i], state);
            return (state -1.0) ^ 2;
        };
        ASSERT_TRUE(gradient_same(functor, params, 1e-5));
    }
}