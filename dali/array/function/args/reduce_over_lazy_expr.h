#ifndef DALI_ARRAY_FUNCTION_ARGS_REDUCE_OVER_LAZY_EXPR_H
#define DALI_ARRAY_FUNCTION_ARGS_REDUCE_OVER_LAZY_EXPR_H

#include <tuple>

#include "dali/array/function/expression.h"

template<template<class>class Functor, typename LeftT, typename RightT>
struct LazyBinary;

template<template<class>class Functor, typename ExprT>
struct LazyUnary;

template<class Functor, typename ExprT>
struct LazyAllReducer;

template<class Functor, typename ExprT, bool return_indices>
struct LazyAxisReducer;

namespace internal {
    template<typename ExprT>
    struct NonRecursiveLazySumAxis;
}  // namespace internal

template<typename Reducer>
struct ReduceOverLazyExpr {
    typedef std::tuple<typename Reducer::outtype_t, typename Reducer::state_t> outtuple_t;

    template<template<class>class Functor, typename LeftT, typename RightT, typename... Args>
    static outtuple_t unfold_helper(
            const outtuple_t& state,
            const LazyBinary<Functor, LeftT,RightT>& binary_expr,
            const Args&... args) {
        return unfold_helper(state, binary_expr.left, binary_expr.right, args...);
    }

    template<typename ExprT, typename... Args>
    static outtuple_t unfold_helper(
            const outtuple_t& state,
            const internal::NonRecursiveLazySumAxis<ExprT>& reducer_expr,
            const Args&... args) {
        return unfold_helper(state, reducer_expr.expr, args...);
    }

    template<template<class>class Functor, typename ExprT, typename... Args>
    static outtuple_t unfold_helper(
            const outtuple_t& state,
            const LazyUnary<Functor,ExprT>& elementwise_expr,
            const Args&... args) {
        return unfold_helper(state, elementwise_expr.expr, args...);
    }

    template<class Functor, typename ExprT, typename... Args>
    static outtuple_t unfold_helper(
            const outtuple_t& state,
            const LazyAllReducer<Functor,ExprT>& reducer_expr,
            const Args&... args) {
        return unfold_helper(state, reducer_expr.expr, args...);
    }

    template<class Functor, typename ExprT, bool return_indices, typename... Args>
    static outtuple_t unfold_helper(
            const outtuple_t& state,
            const LazyAxisReducer<Functor,ExprT,return_indices>& reducer_expr,
            const Args&... args) {
        return unfold_helper(state, reducer_expr.expr, args...);
    }

    template<typename T, typename... Args>
    static outtuple_t unfold_helper(const outtuple_t& state, const T& arg, const Args&... args) {
        static_assert(!std::is_base_of<LazyExpType,T>::value,
                "Every lazy expression needs to have a corresponding `unfold_helper` method in " __FILE__
                "Did you add an new lazy expression and forget to implement `unfold_helper`?");
        return unfold_helper(Reducer::reduce_step(state, arg), args...);
    }

    static outtuple_t unfold_helper(const outtuple_t& state) {
        return state;
    }

    template<typename... Args>
    static typename Reducer::outtype_t reduce(const Args&... args) {
        auto initial_tuple = outtuple_t();
        return std::get<0>(unfold_helper(initial_tuple, args...));
    }
};

#endif // DALI_ARRAY_FUNCTION_ARGS_REDUCE_OVER_LAZY_EXPR_H