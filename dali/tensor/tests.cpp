#include <chrono>
#include <vector>
#include <iomanip>
#include <gtest/gtest.h>

#include "dali/test_utils.h"
// #include "dali/tensor/Index.h"
// #include "dali/layers/Layers.h"
#include "dali/tensor/tensor.h"
#include "dali/tensor/op.h"
#include "dali/array/op.h"
#include "dali/tensor/solver.h"

using std::vector;
using std::chrono::milliseconds;

class TensorTests : public MemorySafeTest {
  protected:
    static void SetUpTestCase() {
    }
};

TEST_F(TensorTests, sum_test) {
    auto A = Tensor({10, 20}, initializer::uniform(-2.0, 2.0), DTYPE_FLOAT);
    auto res = A.sum();
    float sum = 0.0;
    for (int i = 0; i < A.number_of_elements(); ++i) {
        sum += (float)A.w(i);
    }
    ASSERT_NEAR(sum, (float)res.w(0), 1e-4);
}

TEST_F(TensorTests, max_min_test) {
    Tensor A({5, 5}, DTYPE_INT32);
    for (int i = 0; i < A.number_of_elements(); i++) {
        A.w(i) = i - 15;
    }

    ASSERT_NEAR((int)A.min().w, -15, 1e-6);
    ASSERT_NEAR((int)A.max().w, 9, 1e-6);
}

TEST_F(TensorTests, equals) {
    Tensor A({10, 20}, initializer::uniform(-2.0, 2.0));
    Tensor B({10, 20}, initializer::uniform(-2.0, 2.0));

    EXPECT_TRUE(tensor_ops::equals(A, A)) << "A equals A.";
    EXPECT_FALSE(tensor_ops::equals(A, B)) << "A different from B.";
    EXPECT_TRUE(tensor_ops::allclose(A, A, 1e-4)) << "A near A.";
    EXPECT_FALSE(tensor_ops::allclose(A, B, 1e-4)) << "A not near B.";

    EXPECT_MAT_ON_GPU(A);
    EXPECT_MAT_ON_GPU(B);
}

TEST_F(TensorTests, L2_norm) {
    auto functor = [](vector<Tensor> Xs)-> Tensor {
        return tensor_ops::L2_norm(Xs[0]);
    };

    EXPERIMENT_REPEAT {
        Tensor A({2, 3}, initializer::uniform(-2.0, 2.0), DTYPE_DOUBLE);
        expect_args_remain_on_gpu(functor, {A});
        EXPECT_TRUE(gradient_same(functor, {A}));
    }
}


TEST_F(TensorTests, sum) {
    auto functor = [](vector<Tensor> Xs)-> Tensor {
        return Xs[0].sum();
    };

    EXPERIMENT_REPEAT {
        auto A = Tensor({10, 20}, initializer::uniform(-2.0, 2.0), DTYPE_DOUBLE);
        expect_args_remain_on_gpu(functor, {A});
        EXPECT_TRUE(gradient_same(functor, {A}));
    }
}

#define DEFINE_REDUCTION_TENSOR_TEST(TESTNAME, REDUCTION_NAME, LOWER_BOUND, UPPER_BOUND, FAIL_ON_ZERO_GRADIENT)\
    TEST_F(TensorTests, TESTNAME) {\
        int axis;\
        auto functor = [&axis](vector<Tensor> Xs)-> Tensor {\
            return tensor_ops::REDUCTION_NAME(Xs[0], axis);\
        };\
        EXPERIMENT_REPEAT {\
            std:vector<int> shape = {2, 3, 1, 1, 2, 3};\
            for (axis = 0; axis < shape.size(); axis++) {\
                Tensor A(shape, initializer::uniform(LOWER_BOUND, UPPER_BOUND), DTYPE_DOUBLE);\
                expect_args_remain_on_gpu(functor, {A});\
                EXPECT_TRUE(gradient_same(functor, {A}, 1e-3, 1e-4, FAIL_ON_ZERO_GRADIENT));\
            }\
        }\
    }

DEFINE_REDUCTION_TENSOR_TEST(L2_norm_axis, L2_norm, -2.0, 2.0, true);
DEFINE_REDUCTION_TENSOR_TEST(sum_axis, sum, -2.0, 2.0, true);
DEFINE_REDUCTION_TENSOR_TEST(mean_axis, mean, -2.0, 2.0, true);
DEFINE_REDUCTION_TENSOR_TEST(min_axis_positive, min, 0.1, 20.0, true);
DEFINE_REDUCTION_TENSOR_TEST(max_axis_positive, max, 0.1, 20.0, true);
DEFINE_REDUCTION_TENSOR_TEST(min_axis_negative, min, -20.0, -0.1, true);
DEFINE_REDUCTION_TENSOR_TEST(max_axis_negative, max, -20.0, -0.1, true);

TEST_F(TensorTests, sigmoid_gpu_vs_cpu) {
    auto functor = [](vector<Tensor> Xs)-> Tensor {
        return Xs[0].sigmoid();
    };

    EXPERIMENT_REPEAT {
        Tensor A({10, 20}, initializer::uniform(-2.0, 2.0), DTYPE_DOUBLE);
        EXPECT_TRUE(cpu_vs_gpu(functor, {A}));
    }
}

// TEST_F(MatrixTests, identity_init) {
//     R init_val = 2.0;
//     auto A = Mat<R>(10, 10, weights<R>::eye(init_val));
//     EXPECT_MAT_ON_GPU(A);
//     for (int i = 0; i < A.dims(0); i++) {
//         for (int j = 0; j < A.dims(1); j++) {
//             if (i == j) {
//                 EXPECT_TRUE(A.w(i, j) == init_val);
//             } else {
//                 EXPECT_TRUE(A.w(i, j) == 0.0);
//             }
//         }
//     }
// }
//
//
//
// TEST_F(MatrixTests, max_scalar) {
//     int input_size = 5;
//     int hidden_size = 3;
//
//     EXPERIMENT_REPEAT {
//         auto mat = Mat<R>(input_size, hidden_size, weights<R>::uniform(1.5, 20.0));
//         auto mat2 = Mat<R>(input_size, hidden_size, weights<R>::uniform(-20.0, 1.3));
//         auto combined = MatOps<R>::hstack({mat, mat2});
//         R maxxand = 1.4;
//         auto functor = [&maxxand](vector<Mat<R>> Xs)-> Mat<R> {
//             return MatOps<R>::eltmax(Xs[0], maxxand);
//         };
//         ASSERT_TRUE(gradient_same(functor, {combined}, 1e-4));
//     }
// }
//
//
// TEST_F(MatrixTests, addition_vector) {
//     auto functor = [](vector<Mat<R>> Xs)-> Mat<R> {
//         return MatOps<R>::add({ Xs[0], Xs[1], Xs[2] });
//     };
//     EXPERIMENT_REPEAT {
//         auto A = Mat<R>(10, 20, weights<R>::uniform(2.0));
//         auto B = Mat<R>(10, 20,  weights<R>::uniform(0.5));
//         auto C = Mat<R>(10, 20,  weights<R>::uniform(0.5));
//         ASSERT_TRUE(gradient_same(functor, {A, B, C}, 1e-5, DEFAULT_GRAD_EPS, true));
//     }
// }
//
// TODO: implement repmat
// TEST_F(MatrixTests, broadcast_row_vector) {
//     auto functor = [](vector<Mat<R>> Xs)-> Mat<R> {
//         return MatOps<R>::broadcast_row_vector(Xs[0], 10);
//     };
//     EXPERIMENT_REPEAT {
//         auto A = Mat<R>(1,  20,  weights<R>::uniform(2.0));
//         ASSERT_TRUE(gradient_same(functor, {A}, 1e-5, DEFAULT_GRAD_EPS, true));
//     }
// }
//
// TEST_F(MatrixTests, broadcast_col_vector) {
//     auto functor = [](vector<Mat<R>> Xs)-> Mat<R> {
//         return MatOps<R>::broadcast_col_vector(Xs[0], 10);
//     };
//     EXPERIMENT_REPEAT {
//         auto A = Mat<R>(20,  1,  weights<R>::uniform(2.0));
//         ASSERT_TRUE(gradient_same(functor, {A}, 1e-5, DEFAULT_GRAD_EPS, true));
//     }
// }
//


TEST(TensorIOTests, load_fortran) {
    auto arange = Tensor::load(
        utils::dir_join({STR(DALI_DATA_DIR), "tests", "arange12.npy"})
    );
    auto arange_fortran = Tensor::load(
        utils::dir_join({STR(DALI_DATA_DIR), "tests", "arange12.fortran.npy"})
    );
    ASSERT_TRUE(tensor_ops::equals(arange, arange_fortran));
    for (int i = 0; i < 12; i++) {
        EXPECT_EQ_DTYPE(i, arange_fortran.w(i), arange_fortran.dtype());
    }
}

TEST(TensorIOTests, save_load_test) {
    // load arange, then save it to a new file
    auto arange = Tensor::load(
        utils::dir_join({STR(DALI_DATA_DIR), "tests", "arange12.npy"})
    );
    Tensor::save(
        utils::dir_join({STR(DALI_DATA_DIR),  "tests", "arange12.temp.npy"}),
        arange
    );
    auto reloaded = Tensor::load(
        utils::dir_join({STR(DALI_DATA_DIR),  "tests", "arange12.temp.npy"})
    );
    ASSERT_TRUE(tensor_ops::equals(arange, reloaded));
    for (int i = 0; i < 12; i++) {
        EXPECT_EQ_DTYPE(i, arange.w(i), arange.dtype());
    }
}

TEST_F(TensorTests, lazy_allocation) {
    // if memory must be filled with zeros,
    // then allocation is lazy
    auto zero_mat = Tensor::zeros({4,5});

    ASSERT_TRUE(!zero_mat.w.memory()->is_any_allocated());
    ASSERT_TRUE(!zero_mat.dw.memory()->is_any_allocated());

    // if memory must be filled with gaussian
    // noise, allocation is not lazy
    Tensor gauss_mat({4, 5}, initializer::gaussian(0.0, 0.5));

    #ifdef DALI_USE_CUDA
    ASSERT_TRUE(gauss_mat.w.memory()->is_allocated(memory::Device::gpu(0)) && !gauss_mat.w.memory()->is_allocated(memory::Device::cpu()));
    #else
    ASSERT_TRUE(gauss_mat.w.memory()->is_allocated(memory::Device::cpu()));
    #endif

    // the gradients are set to 0, but are also lazily
    // allocated and cleared.
    ASSERT_TRUE(!gauss_mat.dw.memory()->is_any_allocated());
}

TEST_F(TensorTests, view_transpose) {
    // transpose is a view
    Tensor a({4, 5}, initializer::arange());
    auto a_T = a.transpose();
    ASSERT_EQ(a.w.memory(), a_T.w.memory());
}

TEST_F(TensorTests, reshape) {
    // reshape is a view
    Tensor a({4, 5}, initializer::arange());
    auto a_reshaped = a.reshape({2, 2, 1, 5});
    ASSERT_EQ(a.w.memory(), a_reshaped.w.memory());
}

//
// TEST_F(MatrixTests, slice) {
//     auto functor = [](vector<Mat<R>> Xs)-> Mat<R> {
//         return Xs[0].slice(2, 5);
//     };
//     EXPERIMENT_REPEAT {
//         Mat<R> block(10, 2, weights<R>::uniform(2.0));
//         ASSERT_TRUE(gradient_same(functor, {block}));
//     }
//
//     Mat<R> block(10, 2, weights<R>::uniform(2.0));
//     auto subblock = block.slice(2, 5);
//
//     // ensure the slice is a view!
//     ASSERT_EQ(&subblock.w().memory() , &block.w().memory());
//     ASSERT_EQ(&subblock.dw().memory() , &block.dw().memory());
// }

// TEST_F(MatrixTests, argmax_argmin) {
//     auto A = Mat<R>(5, 5, weights<R>::eye());
//     // orientation in an identity matrix does not matter
//     // for argmax:
//     auto indices_max_col = A.argmax(0);
//     EXPECT_EQ(indices_max_col, std::vector<int>({0, 1, 2, 3, 4}));
//     auto indices_max_row = A.argmax(1);
//     EXPECT_EQ(indices_max_row, std::vector<int>({0, 1, 2, 3, 4}));
//
//     // however for an irregular assymetric pattern, then
//     // orientation matters:
//     auto B   = Mat<R>(6, 5);
//     B.w(0,0) = -12.0;
//     B.w(1,3) = -32.0;
//     B.w(2,4) = -44.0;
//     B.w(3,0) = -35.0;
//     B.w(4,2) = -32.0;
//     B.w(5,3) = -27.0;
//     #ifdef DALI_USE_CUDA
//     // force computation to happen on device if possible
//     B.w().memory().to_gpu();
//     #endif
//
//     auto indices_min_col = B.argmin(0);
//     EXPECT_EQ(indices_min_col, std::vector<int>({3, 0, 4, 1, 2}));
//
//     auto indices_min_row = B.argmin(1);
//     EXPECT_EQ(indices_min_row, std::vector<int>({0, 3, 4, 0, 2, 3}));
//
//     auto Z = Mat<R>(3, 9);
//     Z.w(12) = 55;
//
//     Z.w(13) = -12;
//     #ifdef DALI_USE_CUDA
//     // force computation to happen on device if possible
//     Z.w().memory().to_gpu();
//     #endif
//
//     // argmin without dimension argument treats
//     // matrix as one long strand of memory
//     EXPECT_EQ(Z.argmin(), 13);
//     EXPECT_EQ(Z.argmax(), 12);
// }
//
// TEST_F(MatrixTests, argsort) {
//     auto mats = vector<Mat<R>>({
//         MatOps<R>::fill(Mat<R>(1,1), 3),
//         MatOps<R>::fill(Mat<R>(1,1), 9),
//         MatOps<R>::fill(Mat<R>(1,1), 1),
//     });
//     auto sorted = utils::argsort(mats);
//     ASSERT_EQ(sorted, std::vector<size_t>({2, 0, 1}));
// }
//
//
// TEST_F(MatrixTests, mat_argsort) {
//     // shape of matrix has no influence on
//     // argsort
//     auto A = Mat<R>(2, 2);
//     A.w(0) = -5;
//     A.w(1) =  12.0;
//     A.w(2) = -33.0;
//     A.w(3) =  66.0;
//
//     #ifdef DALI_USE_CUDA
//     A.w().memory().to_gpu();
//     #endif
//
//     auto sorted = A.argsort();
//     ASSERT_EQ(sorted, std::vector<int>({2, 0, 1, 3}));
// }
//
TEST_F(TensorTests, mean) {
    Tensor B({3, 4}, initializer::ones(), DTYPE_DOUBLE);
    auto res = B.mean();
    ASSERT_NEAR(1.0, (double)res.w, 1e-6);

    auto functor = [](vector<Tensor> Xs)-> Tensor {
        return Xs[0].mean();
    };
    EXPERIMENT_REPEAT {
        Tensor A({10, 20}, initializer::uniform(-2.0, 2.0), DTYPE_DOUBLE);
        ASSERT_TRUE(gradient_same(functor, {A}));
    }
}

TEST_F(TensorTests, max) {
    auto functor = [](vector<Tensor> Xs)-> Tensor {
        return Xs[0].max();
    };
    Tensor B({2, 3}, initializer::arange(), DTYPE_DOUBLE);
    auto res = B.max();
    ASSERT_NEAR(5.0, (double)res.w, 1e-6);

    EXPERIMENT_REPEAT {
        Tensor A({2, 3}, initializer::arange(), DTYPE_DOUBLE);
        ASSERT_TRUE(gradient_same(functor, {A}));
    }
}

TEST_F(TensorTests, min) {
    auto functor = [](vector<Tensor> Xs)-> Tensor {
        return Xs[0].min();
    };
    Tensor B({2, 3}, initializer::arange(), DTYPE_DOUBLE);
    auto res = B.min();
    ASSERT_NEAR(0.0, (double)res.w, 1e-6);

    EXPERIMENT_REPEAT {
        Tensor A({2, 3}, initializer::arange(), DTYPE_DOUBLE);
        ASSERT_TRUE(gradient_same(functor, {A}));
    }
}

#define DEFINE_UNARY_TENSOR_TEST(TESTNAME, UNARY, LOWER_BOUND, UPPER_BOUND, EPS, FAIL_ON_ZERO_GRADIENT)\
    TEST(TensorUnaryTests, TESTNAME) {\
        utils::random::set_seed(100);\
        auto functor = [](vector<Tensor> Xs)-> Tensor {\
            return Xs[0].UNARY();\
        };\
        EXPERIMENT_REPEAT {\
            Tensor A({2, 3, 4},\
                initializer::uniform(LOWER_BOUND, UPPER_BOUND),\
                DTYPE_DOUBLE\
            );\
            ASSERT_TRUE(gradient_same(functor, {A}, EPS, 1e-4, FAIL_ON_ZERO_GRADIENT));\
        }\
        utils::random::reseed();\
    }\

DEFINE_UNARY_TENSOR_TEST(square, square, -5.0, 5.0, 1e-3, true);
DEFINE_UNARY_TENSOR_TEST(sqrt, sqrt, 0.5, 5.0, 1e-3, true);
DEFINE_UNARY_TENSOR_TEST(eltinv, eltinv, 0.5, 5.0, 1e-3, true);
DEFINE_UNARY_TENSOR_TEST(sigmoid, sigmoid, -20.0, 20.0, 1e-3, true);
DEFINE_UNARY_TENSOR_TEST(tanh, tanh, -20.0, 20.0, 1e-3, true);
DEFINE_UNARY_TENSOR_TEST(softplus, softplus, -20.0, 20.0, 1e-2, true);
DEFINE_UNARY_TENSOR_TEST(log, log, 0.1, 20.0, 1e-2, true);

DEFINE_UNARY_TENSOR_TEST(exp, exp, -20.0, 2.0, 0.1, true);

DEFINE_UNARY_TENSOR_TEST(relu_positive, relu, 0.2, 20.0, 1e-3, true);
DEFINE_UNARY_TENSOR_TEST(relu_negative, relu, -20.0, -0.2, 1e-3, false);
DEFINE_UNARY_TENSOR_TEST(abs_positive, abs, 0.2, 20.0, 1e-3, true);
DEFINE_UNARY_TENSOR_TEST(abs_negative, abs, -20.0, -0.2, 1e-3, true);

// TEST_F(MatrixTests, binary_cross_entropy) {
//     // We observe the KL divergence to 0 or 1 for each unit
//     // in our input matrix with respect to the target.
//     EXPERIMENT_REPEAT {
//         auto A = Mat<R>(10, 20, weights<R>::uniform(0.1, 0.9));
//         R target = utils::randdouble(0.01, 0.99);
//         auto functor = [target](vector<Mat<R>> Xs)-> Mat<R> {
//             return MatOps<R>::binary_cross_entropy(Xs[0], target);
//         };
//         ASSERT_TRUE(gradient_same(functor, {A}, 1e-2));
//     }
// }
//
// TEST_F(MatrixTests, binary_cross_entropy_matrix_target) {
//     // We observe the KL divergence to 0 or 1 for each unit
//     // in our input matrix with respect to the target.
//     EXPERIMENT_REPEAT {
//         auto A = Mat<R>(10, 20, weights<R>::uniform(0.1, 0.9));
//         auto target = Mat<R>(10, 20, weights<R>::uniform(0.1, 0.9));
//         target.constant = true;
//         auto functor = [target](vector<Mat<R>> Xs)-> Mat<R> {
//             return MatOps<R>::binary_cross_entropy(Xs[0], target);
//         };
//         ASSERT_TRUE(gradient_same(functor, {A}, 1e-2));
//     }
// }
//
// TEST_F(MatrixTests, sigmoid_binary_cross_entropy) {
//     // we can now extend the range of our random numbers to be beyond
//     // 0 and 1 since sigmoid will clamp them to 0 or 1.
//     EXPERIMENT_REPEAT {
//         auto A = Mat<R>(10, 20, weights<R>::uniform(5.0));
//         R target = utils::randdouble(0.1, 0.9);
//         auto functor = [target](vector<Mat<R>> Xs)-> Mat<R> {
//             return MatOps<R>::sigmoid_binary_cross_entropy(Xs[0], target);
//         };
//         ASSERT_TRUE(gradient_same(functor, {A}, 1e-2, 1e-4));
//     }
// }
//
// TEST_F(MatrixTests, sigmoid_binary_cross_entropy_matrix_target) {
//     // we can now extend the range of our random numbers to be beyond
//     // 0 and 1 since sigmoid will clamp them to 0 or 1.
//     EXPERIMENT_REPEAT {
//         auto A = Mat<R>(10, 20, weights<R>::uniform(5.0));
//         auto target = Mat<R>(10, 20, weights<R>::uniform(0.1, 0.9));
//         target.constant = true;
//         auto functor = [target](vector<Mat<R>> Xs)-> Mat<R> {
//             return MatOps<R>::sigmoid_binary_cross_entropy(Xs[0], target);
//         };
//         ASSERT_TRUE(gradient_same(functor, {A}, 1e-2, 1e-4));
//     }
// }
//
// TEST_F(MatrixTests, margin_loss_colwise) {
//     utils::random::set_seed(100);
//     // we can now extend the range of our random numbers to be beyond
//     // 0 and 1 since sigmoid will clamp them to 0 or 1.
//     EXPERIMENT_REPEAT {
//         auto A = Mat<R>(10, 20, weights<R>::uniform(5.0));
//         R margin = utils::randdouble(0.01, 0.1);
//         uint target = utils::randinteger<uint>(0, A.dims(0) - 1);
//         auto functor = [target, margin](vector<Mat<R>> Xs)-> Mat<R> {
//             return MatOps<R>::margin_loss_colwise(Xs[0], target, margin);
//         };
//         ASSERT_TRUE(gradient_same(functor, {A}, 1e-3, 1e-4));
//     }
//     utils::random::reseed();
// }
//
//
// TEST_F(MatrixTests, margin_loss_rowwise) {
//     utils::random::set_seed(100);
//     // we can now extend the range of our random numbers to be beyond
//     // 0 and 1 since sigmoid will clamp them to 0 or 1.
//     EXPERIMENT_REPEAT {
//         auto A = Mat<R>(10, 20, weights<R>::uniform(5.0));
//         R margin = utils::randdouble(0.01, 0.1);
//         uint target = utils::randinteger<uint>(0, A.dims(1) - 1);
//         auto functor = [target, margin](vector<Mat<R>> Xs)-> Mat<R> {
//             return MatOps<R>::margin_loss_rowwise(Xs[0], target, margin);
//         };
//         ASSERT_TRUE(gradient_same(functor, {A}, 1e-3, 1e-4));
//     }
//     utils::random::reseed();
// }
//
//
//
// TEST_F(MatrixTests, norm) {
//     auto functor = [](vector<Mat<R>> Xs)-> Mat<R> {
//         return Xs[0].L2_norm();
//     };
//     EXPERIMENT_REPEAT {
//         auto A = Mat<R>(10, 20, weights<R>::uniform(20.0));
//         ASSERT_TRUE(gradient_same(functor, {A}, 1e-4, DEFAULT_GRAD_EPS, true));
//     }
// }

// TEST_F(MatrixTests, matrix_dot_plus_bias) {
//     auto functor = [](vector<Mat<R>> Xs)-> Mat<R> {
//         auto res = Xs[1].dot(Xs[0]) + Xs[2];
//         return res;
//     };
//     int num_examples = 20;
//     int hidden_size = 10;
//     int input_size = 5;
//     EXPERIMENT_REPEAT {
//         auto X = Mat<R>(num_examples, input_size,   weights<R>::uniform(20.0));
//         auto W = Mat<R>(input_size,   hidden_size,  weights<R>::uniform(2.0));
//         auto bias = Mat<R>(1, hidden_size, weights<R>::uniform(2.0));
//         ASSERT_TRUE(gradient_same(functor, {W, X, bias}, 1e-4));
//     }
// }
//
//
// TEST_F(MatrixTests, matrix_divide_scalar) {
//     EXPERIMENT_REPEAT {
//         auto A = Mat<R>(10, 20, weights<R>::uniform(-20.0, 20.0));
//         auto scalar = (R) utils::randdouble(0.1, 20.0);
//         auto functor = [&scalar](vector<Mat<R>> Xs)-> Mat<R> {
//             return Xs[0] / scalar;
//         };
//         ASSERT_TRUE(gradient_same(functor, {A}, 1e-3, DEFAULT_GRAD_EPS, true));
//     }
// }
//
//
// TEST_F(MatrixTests, divide_inplace_matrix) {
//     EXPERIMENT_REPEAT {
//         auto A = Mat<R>(3, 4, weights<R>::uniform(0.1, 20.0));
//         auto B = Mat<R>(3, 4, weights<R>::uniform(1.0, 2.0));
//
//         auto functor = [&A, &B](vector<Mat<R>>& Xs)-> Mat<R> {
//             auto A_temp = A;
//             auto B_temp = B;
//             A_temp /= B_temp;
//             return A_temp;
//         };
//         ASSERT_TRUE(gradient_same(functor, {A, B}, 1e-3, DEFAULT_GRAD_EPS, true));
//     }
// }
//
//
// TEST_F(MatrixTests, divide_inplace_scalar) {
//     auto functor = [](vector<Mat<R>> Xs)-> Mat<R> {
//         auto temp = Xs[0];
//         temp /= (R)20.0;
//         return (temp - 2.0) ^ 2;
//     };
//     EXPERIMENT_REPEAT {
//         auto A = Mat<R>(3, 4, weights<R>::uniform(0.001, 20.0));
//         ASSERT_TRUE(gradient_same(functor, {A}, 1e-4, DEFAULT_GRAD_EPS, false));
//     }
// }

typedef MemorySafeTest TensorOpsTests;
//
// TEST_F(TensorOpsTests, matrix_mul_add_mul_with_bias_colwise) {
//     auto functor = [](vector<Mat<R>> Xs)-> Mat<R> {
//         return MatOps<R>::mul_add_mul_with_bias_colwise({Xs[0], Xs[2], Xs[4]}, {Xs[1], Xs[3], Xs[5]}, Xs[6]);
//     };
//     int num_examples = 20;
//     int hidden_size = 10;
//     int input_size = 5;
//     int other_input_size = 7;
//     EXPERIMENT_REPEAT {
//         auto W       = Mat<R>(hidden_size,  input_size,         weights<R>::uniform(2.0));
//         auto X       = Mat<R>(input_size,   num_examples,       weights<R>::uniform(20.0));
//
//         auto Wfancy   = Mat<R>(hidden_size, input_size,         weights<R>::uniform(2.0));
//         auto Xfancy   = Mat<R>(input_size,  1,                  weights<R>::uniform(20.0));
//
//
//         auto X_other = Mat<R>(other_input_size, num_examples,      weights<R>::uniform(20.0));
//         auto W_other = Mat<R>(hidden_size,      other_input_size,  weights<R>::uniform(2.0));
//
//         auto bias    = Mat<R>(1,      hidden_size,                 weights<R>::uniform(2.0));
//         ASSERT_TRUE(gradient_same(functor, {W, X, Wfancy, Xfancy, W_other, X_other, bias}, 0.0003));
//     }
// }
//
//
// TEST_F(MatrixTests, log_exp) {
//     EXPERIMENT_REPEAT {
//         graph::NoBackprop nb;
//         auto mat = Mat<R>(10, 10, weights<R>::uniform(0.1, 20.0));
//         auto log_mat = mat.log();
//         auto exp_log_mat = log_mat.exp();
//         #ifdef DALI_USE_CUDA
//         ASSERT_MATRIX_CLOSE(mat, exp_log_mat, 1e-3);
//         #else
//         ASSERT_MATRIX_CLOSE(mat, exp_log_mat, 1e-6);
//         #endif
//     }
// }
//
// TEST_F(MatrixTests, hstack) {
//     {
//         graph::NoBackprop nb;
//
//         Mat<R> a(2, 3);
//         Mat<R> b(2, 4);
//
//         // A:
//         // 0 1 2
//         // 7 8 9
//         // B:
//         // 3  4  5  6
//         // 10 11 12 13
//
//         for (int i = 0; i < 3; i++)
//             a.w(i) = i;
//         for (int i = 0; i < 4; i++)
//             b.w(i) = i + 3;
//         for (int i = 3; i < 6; i++)
//             a.w(i) = i + 4;
//         for (int i = 4; i < 8; i++)
//             b.w(i) = i + 6;
//
//         auto c = MatOps<R>::hstack({a, b});
//         for (int i = 0; i < 14;i++) {
//             ASSERT_EQ(c.w(i), i);
//         }
//     }
//
//     EXPERIMENT_REPEAT {
//         auto mat = Mat<R>(2, 3, weights<R>::uniform(20.0));
//         auto mat2 = Mat<R>(2, 4, weights<R>::uniform(20.0));
//
//         auto functor = [](vector<Mat<R>> Xs)-> Mat<R> {
//             return MatOps<R>::hstack(Xs);
//         };
//         ASSERT_TRUE(gradient_same(functor, {mat, mat2}, 1e-4));
//     }
// }
//
//
// TEST_F(MatrixTests, vstack) {
//     {
//         graph::NoBackprop nb;
//
//         Mat<R> a(2, 3);
//         Mat<R> b(4, 3);
//         // A:
//         // 0 1 2
//         // 1 2 3
//         // B:
//         // 2 3 4
//         // 3 4 5
//         // 4 5 6
//         // 5 6 7
//
//         for (int row=0; row < 2; ++row) {
//             for (int col = 0; col < 3; ++col) {
//                 a.w(row,col) = (R)(row + col);
//             }
//         }
//
//         for (int row=0; row < 4; ++row) {
//             for (int col = 0; col < 3; ++col) {
//                 b.w(row,col) = (R)(row + col + 2);
//             }
//         }
//
//         auto c = MatOps<R>::vstack(a,b);
//
//         for (int row = 0; row < 6; ++row) {
//             for (int col = 0; col < 3; ++col) {
//                 SCOPED_TRACE("Index (" + std::to_string(row) + "," + std::to_string(col) + ")");
//                 ASSERT_NEAR(c.w(row, col), (R)(row + col), 1e-9);
//             }
//         }
//     }
//
//     EXPERIMENT_REPEAT {
//         auto mat = Mat<R>(2, 3, weights<R>::uniform(20.0));
//         auto mat2 = Mat<R>(4, 3, weights<R>::uniform(20.0));
//
//         auto functor = [](vector<Mat<R>> Xs)-> Mat<R> {
//             return MatOps<R>::vstack(Xs);
//         };
//         ASSERT_TRUE(gradient_same(functor, {mat, mat2}, 1e-4));
//     }
// }
//
//
// TEST_F(MatrixTests, abs) {
//     int input_size = 5;
//     int hidden_size = 3;
//
//     EXPERIMENT_REPEAT {
//         auto mat = Mat<R>(input_size, hidden_size, weights<R>::uniform(0.1, 20.0));
//         auto mat2 = Mat<R>(input_size, hidden_size, weights<R>::uniform(-20.0, -0.1));
//
//         auto functor = [](vector<Mat<R>> Xs)-> Mat<R> {
//             return MatOps<R>::hstack(Xs).abs();
//         };
//         ASSERT_TRUE(gradient_same(functor, {mat, mat2}, 1e-4));
//     }
// }
//
//
// TEST_F(TensorOpsTests, dropout) {
//     int seed = 1234;
//     auto functor = [&seed](vector<Mat<R>> Xs)-> Mat<R> {
//         auto C = Xs[0] * Xs[1];
//         utils::random::set_seed(seed);
//         auto D = MatOps<R>::dropout(C, 0.5);
//         utils::random::reseed();
//         auto Z = D + Xs[2];
//         return Z;
//     };
//     int num_examples = 3;
//     int hidden_size = 4;
//     int input_size = 5;
//     EXPERIMENT_REPEAT {
//         seed = utils::randint(0, 2000);
//         auto A = Mat<R>(hidden_size, input_size, weights<R>::uniform(2.0));
//         auto B = Mat<R>(hidden_size, input_size, weights<R>::uniform(20.0));
//         auto C = Mat<R>(1,           input_size, weights<R>::uniform(20.0));
//         ASSERT_TRUE(gradient_same(functor, {A, B, C}, 0.0003));
//     }
// }
//
// TEST_F(TensorOpsTests, dropout_normalized) {
//     int seed = 1234;
//     auto functor = [&seed](vector<Mat<R>> Xs)-> Mat<R> {
//         auto C = Xs[0] * Xs[1];
//         utils::random::set_seed(seed);
//         auto D = MatOps<R>::dropout_normalized(C, 0.5);
//         utils::random::reseed();
//         auto Z = D + Xs[2];
//         return Z;
//     };
//     int num_examples = 3;
//     int hidden_size = 4;
//     int input_size = 5;
//     EXPERIMENT_REPEAT {
//         seed = utils::randint(0, 2000);
//         auto A = Mat<R>(hidden_size, input_size, weights<R>::uniform(2.0));
//         auto B = Mat<R>(hidden_size, input_size, weights<R>::uniform(20.0));
//         auto C = Mat<R>(1,           input_size, weights<R>::uniform(20.0));
//         ASSERT_TRUE(gradient_same(functor, {A, B, C}, 0.0003));
//     }
// }
//
// TEST_F(TensorOpsTests, fast_dropout) {
//     int seed = 1234;
//     auto functor = [&seed](vector<Mat<R>> Xs)-> Mat<R> {
//         auto C = Xs[0] * Xs[1];
//         utils::random::set_seed(seed);
//         auto D = MatOps<R>::fast_dropout(C);
//         utils::random::reseed();
//         auto Z = D + Xs[2];
//         return Z;
//     };
//     int num_examples = 3;
//     int hidden_size = 4;
//     int input_size = 5;
//     EXPERIMENT_REPEAT {
//         seed = utils::randint(0, 2000);
//         auto A = Mat<R>(hidden_size, input_size, weights<R>::uniform(2.0));
//         auto B = Mat<R>(hidden_size, input_size, weights<R>::uniform(20.0));
//         auto C = Mat<R>(1,           input_size, weights<R>::uniform(20.0));
//         ASSERT_TRUE(gradient_same(functor, {A, B, C}, 0.0003));
//     }
// }
//
// TEST_F(TensorOpsTests, softmax_colwise) {
//     int row_size = 5;
//     int col_size = 10;
//     EXPERIMENT_REPEAT {
//         auto A = Mat<R>(row_size, col_size, weights<R>::uniform(-3.0, 3.0));
//         int row = utils::randint(0, row_size - 1);
//         auto functor = [row](vector<Mat<R>> Xs)-> Mat<R>{
//             auto soft = MatOps<R>::softmax_colwise(Xs[0]);
//             //soft.print();
//             auto g = soft[row];
//             //g.print();
//             return g;
//         };
//         ASSERT_TRUE(gradient_same(functor, {A}, 1e-3, 1e-3));
//     }
// }
//
// TEST_F(TensorOpsTests, softmax_rowwise) {
//     int row_size = 5;
//     int col_size = 10;
//     EXPERIMENT_REPEAT {
//         auto A = Mat<R>(row_size, col_size, weights<R>::uniform(-3.0, 3.0));
//         int col = utils::randint(0, col_size - 1);
//         auto functor = [col](vector<Mat<R>> Xs)-> Mat<R>{
//             auto soft = MatOps<R>::softmax_rowwise(Xs[0]);
//             auto g = soft.T()[col];
//             return g;
//         };
//         ASSERT_TRUE(gradient_same(functor, {A}, 1e-3));
//     }
// }
//
// TEST_F(TensorOpsTests, resize_decrease_rows) {
//     int row_size = 3, col_size = 4;
//     // decrease number of rows by 1
//     auto A = Mat<R>(row_size, col_size);
//     for (int i = 0; i < 12; i++) {
//         A.w(i) = i;
//     }
//
//     auto new_shape = mshadow::Shape2(row_size - 1, col_size);
//     A.w().resize(new_shape);
//     for (int i = 0; i < (row_size - 1) * col_size ; i++) {
//         ASSERT_EQ(A.w(i), i);
//     }
//     ASSERT_EQ(A.w().shape, new_shape);
// }
//
// TEST_F(TensorOpsTests, resize_increase_rows) {
//     int row_size = 3, col_size = 4;
//     // increase number of rows by 1
//     auto A = Mat<R>(row_size, col_size);
//     for (int i = 0; i < row_size * col_size; i++) {
//         A.w(i) = i;
//     }
//     auto new_shape = mshadow::Shape2(row_size + 1, col_size);
//     A.w().resize(new_shape, 3.5);
//     for (int i = 0; i < row_size * col_size; i++) {
//         ASSERT_EQ(A.w(i), i);
//     }
//     for (int i = row_size * col_size; i < (row_size + 1) * col_size; i++) {
//         ASSERT_EQ(A.w(i), 3.5);
//     }
//     ASSERT_EQ(A.w().shape, new_shape);
// }
//
// TEST_F(TensorOpsTests, resize_decrease_cols) {
//     int row_size = 3, col_size = 4;
//     // decrease number of columns by 1
//     auto A = Mat<R>(row_size, col_size);
//     for (int i = 0; i < row_size * col_size; i++) {
//         A.w(i) = i;
//     }
//     auto new_shape = mshadow::Shape2(row_size, col_size - 1);
//     A.w().resize(new_shape);
//     for (int i = 0; i < row_size; i++) {
//         for (int j = 0; j < col_size - 1; j++) {
//             ASSERT_EQ(A.w(i,j), i * col_size + j);
//         }
//     }
//     ASSERT_EQ(A.w().shape, new_shape);
// }
//
// TEST_F(TensorOpsTests, resize_increase_cols) {
//     int row_size = 3, col_size = 4;
//     // increase number of columns by 1
//     auto A = Mat<R>(row_size, col_size);
//     for (int i = 0; i < row_size * col_size; i++) {
//         A.w(i) = i;
//     }
//     auto new_shape = mshadow::Shape2(row_size, col_size + 1);
//     A.w().resize(new_shape, 4.2);
//     for (int i = 0; i < row_size; i++) {
//         for (int j = 0; j < col_size; j++) {
//             ASSERT_EQ(A.w(i,j), i * col_size + j);
//         }
//     }
//     for (int i = 0; i < row_size; i++) {
//         for (int j = col_size; j < col_size + 1; j++) {
//             ASSERT_EQ(A.w(i,j), 4.2);
//         }
//     }
//     ASSERT_EQ(A.w().shape, new_shape);
// }
//
// TEST_F(TensorOpsTests, resize_increase_rows_and_cols) {
//     int row_size = 3, col_size = 4;
//     // increase number of rows and columns by 1
//     auto A = Mat<R>(row_size, col_size);
//     for (int i = 0; i < row_size * col_size; i++) {
//         A.w(i) = i;
//     }
//     auto new_shape = mshadow::Shape2(row_size + 1, col_size + 1);
//     A.w().resize(new_shape, 4.2);
//     for (int i = 0; i < row_size; i++) {
//         for (int j = 0; j < col_size; j++) {
//             ASSERT_EQ(A.w(i,j), i * col_size + j);
//         }
//     }
//     for (int i = 0; i < row_size; i++) {
//         for (int j = col_size; j < col_size + 1; j++) {
//             ASSERT_EQ(A.w(i,j), 4.2);
//         }
//     }
//     for (int i = row_size; i < row_size + 1; i++) {
//         for (int j = 0; j < col_size + 1; j++) {
//             ASSERT_EQ(A.w(i,j), 4.2);
//         }
//     }
//     ASSERT_EQ(A.w().shape, new_shape);
// }
//
// TEST_F(TensorOpsTests, resize_decrease_rows_and_cols) {
//     int row_size = 3, col_size = 4;
//     // decrease number of rows and columns by 1
//     auto A = Mat<R>(row_size, col_size);
//     for (int i = 0; i < row_size * col_size; i++) {
//         A.w(i) = i;
//     }
//     auto new_shape = mshadow::Shape2(row_size - 1, col_size - 1);
//     A.w().resize(new_shape, 4.2);
//     for (int i = 0; i < row_size - 1; i++) {
//         for (int j = 0; j < col_size - 1; j++) {
//             ASSERT_EQ(A.w(i,j), i * col_size + j);
//         }
//     }
//     ASSERT_EQ(A.w().shape, new_shape);
// }
//
// TEST_F(TensorOpsTests, resize_1D_decrease_rows) {
//     int row_size = 3;
//     // decrease number of rows by 1
//     TensorInternal<R,1> A(mshadow::Shape1(row_size));
//
//     for (int i = 0; i < row_size; i++) {
//         A(i) = i;
//     }
//
//     auto new_shape = mshadow::Shape1(row_size - 1);
//     A.resize(new_shape);
//     for (int i = 0; i < (row_size - 1); i++) {
//         ASSERT_EQ(A(i), i);
//     }
//     ASSERT_EQ(A.shape, new_shape);
// }
//
// TEST_F(TensorOpsTests, resize_1D_increase_rows) {
//     int row_size = 3;
//     // increase number of rows by 1
//     TensorInternal<R,1> A(mshadow::Shape1(row_size));
//
//     for (int i = 0; i < row_size; i++) {
//         A(i) = i;
//     }
//
//     auto new_shape = mshadow::Shape1(row_size + 1);
//     A.resize(new_shape, 666.0);
//     for (int i = 0; i < (row_size); i++) {
//         ASSERT_EQ(A(i), i);
//     }
//     ASSERT_EQ(A(row_size), 666.0);
//     ASSERT_EQ(A.shape, new_shape);
// }
//
// TEST_F(TensorOpsTests, circular_convolution) {
//     auto functor = [](vector<Mat<R>> Xs)-> Mat<R> {
//         return MatOps<R>::circular_convolution(Xs[0], Xs[1]);
//     };
//     EXPERIMENT_REPEAT {
//         auto matrix = Mat<R>(4, 5, weights<R>::uniform(-20.0, 20.0));
//         auto shift  = Mat<R>(4, 5, weights<R>::uniform(-20.0, 20.0));
//         ASSERT_TRUE(gradient_same(functor, {matrix, shift}, 1e-4));
//     }
// }
//
//
// TEST_F(TensorOpsTests, DISABLED_matrix_conv1d_grad) {
//     auto functor = [](vector<Mat<R>> Xs)-> Mat<R> {
//         return MatOps<R>::conv1d(Xs[0], std::initializer_list<Mat<R>>({Xs[1], Xs[2]})).tanh();
//     };
//     EXPERIMENT_REPEAT {
//         auto kernel1 = Mat<R>(5, 5, weights<R>::uniform(-20.0, 20.0));
//         auto kernel2 = Mat<R>(5, 5, weights<R>::uniform(-20.0, 20.0));
//         auto image = Mat<R>(5, 20, weights<R>::uniform(-20.0, 20.0));
//         ASSERT_TRUE(gradient_same(functor, {image, kernel1, kernel2}, 1e-2));
//     }
// }
//
// TEST_F(TensorOpsTests, DISABLED_matrix_conv2d) {
//     /*graph::NoBackprop nb;
//
//     auto image = Mat<R>(10, 10);
//     int block_width  = 4,
//         block_offset = 3,
//         kernel_width = 3,
//         kernel_height = 3;
//     R filler = 2.0;
//
//     image.w()->w.block(
//         block_offset,
//         block_offset,
//         block_width,
//         block_width).fill(filler);
//
//     auto kernel = Mat<R>(kernel_width, kernel_height);
//
//     kernel = MatOps<R>::fill(kernel, 1);
//
//     auto out = MatOps<R>::conv2d(image, kernel);
//
//     auto expected = Mat<R>(
//         image.dims(0) - kernel.dims(0) + 1,
//         image.dims(1) - kernel.dims(1) + 1);
//
//     expected.w()->w.block(
//         block_offset,
//         block_offset,
//         block_width - kernel_width + 1,
//         block_width - kernel_height + 1).fill(filler);
//
//     ASSERT_EQ( (*out.sum().w())(0), (block_width * block_width * filler)) << "Sum of convolution with image should be sum of image";
//
//     // TODO: test more properties here.
//     ASSERT_TRUE((
//         expected.w()->w.block(
//             block_offset,
//             block_offset,
//             block_width - kernel_width + 1,
//             block_width - kernel_height + 1).array() ==
//         out.w()->w.block(
//             block_offset,
//             block_offset,
//             block_width - kernel_width + 1,
//             block_width - kernel_height + 1).array()).all()) << "Center of kernel activations should match up.";
// */
// }
//
// TEST_F(TensorOpsTests, DISABLED_matrix_conv2d_grad) {
//     auto functor = [](vector<Mat<R>> Xs)-> Mat<R> {
//         return MatOps<R>::conv2d(Xs[0], Xs[1]).tanh();
//     };
//     EXPERIMENT_REPEAT {
//         auto kernel = Mat<R>(5, 5, weights<R>::uniform(-20.0, 20.0));
//         auto image = Mat<R>(8, 8, weights<R>::uniform(-20.0, 20.0));
//         ASSERT_TRUE(gradient_same(functor, {image, kernel}, 1e-4));
//     }
// }
//
// TEST_F(TensorOpsTests, softmax_temperature) {
//     graph::NoBackprop nb;
//
//     auto mat = Mat<R>(10, 1);
//     for (int i = 0; i < 10; i++) mat.w(i) = i;
//
//     auto base_prob = MatOps<R>::softmax_colwise(mat, 1.0);
//
//     auto flat = MatOps<R>::softmax_colwise(
//         MatOps<R>::fill(
//             Mat<R>::empty_like(mat),
//             1.0
//         )
//     );
//
//     auto kl = MatOps<R>::cross_entropy(
//         base_prob,
//         flat
//     ).sum();
//
//     // gets flatter with higher temperature
//     for (int i = 2; i < 11; i++) {
//         R temperature = 1.0 * i;
//         auto new_prob = MatOps<R>::softmax_colwise(
//             mat,
//             temperature
//         );
//         auto new_kl = MatOps<R>::cross_entropy(
//             new_prob,
//             flat
//         ).sum();
//         ASSERT_TRUE(new_kl.w(0) < kl.w(0));
//         kl = new_kl;
//     }
// }
//
// TEST_F(TensorOpsTests, cross_entropy_grad) {
//     EXPERIMENT_REPEAT {
//         const int hidden_size = 10;
//         double temperature = 1.0; // utils::randdouble(0.1, 100);
//         int target = utils::randint(0, hidden_size - 1);
//         auto layer = Mat<R>(hidden_size, 5, weights<R>::uniform(-2.0, 2.0));
//         auto input = Mat<R>(5,  3, weights<R>::uniform(-2.0, 2.0));
//
//         auto functor = [target, temperature](vector<Mat<R>> Xs)-> Mat<R> {
//             auto soft = MatOps<R>::softmax_colwise(
//                     Xs[1].dot(Xs[0]),
//                     temperature
//                 );
//             return MatOps<R>::cross_entropy_colwise(
//                 soft,
//                 target);
//         };
//
//         ASSERT_TRUE(gradient_same(functor, {input, layer}, 1e-2));
//     }
// }
//
// TEST_F(TensorOpsTests, softmax_cross_entropy_colwise_grad) {
//     EXPERIMENT_REPEAT {
//         auto input = Mat<R>(3,  2, weights<R>::uniform(-2.0, 2.0));
//
//         vector<uint> targets;
//         for (int i = 0; i < input.dims(1); ++i)
//             targets.push_back(utils::randint(0, input.dims(0) - 1));
//         Indexing::Index indexed_targets(&targets);
//
//
//         auto functor = [indexed_targets](vector<Mat<R>> Xs)-> Mat<R> {
//             return MatOps<R>::softmax_cross_entropy_colwise(
//                 Xs[0],
//                 indexed_targets);
//         };
//
//         ASSERT_TRUE(gradient_same(functor, {input}, 1e-2));
//     }
// }
//
// TEST_F(TensorOpsTests, cross_entropy_colwise_multiindex) {
//     EXPERIMENT_REPEAT {
//         graph::NoBackprop nb;
//
//         Mat<R> input (3, 5, weights<R>::uniform(0.01, 0.99));
//         Mat<R> softmaxed = MatOps<R>::softmax_colwise(input);
//
//         vector<uint> targets;
//         for (int i = 0; i < input.dims(1); ++i)
//             targets.push_back(utils::randint(0, input.dims(0) - 1));
//         Mat<R> actual_res = MatOps<R>::softmax_cross_entropy_colwise(
//                 input, Indexing::Index(&targets));
//         #ifdef DALI_USE_CUDA
//             EXPECT_TRUE(actual_res.w().memory().gpu_fresh);
//         #endif
//
//         for (int i = 0; i < targets.size(); ++i) {
//             // take each column separately
//             auto expected_res = MatOps<R>::cross_entropy_colwise(softmaxed(NULL,i), targets[i]);
//             ASSERT_NEAR(actual_res.w(i), expected_res.w(0), 1e-4);
//         }
//     }
// }
//
//
//
// TEST_F(TensorOpsTests, softmax_cross_entropy_rowwise_grad) {
//     // utils::random::set_seed(1234);
//     EXPERIMENT_REPEAT {
//         auto input = Mat<R>(2, 3, weights<R>::uniform(-2.0, 2.0));
//
//         vector<uint> targets;
//         for (int i = 0; i < input.dims(0); ++i)
//             targets.push_back(utils::randint(0, input.dims(1) - 1));
//         Indexing::Index indexed_targets(&targets);
//
//
//         auto functor = [indexed_targets](vector<Mat<R>> Xs)-> Mat<R> {
//             return MatOps<R>::softmax_cross_entropy_rowwise(
//                 Xs[0],
//                 indexed_targets);
//         };
//
//         ASSERT_TRUE(gradient_ratio_same(functor, {input}, 1e-2));
//     }
//     // utils::random::reseed();
// }
//
// TEST_F(TensorOpsTests, cross_entropy_rowwise_multiindex) {
//     EXPERIMENT_REPEAT {
//         graph::NoBackprop nb;
//
//         Mat<R> input (5, 3, weights<R>::uniform(0.01, 0.99));
//         Mat<R> softmaxed = MatOps<R>::softmax_rowwise(input);
//
//         vector<uint> targets;
//         for (int i = 0; i < input.dims(0); ++i)
//             targets.push_back(utils::randint(0, input.dims(1) - 1));
//         Mat<R> actual_res = MatOps<R>::softmax_cross_entropy_rowwise(
//                 input, Indexing::Index(&targets));
//         #ifdef DALI_USE_CUDA
//             EXPECT_TRUE(actual_res.w().memory().gpu_fresh);
//         #endif
//
//         for (int i = 0; i < targets.size(); ++i) {
//             // take each column separately
//             auto expected_res = MatOps<R>::cross_entropy_rowwise(softmaxed[i], targets[i]);
//             ASSERT_NEAR(actual_res.w(i), expected_res.w(0), 1e-4);
//         }
//     }
// }
//
// TEST_F(TensorOpsTests, cross_entropy_rowwise_grad) {
//     EXPERIMENT_REPEAT {
//         auto input = Mat<R>(2, 3, weights<R>::uniform(0.01, 1.0));
//
//         auto targets = Mat<int>(2, 1);
//         for (int i = 0; i < input.dims(0); ++i)
//             targets.w(i) = utils::randint(0, input.dims(1) - 1);
//
//         auto functor = [&targets](vector<Mat<R>> Xs)-> Mat<R> {
//             return MatOps<R>::cross_entropy_rowwise(
//                 Xs[0],
//                 targets);
//         };
//
//         ASSERT_TRUE(gradient_same(functor, {input}, 1e-2));
//     }
// }
//
// TEST_F(TensorOpsTests, cross_entropy_colwise_grad) {
//     EXPERIMENT_REPEAT {
//         auto input = Mat<R>(3, 2, weights<R>::uniform(0.01, 1.0));
//
//         auto targets = Mat<int>(2, 1);
//         for (int i = 0; i < input.dims(1); ++i)
//             targets.w(i) = utils::randint(0, input.dims(0) - 1);
//
//         auto functor = [&targets](vector<Mat<R>> Xs)-> Mat<R> {
//             return MatOps<R>::cross_entropy_colwise(
//                 Xs[0],
//                 targets);
//         };
//
//         ASSERT_TRUE(gradient_same(functor, {input}, 1e-2));
//     }
// }
//
// TEST_F(TensorOpsTests, cross_entropy_grad_thought_target) {
//     double temperature;
//     int target;
//
//     EXPERIMENT_REPEAT {
//         const int hidden_size  = 10;
//         const int num_examples = 3;
//         const int input_size   = 5;
//         double temperature = 1.0; // utils::randdouble(0.1, 100);
//
//         auto layer = Mat<R>(hidden_size, input_size,     weights<R>::uniform(-2.0, 2.0));
//         auto input = Mat<R>(input_size,  num_examples,   weights<R>::uniform(-2.0, 2.0));
//
//         auto target = Mat<R>(hidden_size,  num_examples, weights<R>::uniform(0.15, 0.85));
//
//
//         auto functor = [target, temperature](vector<Mat<R>> Xs)-> Mat<R> {
//             auto soft = MatOps<R>::softmax_colwise(
//                     Xs[1].dot(Xs[0]),
//                     temperature
//                 );
//             return MatOps<R>::cross_entropy(
//                 soft,
//                 Xs[2]);
//         };
//
//         ASSERT_TRUE(gradient_same(functor, {input, layer, target}, 1e-1));
//     }
// }
//
// TEST_F(MatrixTests, row_pluck) {
//
//     EXPERIMENT_REPEAT {
//         Mat<R> A(5, 3, weights<R>::uniform(20.0));
//         int row = utils::randint(0, A.dims(0) - 1);
//         auto functor = [row](vector<Mat<R>> Xs) {
//             return Xs[0][row];
//         };
//         ASSERT_TRUE(gradient_same(functor, {A}, 1e-4));
//     }
// }
//
// TEST_F(MatrixTests, col_pluck) {
//     EXPERIMENT_REPEAT {
//         Mat<R> A(5, 3, weights<R>::uniform(20.0));
//         const int col = utils::randint(0, A.dims(1) - 1);
//         auto functor = [col](vector<Mat<R>> Xs) {
//             #ifdef DALI_USE_CUDA
//                 // to ensure op works on gpu we force memory
//                 // freshness of the device
//                 Xs[0].w().memory().to_gpu();
//             #endif
//             auto res = MatOps<R>::col_pluck(Xs[0], col);
//             return res;
//         };
//         ASSERT_TRUE(gradient_same(functor, {A}, 1e-4, 1e-2, true));
//     }
// }
//
// TEST_F(MatrixTests, col_pluck_gpu_vs_cpu) {
//     EXPERIMENT_REPEAT {
//         Mat<R> A(5, 3, weights<R>::uniform(20.0));
//         int col = utils::randint(0, A.dims(1) - 1);
//         auto functor = [col](vector<Mat<R>> Xs) {
//             return MatOps<R>::col_pluck(Xs[0], col);
//         };
//         ASSERT_TRUE(cpu_vs_gpu(functor, {A}, 1e-4));
//     }
// }
//
// TEST_F(MatrixTests, rows_pluck_forward_correctness) {
//
//     EXPERIMENT_REPEAT {
//         graph::NoBackprop nb;
//         const int num_plucks = 4;
//         Mat<R> A(10, 5, weights<R>::uniform(20.0));
//
//         vector<uint> plucks;
//         for (int i = 0; i < num_plucks; ++i) {
//             plucks.push_back(utils::randint(0, A.dims(0) - 1));
//         }
//         auto res = A[&plucks];
//         #ifdef DALI_USE_CUDA
//             EXPECT_TRUE(res.w().memory().gpu_fresh);
//         #endif
//
//         for (int pluck_idx = 0; pluck_idx < plucks.size(); ++pluck_idx) {
//             auto actual_row = res[pluck_idx];
//             auto expected_row = A[plucks[pluck_idx]];
//             EXPECT_MATRIX_CLOSE(actual_row, expected_row, 1e-4);
//         }
//     }
// }
//
// TEST_F(MatrixTests, rows_pluck) {
//     EXPERIMENT_REPEAT {
//         const int num_plucks = 4;
//         Mat<R> A(10, 5, weights<R>::uniform(20.0));
//
//         vector<uint> plucks;
//         for (int i = 0; i < num_plucks; ++i) {
//             plucks.push_back(utils::randint(0, A.dims(0) - 1));
//         }
//
//         auto functor = [&plucks](vector<Mat<R>> Xs) {
//             return Xs[0][&plucks];
//         };
//         ASSERT_TRUE(gradient_same(functor, {A}, 1e-4));
//     }
// }
//
// TEST_F(TensorOpsTests, vector_softmax) {
//     int softmax_size = 10;
//     EXPERIMENT_REPEAT {
//         vector<Mat<R>> matrices;
//         for (int i = 0; i < softmax_size; i++) {
//             matrices.emplace_back(1,1, weights<R>::uniform(0.0, 2.0));
//         }
//         int row = utils::randint(0, softmax_size-1);
//
//         auto functor = [row, softmax_size](vector<Mat<R>> Xs)-> Mat<R> {
//             auto mats = MatOps<R>::softmax(Xs);
//             return mats[row];
//         };
//         ASSERT_TRUE(gradient_same(functor, matrices, 5e-3));
//     }
// }


// void copy_constructor_helper(bool copy_w, bool copy_dw) {
//     Mat<R> original(3,3, weights<R>::uniform(20.0));
//     Mat<R> copy(original, copy_w, copy_dw);

//     copy.w(0,0) += 1.0;
//     copy.dw(0,0) += 1.0;

//     if (copy_w) {
//         ASSERT_MATRIX_NEQ(original, copy);
//     } else {
//         ASSERT_MATRIX_EQ(original, copy);
//     }

//     if (copy_dw) {
//         ASSERT_MATRIX_GRAD_NOT_CLOSE(original, copy, 1e-5);
//     } else {
//         ASSERT_MATRIX_GRAD_CLOSE(original, copy, 1e-5);
//     }

//     copy.w(0,0) -= 1.0;
//     copy.dw(0,0) -= 1.0;
//     ASSERT_MATRIX_GRAD_CLOSE(original, copy, 1e-5);
//     ASSERT_MATRIX_CLOSE(original, copy, 1e-5);
// }


// TEST_F(TensorTests, copy_constructor) {
//     copy_constructor_helper(false, false);
//     copy_constructor_helper(false, true);
//     copy_constructor_helper(true,  false);
//     copy_constructor_helper(true,  true);
// }
//
// TEST_F(MatrixTests, matrix_constant_check) {
//     int num_examples           = 10;
//     int input_size             = 3;
//
//     auto X  = Mat<R>(input_size, num_examples, weights<R>::uniform(20.0));
//     // THE ONLY VARIABLE CONSIDERED CONSTANT IN THIS TEST IS X HERE
//     auto X_const = MatOps<R>::consider_constant(X);
//     auto B = Mat<R>(input_size, num_examples, weights<R>::uniform(20.0));
//     auto error = (((X_const * B) - 2.0) ^ 2).sum();
//     error.grad();
//     graph::backward();
//
//     EXPECT_TRUE(MatOps<R>::grad_allclose(X, Mat<R>::zeros_like(X), 1e-9));
//     EXPECT_FALSE(MatOps<R>::grad_allclose(B, Mat<R>::zeros_like(B), 1e-9));
//     // HERE X IS NO LONGER CONST
//     X = Mat<R>(input_size, num_examples, weights<R>::uniform(20.0));
//     B = Mat<R>(input_size, num_examples, weights<R>::uniform(20.0));
//     error = (((X * B) - 2.0) ^ 2).sum();
//     error.grad();
//     graph::backward();
//
//     EXPECT_FALSE(MatOps<R>::grad_allclose(X, Mat<R>::zeros_like(X), 1e-9));
//     EXPECT_FALSE(MatOps<R>::grad_allclose(B, Mat<R>::zeros_like(B), 1e-9));
// }
//
TEST_F(TensorTests, scalar_pow) {
    int height = 3;
    int width = 4;

    EXPERIMENT_REPEAT {
        Tensor mat({height, width}, initializer::uniform(1.0, 2.0), DTYPE_DOUBLE);
        R exponent = utils::randdouble(0.4, 2.5);

        auto functor = [exponent](vector<Tensor> Xs)-> Tensor {
            return Xs[0] ^ exponent;
        };
        ASSERT_TRUE(gradient_same(functor, {mat}, 1e-3));
    }
}

TEST_F(TensorTests, pow) {
    int height = 3;
    int width = 4;

    EXPERIMENT_REPEAT {

        Tensor mat({height, width}, initializer::uniform(0.5, 1.5), DTYPE_DOUBLE);
        Tensor exponent({}, DTYPE_DOUBLE);
        exponent.w = 2.4;

        exponent = exponent.broadcast_scalar_to_ndim(mat.ndim());

        auto functor = [](vector<Tensor> Xs)-> Tensor {
            return Xs[0] ^ Xs[1];
        };
        ASSERT_TRUE(gradient_same(functor, {mat, exponent}, 1e-3));
    }
}

TEST_F(TensorTests, DISABLED_quadratic_form) {
    int left_size = 2;
    int right_size = 3;
    int left_size_outer = 4;
    int right_size_outer = 5;

    EXPERIMENT_REPEAT {
        Tensor left({left_size, left_size_outer}, initializer::uniform(-20.0, 20.0));
        Tensor middle({left_size, right_size}, initializer::uniform(-20.0, 20.0));
        Tensor right({right_size, right_size_outer}, initializer::uniform(-20.0, 20.0));

        auto functor = [](vector<Tensor> Xs)-> Tensor {
            return tensor_ops::quadratic_form(Xs[0], Xs[1], Xs[2]);
        };
        ASSERT_TRUE(gradient_same(functor, {left, middle, right}, 1e-3));
    }
}

typedef std::function<std::shared_ptr<solver::AbstractSolver>(vector<Tensor>)> create_solver_t;

void test_solver(create_solver_t create_solver) {
    // minimize X.T() * W * X + W2 * X;
    Tensor X({5, 1}, initializer::uniform(-20.0, 20.0));
    X = tensor_ops::consider_constant(X);
    Tensor W({5, 5}, initializer::uniform(-20.0, 20.0));
    Tensor W2({1, 5}, initializer::uniform(-20.0, 20.0));

    W = W.dot(W.transpose()); // ensure positive definite.

    vector<Tensor> params({W, W2});
    auto solver = create_solver(params);

    int solver_iterations = 10;

    float last_error;

    for (int iter = 0; iter < solver_iterations; ++iter) {
        auto error = tensor_ops::quadratic_form(X, W, X) + W2.dot(X);
        error.grad();
        graph::backward();
        solver->step(params);

        if (iter > 1) { // some solvers need an epoch to start up.
            ASSERT_LT((float)error.w(0) + 1e-5, last_error);
        }
        last_error = (float)error.w(0);
    }
}

void test_solver_trivial(create_solver_t create_solver) {
    utils::random::set_seed(5000);
    // minimize norm of X
    Tensor X({5}, initializer::uniform(-20.0, 20.0));
    vector<Tensor> params({X});
    auto solver = create_solver(params);
    int solver_iterations = 200;
    float last_error;
    EXPECT_LT(10.0, (double) X.L2_norm().w(0));
    for (int iter = 0; iter < solver_iterations; ++iter) {
        auto error = X.L2_norm();
        error.grad();
        graph::backward();
        solver->step(params);
        last_error = (float)error.w(0);
        if (last_error < 0.1) {
            break;
        }
    }
    EXPECT_GT(0.1, last_error);
}

TEST(Solver, trivial_sgd) {
    test_solver_trivial([](vector<Tensor> params) {
        auto ret = std::make_shared<solver::SGD>(params);
        ret->step_size = 0.5;
        ret->clip_norm = 0;
        return ret;
    });
}

TEST(Solver, DISABLED_sgd) {
    test_solver([](vector<Tensor> params) {
        auto ret = std::make_shared<solver::SGD>(params);
        ret->step_size = 0.01;
        return ret;
    });
}

TEST(Solver, trivial_adagrad) {
    test_solver_trivial([](vector<Tensor> params) {
        auto ret = std::make_shared<solver::AdaGrad>(params);
        ret->step_size = 1;
        ret->clip_norm = 0.0;
        return ret;
    });
}

TEST(Solver, trivial_rmsprop) {
    test_solver_trivial([](vector<Tensor> params) {
        auto ret = std::make_shared<solver::RMSProp>(params);
        ret->step_size = 0.1;
        ret->clip_norm = 0.0;
        return ret;
    });
}

TEST(Solver, trivial_rmspropmomentum) {
    test_solver_trivial([](vector<Tensor> params) {
        auto ret = std::make_shared<solver::RMSPropMomentum>(params);
        ret->step_size = 0.5;
        ret->momentum = 0.2;
        // ret->clip_norm = 0.0;
        return ret;
    });
}

TEST(Solver, trivial_adadelta) {
    test_solver_trivial([](vector<Tensor> params) {
        auto ret = std::make_shared<solver::AdaDelta>(params);
        ret->clip_norm = 0.0;
        ret->rho = 0.1;
        return ret;
    });
}

TEST(Solver, trivial_adam) {
    test_solver_trivial([](vector<Tensor> params) {
        auto ret = std::make_shared<solver::Adam>(params);
        ret->step_size = 0.005;
        return ret;
    });
}
//
// TEST(Solver, adagrad) {
//     test_solver([](vector<Mat<R>> params) {
//         auto ret = std::make_shared<Solver::AdaGrad<R>>(params);
//         ret->step_size = 0.01;
//         return ret;
//     });
// }
//
// TEST(Solver, rmsprop) {
//     test_solver([](vector<Mat<R>> params) {
//         auto ret = std::make_shared<Solver::RMSProp<R>>(params);
//         ret->step_size = 0.1;
//         return ret;
//     });
// }
//
// TEST(Solver, rmspropmomentum) {
//     test_solver([](vector<Mat<R>> params) {
//         auto ret = std::make_shared<Solver::RMSPropMomentum<R>>(params);
//         // ret->step_size = 0.001;
//         return ret;
//     });
// }
//
// TEST(Solver, adadelta) {
//     test_solver([](vector<Mat<R>> params) {
//         auto ret = std::make_shared<Solver::AdaDelta<R>>(params);
//         return ret;
//     });
// }
//
// TEST(Solver, adam) {
//     test_solver([](vector<Mat<R>> params) {
//         auto ret = std::make_shared<Solver::Adam<R>>(params);
//         return ret;
//     });
// }
//
// Mat<R> create_dataset() {
//     int num_points     = 20;
//     int num_dimensions = 5;
//     // create data
//     auto dataset = Mat<R>();
//     {
//         graph::NoBackprop nb;
//         auto pointsA = Mat<R>(
//             num_dimensions,
//             num_points,
//             weights<R>::gaussian(0.0, 2.0)
//         );
//         auto pointsB = Mat<R>(
//             num_dimensions,
//             num_points,
//             weights<R>::gaussian(0.0, 2.0)
//         );
//         auto point = Mat<R>(num_dimensions, 1);
//         for (int i = 0; i < num_dimensions; i++)
//             point.w(i) = 2;
//         pointsA += point;
//
//         for (int i = 0; i < num_dimensions; i++)
//             point.w(i) = -2;
//         pointsB += point;
//         dataset = MatOps<R>::hstack({pointsA, pointsB});
//     }
//     return dataset;
// }
//
//
// void test_solver_optimization(std::string solvername) {
//     utils::random::set_seed(1234);
//     int num_points = 20;
//     int num_dimensions = 5;
//     // create data
//     auto dataset = Mat<R>();
//     {
//         graph::NoBackprop nb;
//         auto pointsA = Mat<R>(
//             num_points,
//             num_dimensions,
//             weights<R>::gaussian(0.0, 2.0)
//         );
//         auto pointsB = Mat<R>(
//             num_points,
//             num_dimensions,
//             weights<R>::gaussian(0.0, 2.0)
//         );
//         auto point = Mat<R>(1, num_dimensions);
//         for (int i = 0; i < num_dimensions; i++)
//             point.w(i) = 2;
//         pointsA += point;
//
//         for (int i = 0; i < num_dimensions; i++)
//             point.w(i) = -2;
//         pointsB += point;
//         dataset = MatOps<R>::vstack({pointsA, pointsB});
//     }
//
//     int num_classes = 2;
//     auto mat = Mat<R>(num_dimensions, num_classes, weights<R>::uniform(2.0));
//     auto bias = Mat<R>(1,             num_classes, weights<R>::uniform(2.0));
//     auto params = vector<Mat<R>>({mat, bias});
//
//     auto solver = Solver::construct(solvername, params, 0.1);
//     auto labels = vector<uint>();
//     for (int i = 0; i < num_points * 2; i++)
//         labels.emplace_back(i < num_points ? 0 : 1);
//
//     R original_error = 0;
//     {
//         graph::NoBackprop nb;
//         auto mat_err = MatOps<R>::softmax_cross_entropy_rowwise((dataset.dot(mat) + bias), &labels).sum();
//         original_error = mat_err.w(0);
//     }
//     R error = original_error;
//     for (int e = 0; e < 100; e++) {
//         auto KL = MatOps<R>::softmax_cross_entropy_rowwise((dataset.dot(mat) + bias), &labels).sum();
//         KL.grad();
//         graph::backward();
//         solver->step(params);
//         error = KL.w(0);
//     }
//     // make 10x improvements (or else no VC funding)
//     ASSERT_PRED_FORMAT2(SCALAR_COMP_LE, error, original_error / 10.0);
// }
//
// TEST(Solver, adagrad_optimization_test) {
//     test_solver_optimization("adagrad");
// }
// TEST(Solver, sgd_optimization_test) {
//     test_solver_optimization("sgd");
// }
// TEST(Solver, adadelta_optimization_test) {
//     test_solver_optimization("adadelta");
// }
// TEST(Solver, adam_optimization_test) {
//     test_solver_optimization("adam");
// }
// TEST(Solver, rmsprop_optimization_test) {
//     test_solver_optimization("rmsprop");
// }
// TEST(Solver, rmspropmomentum_optimization_test) {
//     test_solver_optimization("RMSPropMomentum");
// }
