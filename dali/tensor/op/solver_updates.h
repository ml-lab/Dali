#ifndef DALI_TENSOR_OP_SOLVER_UPDATES_H
#define DALI_TENSOR_OP_SOLVER_UPDATES_H

#include "dali/tensor/Mat.h"
#include "dali/tensor/Tape.h"
#include "dali/utils.h"

template<typename R> class Mat;

namespace matops {
    template<typename R>
    struct SolverUpdates {
        static void clip_and_regularize(Mat<R> param, R clipval, R regc);


        static void sgd_update(Mat<R> matrix, R step_size);
        static void adagrad_update(Mat<R> matrix,
                                   TensorInternal<R, 1>& cache,
                                   R step_size,
                                   R smooth_eps);

        static void rmsprop_update(Mat<R> param, TensorInternal<R,1>& cache,
                R decay_rate, R step_size, R smooth_eps);

        static void adadelta_update(Mat<R> param,
                                    TensorInternal<R,1>& gsum,
                                    TensorInternal<R,1>& xsum,
                                    R rho,
                                    R smooth_eps);

        static void adam_update(Mat<R> param,
                                TensorInternal<R,1>& m,
                                TensorInternal<R,1>& v,
                                R b1,
                                R b2,
                                R smooth_eps,
                                R step_size,
                                unsigned long long epoch);
    };
}

#endif
