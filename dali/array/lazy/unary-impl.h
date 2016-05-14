#include "dali/array/function/lazy_function.h"
#include "dali/array/TensorFunctions.h"

template<template<class>class Functor, typename ExprT>
struct LazyUnary : public LazyFunction<LazyUnary<Functor,ExprT>, ExprT> {
    ExprT expr;

    LazyUnary(ExprT expr_) : LazyFunction<LazyUnary<Functor,ExprT>, ExprT>(expr_),
                             expr(expr_) {
    }

    template<int devT,typename T>
    auto to_mshadow_expr(memory::Device device, const std::vector<int>& output_shape) const ->
            decltype(
                mshadow::expr::F<Functor<T>>(
                     MshadowWrapper<devT,T,ExprT>::wrap(expr, device, output_shape)
                )
            ) {
        auto left_expr = MshadowWrapper<devT,T,ExprT>::wrap(expr, device, output_shape);
        return mshadow::expr::F<Functor<T>>(left_expr);
    }
};

namespace lazy {
    template<template<class>class Functor, typename ExprT>
    LazyUnary<Functor,ExprT> F(const Exp<ExprT>& expr) {
        return LazyUnary<Functor,ExprT>(expr.self());
    }

    template<typename ExprT>
    LazyUnary<tensor_ops::op::identity,ExprT> identity(const Exp<ExprT>& expr) {
        return LazyUnary<tensor_ops::op::identity,ExprT>(expr.self());
    }

    template<typename ExprT>
    LazyUnary<tensor_ops::op::sigmoid,ExprT> sigmoid(const Exp<ExprT>& expr) {
        return LazyUnary<tensor_ops::op::sigmoid,ExprT>(expr.self());
    }

    template<typename ExprT>
    LazyUnary<tensor_ops::op::tanh,ExprT> tanh(const Exp<ExprT>& expr) {
        return LazyUnary<tensor_ops::op::tanh,ExprT>(expr.self());
    }

    template<typename ExprT>
    LazyUnary<tensor_ops::op::relu,ExprT> relu(const Exp<ExprT>& expr) {
        return LazyUnary<tensor_ops::op::relu,ExprT>(expr.self());
    }

    template<typename ExprT>
    LazyUnary<tensor_ops::op::log_or_zero,ExprT> log_or_zero(const Exp<ExprT>& expr) {
        return LazyUnary<tensor_ops::op::log_or_zero,ExprT>(expr.self());
    }

    template<typename ExprT>
    LazyUnary<tensor_ops::op::abs,ExprT> abs(const Exp<ExprT>& expr) {
        return LazyUnary<tensor_ops::op::abs,ExprT>(expr.self());
    }

    template<typename ExprT>
    LazyUnary<tensor_ops::op::sign,ExprT> sign(const Exp<ExprT>& expr) {
        return LazyUnary<tensor_ops::op::sign,ExprT>(expr.self());
    }
}  // namespace lazy
