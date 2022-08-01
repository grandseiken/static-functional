#ifndef STATIC_FUNCTIONAL_INCLUDE_SFN_FUNCTIONAL_H
#define STATIC_FUNCTIONAL_INCLUDE_SFN_FUNCTIONAL_H
#include <sfn/type_list.h>
#include <functional>
#include <type_traits>

namespace sfn {
//-------------------------------------------------------------------------------------------------
// function_traits
//-------------------------------------------------------------------------------------------------
namespace detail {
struct function_traits_not_impl {
  inline static constexpr bool is_function = false;
  inline static constexpr bool is_function_ptr = false;
  inline static constexpr bool is_function_ref = false;
  inline static constexpr bool is_member_function_ptr = false;
};

template <bool IsType, bool IsPtr, bool IsRef, bool IsMemPtr, typename R, typename... Args>
struct function_traits_impl {
  using function_type = R(Args...);
  using return_type = R;
  using parameter_types = list<Args...>;
  inline static constexpr bool is_function_type = IsType;
  inline static constexpr bool is_function_ptr = IsPtr;
  inline static constexpr bool is_function_ref = IsRef;
  inline static constexpr bool is_member_function_ptr = IsMemPtr;
};
}  // namespace detail

template <typename>
struct function_traits : detail::function_traits_not_impl {};
template <typename R, typename... Args>
struct function_traits<R(Args...)>
: detail::function_traits_impl<true, false, false, false, R, Args...> {};
template <typename R, typename... Args>
struct function_traits<R (*)(Args...)>
: detail::function_traits_impl<false, true, false, false, R, Args...> {};
template <typename R, typename... Args>
struct function_traits<R (*const)(Args...)>
: detail::function_traits_impl<false, true, false, false, R, Args...> {};
template <typename R, typename... Args>
struct function_traits<R (&)(Args...)>
: detail::function_traits_impl<false, false, true, false, R, Args...> {};
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)>
: detail::function_traits_impl<false, false, false, true, R, C&, Args...> {};
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const>
: detail::function_traits_impl<false, false, false, true, R, const C&, Args...> {};

//-------------------------------------------------------------------------------------------------
// concepts & signatures
//-------------------------------------------------------------------------------------------------
template <typename T>
concept member_function = function_traits<T>::is_member_function_ptr;
template <typename T>
concept function = function_traits<T>::is_function_ptr || function_traits<T>::is_function_ref ||
    member_function<T>;
template <typename T>
concept function_type = function_traits<T>::is_function_type;
template <typename T>
concept functional = function<T> || function_type<T>;

template <function_type T>
using ptr = T*;
template <functional T>
using function_type_of = typename function_traits<T>::function_type;
template <functional T>
using return_type_of = typename function_traits<T>::return_type;
template <functional T>
using parameter_types_of = typename function_traits<T>::parameter_types;

//-------------------------------------------------------------------------------------------------
// unwrap
//-------------------------------------------------------------------------------------------------
namespace detail {
template <typename T>
constexpr decltype(auto) maybe_move(typename std::remove_reference_t<T>& v) {
  if constexpr (!std::is_const_v<std::remove_reference_t<T>> && !std::is_lvalue_reference_v<T>) {
    return std::move(v);
  } else {
    return v;
  }
}

template <member_function auto F, type_list>
struct unwrap_f;
template <member_function auto F, typename... Args>
struct unwrap_f<F, list<Args...>> {
  static inline constexpr decltype(auto) f(Args... args) {
    return std::invoke(F, maybe_move<Args>(args)...);
  }
};
template <function auto F, bool MemPtr = member_function<decltype(F)>>
struct unwrap_if {
  static inline constexpr auto value = F;
};
template <function auto F>
struct unwrap_if<F, true> {
  static inline constexpr auto value = &unwrap_f<F, parameter_types_of<decltype(F)>>::f;
};
}  // namespace detail

template <function auto F>
inline constexpr auto unwrap = detail::unwrap_if<F>::value;

//-------------------------------------------------------------------------------------------------
// sequence
//-------------------------------------------------------------------------------------------------
namespace detail {
template <typename, function auto... F>
struct sequence_f;
template <typename... Args, function auto... F>
struct sequence_f<list<Args...>, F...> {
  inline static constexpr decltype(auto) f(Args... args) {
    return (F(args...), ...);
  }
};
}  // namespace detail

template <typename T, typename... Rest>
concept sequencable = functional<T> &&(functional<Rest>&&...) &&
    (std::is_same_v<parameter_types_of<T>, parameter_types_of<Rest>> && ...) &&
    (sizeof...(Rest) == 0u ||
     (none_of<parameter_types_of<T>, std::is_rvalue_reference> &&
      all_of<parameter_types_of<T>, std::is_copy_constructible>));

template <function auto F, function auto... Rest>
requires sequencable<decltype(F), decltype(Rest)...>
inline constexpr auto sequence =
    &detail::sequence_f<parameter_types_of<decltype(F)>, unwrap<F>, unwrap<Rest>...>::f;

template <function auto F>
requires sequencable<decltype(F)>
inline constexpr auto sequence<F> = F;

//-------------------------------------------------------------------------------------------------
// cast
//-------------------------------------------------------------------------------------------------
namespace detail {
template <typename T>
using is_default_constructible = std::is_constructible<T>;
template <typename... Ts, typename... Args>
constexpr bool all_constructible(list<Ts...>, list<Args...>) {
  return (std::is_constructible_v<Ts, Args> && ...);
}

template <function auto F, typename R, type_list, type_list, type_list, type_list>
struct cast_f;
template <function auto F, typename R, typename... UsedSourceArgs, typename... UnusedSourceArgs,
          typename... UsedTargetArgs, typename... UnusedTargetArgs>
struct cast_f<F, R, list<UsedSourceArgs...>, list<UnusedSourceArgs...>, list<UsedTargetArgs...>,
              list<UnusedTargetArgs...>> {
  inline static constexpr decltype(auto) f(UsedTargetArgs... args, UnusedTargetArgs...) {
    return R(F(UsedSourceArgs(maybe_move<UsedTargetArgs>(args))..., UnusedSourceArgs{}...));
  }
};

template <functional Source, functional Target>
struct cast_impl {
  using source_args = parameter_types_of<Source>;
  using target_args = parameter_types_of<Target>;
  inline static constexpr auto n =
      size<source_args> < size<target_args> ? size<source_args> : size<target_args>;
  using used_source_args = sublist<source_args, 0u, n>;
  using used_target_args = sublist<target_args, 0u, n>;
  using unused_source_args = sublist<source_args, n>;
  using unused_target_args = sublist<target_args, n>;

  inline static constexpr bool return_type_convertible =
      std::is_same_v<return_type_of<Target>, void> ||
      std::is_constructible_v<return_type_of<Target>, return_type_of<Source>>;
  inline static constexpr bool matching_parameters_convertible =
      all_constructible(used_source_args{}, used_target_args{});
  inline static constexpr bool missing_parameters_default_constructible =
      all_of<unused_source_args, is_default_constructible>;

  template <function auto F>
  inline static constexpr auto cast =
      &cast_f<F, return_type_of<Target>, used_source_args, unused_source_args, used_target_args,
              unused_target_args>::f;
};

template <function_type T, function auto F,
          bool IsSame = std::is_same_v<T, function_type_of<decltype(F)>>>
struct cast_if {
  static inline constexpr auto value = F;
};
template <function_type T, function auto F>
struct cast_if<T, F, false> {
  static inline constexpr auto value = cast_impl<decltype(F), T>::template cast<F>;
};
}  // namespace detail

template <typename Source, typename Target>
concept castable_to = functional<Source> && functional<Target> &&
    detail::cast_impl<Source, Target>::return_type_convertible &&
    detail::cast_impl<Source, Target>::matching_parameters_convertible &&
    detail::cast_impl<Source, Target>::missing_parameters_default_constructible;

template <function_type T, function auto F>
requires castable_to<decltype(F), T>
inline constexpr auto cast = detail::cast_if<T, unwrap<F>>::value;

//-------------------------------------------------------------------------------------------------
// bind_front / bind_back
//-------------------------------------------------------------------------------------------------
namespace detail {
template <bool Front, function auto F, type_list, type_list, auto...>
struct bind_f;
template <bool Front, function auto F, typename... BoundArgs, typename... UnboundArgs,
          auto... Values>
struct bind_f<Front, F, list<BoundArgs...>, list<UnboundArgs...>, Values...> {
  inline static constexpr decltype(auto) f(UnboundArgs... args) {
    if constexpr (Front) {
      return F(BoundArgs(Values)..., maybe_move<UnboundArgs>(args)...);
    } else {
      return F(maybe_move<UnboundArgs>(args)..., BoundArgs(Values)...);
    }
  }
};

template <bool Front, functional FunctionType, typename... Args>
struct bind_impl {
  using args = parameter_types_of<FunctionType>;
  using bound_args = std::conditional_t<Front, sublist<args, 0u, sizeof...(Args)>,
                                        sublist<args, size<args> - sizeof...(Args)>>;
  using unbound_args = std::conditional_t<Front, sublist<args, sizeof...(Args)>,
                                          sublist<args, 0u, size<args> - sizeof...(Args)>>;

  inline static constexpr bool parameters_convertible =
      all_constructible(bound_args{}, list<const Args&...>{});

  template <function auto F, auto... Values>
  inline static constexpr auto bind = &bind_f<Front, F, bound_args, unbound_args, Values...>::f;
};

template <bool Front, function auto F, auto... Values>
struct bind_if {
  static inline constexpr auto value =
      bind_impl<Front, decltype(F), decltype(Values)...>::template bind<F, Values...>;
};
template <bool Front, function auto F>
struct bind_if<Front, F> {
  static inline constexpr auto value = F;
};
}  // namespace detail

template <typename F, typename... Args>
concept bindable_front = functional<F> && sizeof...(Args) <=
    size<parameter_types_of<F>>&& detail::bind_impl<true, F, Args...>::parameters_convertible;

template <typename F, typename... Args>
concept bindable_back = functional<F> && sizeof...(Args) <=
    size<parameter_types_of<F>>&& detail::bind_impl<false, F, Args...>::parameters_convertible;

template <function auto F, auto... Values>
requires bindable_front<decltype(F), decltype(Values)...>
inline constexpr auto bind_front = detail::bind_if<true, unwrap<F>, Values...>::value;

template <function auto F, auto... Values>
requires bindable_back<decltype(F), decltype(Values)...>
inline constexpr auto bind_back = detail::bind_if<false, unwrap<F>, Values...>::value;

//-------------------------------------------------------------------------------------------------
// compose / compose_front / compose_back
//-------------------------------------------------------------------------------------------------
namespace detail {
template <type_list, type_list, typename, function auto G, function auto F>
struct compose_front_f;
template <typename... GArgs, typename... FArgs, typename ComposedArg, function auto G,
          function auto F>
struct compose_front_f<list<GArgs...>, list<FArgs...>, ComposedArg, G, F> {
  inline static constexpr decltype(auto) f(FArgs... fargs, GArgs... gargs) {
    return G(ComposedArg(F(maybe_move<FArgs>(fargs)...)), maybe_move<GArgs>(gargs)...);
  }
};

template <type_list, type_list, typename, function auto G, function auto F>
struct compose_back_f;
template <typename... GArgs, typename... FArgs, typename ComposedArg, function auto G,
          function auto F>
struct compose_back_f<list<GArgs...>, list<FArgs...>, ComposedArg, G, F> {
  inline static constexpr decltype(auto) f(GArgs... gargs, FArgs... fargs) {
    return G(maybe_move<GArgs>(gargs)..., ComposedArg(F(maybe_move<FArgs>(fargs)...)));
  }
};

template <functional GType, functional FType>
struct compose_front_impl {
  using fargs = parameter_types_of<FType>;
  using gargs = drop_front<parameter_types_of<GType>>;
  using composed_arg = front<parameter_types_of<GType>>;

  inline static constexpr bool type_convertible =
      std::is_constructible_v<composed_arg, return_type_of<FType>>;

  template <function auto G, function auto F>
  inline static constexpr auto compose = &compose_front_f<gargs, fargs, composed_arg, G, F>::f;
};

template <functional GType, functional FType>
struct compose_back_impl {
  using fargs = parameter_types_of<FType>;
  using gargs = drop_back<parameter_types_of<GType>>;
  using composed_arg = back<parameter_types_of<GType>>;

  inline static constexpr bool type_convertible =
      std::is_constructible_v<composed_arg, return_type_of<FType>>;

  template <function auto G, function auto F>
  inline static constexpr auto compose = &compose_back_f<gargs, fargs, composed_arg, G, F>::f;
};
}  // namespace detail

template <typename G, typename F>
concept composable_front = functional<G> && functional<F> &&
    1u <= size<parameter_types_of<G>>&& detail::compose_front_impl<G, F>::type_convertible;

template <typename G, typename F>
concept composable_back = functional<G> && functional<F> &&
    1u <= size<parameter_types_of<G>>&& detail::compose_back_impl<G, F>::type_convertible;

template <typename G, typename F>
concept composable = composable_front<G, F> && 1u == size<parameter_types_of<G>>;

template <function auto G, function auto F>
requires composable_front<decltype(G), decltype(F)>
inline constexpr auto compose_front =
    detail::compose_front_impl<decltype(G), decltype(F)>::template compose<unwrap<G>, unwrap<F>>;

template <function auto G, function auto F>
requires composable_back<decltype(G), decltype(F)>
inline constexpr auto compose_back =
    detail::compose_back_impl<decltype(G), decltype(F)>::template compose<unwrap<G>, unwrap<F>>;

template <function auto G, function auto F>
requires composable<decltype(G), decltype(F)>
inline constexpr auto compose = compose_front<G, F>;

}  // namespace sfn

#endif
