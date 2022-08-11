#ifndef STATIC_FUNCTIONAL_INCLUDE_SFN_FUNCTIONAL_H
#define STATIC_FUNCTIONAL_INCLUDE_SFN_FUNCTIONAL_H
#include <sfn/type_list.h>
#include <type_traits>
#include <utility>

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

template <bool IsType, bool IsPtr, bool IsRef, bool IsMemPtr, bool IsNoexcept, typename R,
          typename... Args>
struct function_traits_impl {
  inline static constexpr bool is_function_type = IsType;
  inline static constexpr bool is_function_ptr = IsPtr;
  inline static constexpr bool is_function_ref = IsRef;
  inline static constexpr bool is_member_function_ptr = IsMemPtr;
  inline static constexpr bool is_noexcept = IsNoexcept;
  using function_type = R(Args...);
  using return_type = R;
  using parameter_types = list<Args...>;
};
}  // namespace detail

template <typename>
struct function_traits : detail::function_traits_not_impl {};
template <typename R, typename... Args>
struct function_traits<R(Args...)>
: detail::function_traits_impl<true, false, false, false, false, R, Args...> {};
template <typename R, typename... Args>
struct function_traits<R (*)(Args...)>
: detail::function_traits_impl<false, true, false, false, false, R, Args...> {};
template <typename R, typename... Args>
struct function_traits<R (*const)(Args...)>
: detail::function_traits_impl<false, true, false, false, false, R, Args...> {};
template <typename R, typename... Args>
struct function_traits<R (&)(Args...)>
: detail::function_traits_impl<false, false, true, false, false, R, Args...> {};
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)>
: detail::function_traits_impl<false, false, false, true, false, R, C&, Args...> {};
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const>
: detail::function_traits_impl<false, false, false, true, false, R, const C&, Args...> {};
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)&>
: detail::function_traits_impl<false, false, false, true, false, R, C&, Args...> {};
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) &&>
: detail::function_traits_impl<false, false, false, true, false, R, C&&, Args...> {};
template <typename R, typename... Args>
struct function_traits<R(Args...) noexcept>
: detail::function_traits_impl<true, false, false, false, true, R, Args...> {};
template <typename R, typename... Args>
struct function_traits<R (*)(Args...) noexcept>
: detail::function_traits_impl<false, true, false, false, true, R, Args...> {};
template <typename R, typename... Args>
struct function_traits<R (*const)(Args...) noexcept>
: detail::function_traits_impl<false, true, false, false, true, R, Args...> {};
template <typename R, typename... Args>
struct function_traits<R (&)(Args...) noexcept>
: detail::function_traits_impl<false, false, true, false, true, R, Args...> {};
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) noexcept>
: detail::function_traits_impl<false, false, false, true, true, R, C&, Args...> {};
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const noexcept>
: detail::function_traits_impl<false, false, false, true, true, R, const C&, Args...> {};
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)& noexcept>
: detail::function_traits_impl<false, false, false, true, true, R, C&, Args...> {};
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)&& noexcept>
: detail::function_traits_impl<false, false, false, true, true, R, C&&, Args...> {};

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
template <function_type T>
using ref = T&;
template <functional T>
using function_type_of = typename function_traits<T>::function_type;
template <functional T>
using return_type_of = typename function_traits<T>::return_type;
template <functional T>
using parameter_types_of = typename function_traits<T>::parameter_types;
template <functional T>
inline constexpr bool is_noexcept = function_traits<T>::is_noexcept;

//-------------------------------------------------------------------------------------------------
// unwrap
//-------------------------------------------------------------------------------------------------
namespace detail {
template <typename T>
inline constexpr bool should_move = std::is_move_constructible_v<std::remove_cvref_t<T>> &&
    !std::is_const_v<std::remove_reference_t<T>> && !std::is_lvalue_reference_v<T>;
template <typename T>
constexpr decltype(auto) maybe_move(typename std::remove_reference_t<T>& v) noexcept {
  if constexpr (should_move<T>) {
    return std::move(v);
  } else {
    return v;
  }
}

template <member_function auto F, typename, type_list>
struct unwrap_f;
template <member_function auto F, typename C, typename... Args>
struct unwrap_f<F, C, list<Args...>> {
  static inline constexpr decltype(auto)
  f(C c, Args... args) noexcept(noexcept((maybe_move<C>(c).*F)(maybe_move<Args>(args)...))) {
    return (maybe_move<C>(c).*F)(maybe_move<Args>(args)...);
  }
};
template <function auto F, bool MemPtr = member_function<decltype(F)>>
struct unwrap_if {
  static inline constexpr auto value = F;
};
template <function auto F>
struct unwrap_if<F, true> {
  using parameter_types = parameter_types_of<decltype(F)>;
  static inline constexpr auto value =
      &unwrap_f<F, front<parameter_types>, sublist<parameter_types, 1>>::f;
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
  inline static constexpr decltype(auto) f(Args... args) noexcept(noexcept((F(args...), ...))) {
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

template <function auto F, bool CastToNoExcept, typename R, type_list, type_list, type_list,
          type_list>
struct cast_f;
template <function auto F, bool CastToNoExcept, typename R, typename... UsedSourceArgs,
          typename... UnusedSourceArgs, typename... UsedTargetArgs, typename... UnusedTargetArgs>
struct cast_f<F, CastToNoExcept, R, list<UsedSourceArgs...>, list<UnusedSourceArgs...>,
              list<UsedTargetArgs...>, list<UnusedTargetArgs...>> {
  inline static constexpr decltype(auto) f(UsedTargetArgs... args, UnusedTargetArgs...) noexcept(
      CastToNoExcept ||
      noexcept(R(F(UsedSourceArgs(maybe_move<UsedTargetArgs>(args))..., UnusedSourceArgs()...)))) {
    return R(F(UsedSourceArgs(maybe_move<UsedTargetArgs>(args))..., UnusedSourceArgs()...));
  }
};

template <functional Source, function_type Target>
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
      &cast_f<F, is_noexcept<Target>, return_type_of<Target>, used_source_args, unused_source_args,
              used_target_args, unused_target_args>::f;
};

template <function_type T, function auto F,
          bool IsSame = std::is_same_v<T, function_type_of<decltype(F)>> ||
              (is_noexcept<T> && is_noexcept<decltype(F)> &&
               std::is_same_v<function_type_of<T>, function_type_of<decltype(F)>>)>
struct cast_if {
  static inline constexpr auto value = F;
};
template <function_type T, function auto F>
struct cast_if<T, F, false> {
  static inline constexpr auto value = cast_impl<decltype(F), T>::template cast<F>;
};
}  // namespace detail

template <typename Source, typename Target>
concept castable_to = functional<Source> && function_type<Target> &&
    detail::cast_impl<Source, Target>::return_type_convertible &&
    detail::cast_impl<Source, Target>::matching_parameters_convertible &&
    detail::cast_impl<Source, Target>::missing_parameters_default_constructible;

template <function_type T, function auto F>
requires castable_to<decltype(F), T>
inline constexpr auto cast = detail::cast_if<T, unwrap<F>>::value;

//-------------------------------------------------------------------------------------------------
// reinterpret
//-------------------------------------------------------------------------------------------------
namespace detail {
template <typename From, typename To>
inline static constexpr bool is_reinterpret_cast_valid = requires {
  reinterpret_cast<To>(std::declval<From>());
};
struct Foo {};
template <typename From, typename To>
inline static constexpr bool is_reinterpretable = std::is_same_v<From, To> ||
    (std::is_reference_v<From> && std::is_pointer_v<To> &&
     is_reinterpret_cast_valid<std::remove_reference_t<From>*, To>) ||
    (std::is_reference_v<To> && std::is_pointer_v<From> &&
     is_reinterpret_cast_valid<From, std::remove_reference_t<To>*>) ||
    (!std::is_reference_v<From> && !std::is_reference_v<To> && is_reinterpret_cast_valid<From, To>);
template <typename... Froms, typename... Tos>
constexpr bool all_reinterpretable(list<Froms...>, list<Tos...>) {
  return (is_reinterpretable<Froms, Tos> && ...);
}
template <typename From, typename To>
requires is_reinterpretable<From, To>
decltype(auto) maybe_reinterpret(typename std::remove_reference_t<From>& v) noexcept {
  if constexpr (std::is_same_v<From, To>) {
    return maybe_move<To>(v);
  } else if constexpr (std::is_reference_v<From> && std::is_pointer_v<To>) {
    return reinterpret_cast<To>(&v);
  } else if constexpr (std::is_reference_v<To> && std::is_pointer_v<From>) {
    return *reinterpret_cast<std::remove_reference_t<To>*>(v);
  } else {
    return reinterpret_cast<To>(v);
  }
}

template <function auto F, typename SourceR, typename TargetR, type_list, type_list>
struct reinterpret_f;
template <function auto F, typename SourceR, typename TargetR, typename... SourceArgs,
          typename... TargetArgs>
struct reinterpret_f<F, SourceR, TargetR, list<SourceArgs...>, list<TargetArgs...>> {
  inline static decltype(auto)
  f(TargetArgs... args) noexcept(noexcept(F(maybe_reinterpret<TargetArgs, SourceArgs>(args)...))) {
    if constexpr (std::is_same_v<SourceR, TargetR>) {
      return F(maybe_reinterpret<TargetArgs, SourceArgs>(args)...);
    } else if constexpr (std::is_reference_v<SourceR> && std::is_pointer_v<TargetR>) {
      return reinterpret_cast<TargetR>(&F(maybe_reinterpret<TargetArgs, SourceArgs>(args)...));
    } else if constexpr (std::is_reference_v<TargetR> && std::is_pointer_v<SourceR>) {
      return *reinterpret_cast<std::remove_reference_t<TargetR>*>(
          F(maybe_reinterpret<TargetArgs, SourceArgs>(args)...));
    } else {
      return reinterpret_cast<TargetR>(F(maybe_reinterpret<TargetArgs, SourceArgs>(args)...));
    }
  }
};

template <function_type T, function auto F,
          bool IsSame = std::is_same_v<T, function_type_of<decltype(F)>>>
struct reinterpret_if {
  static inline constexpr auto value = F;
};
template <function_type T, function auto F>
struct reinterpret_if<T, F, false> {
  static inline constexpr auto value =
      &reinterpret_f<F, return_type_of<decltype(F)>, return_type_of<T>,
                     parameter_types_of<decltype(F)>, parameter_types_of<T>>::f;
};
}  // namespace detail

template <typename Source, typename Target>
concept reinterpretable_as = functional<Source> && function_type<Target> && !is_noexcept<Target> &&
    size<parameter_types_of<Source>> == size<parameter_types_of<Target>> &&
    detail::is_reinterpretable<return_type_of<Source>, return_type_of<Target>> &&
    detail::all_reinterpretable(parameter_types_of<Target>{}, parameter_types_of<Source>{});

template <function_type T, function auto F>
requires reinterpretable_as<decltype(F), T>
inline constexpr auto reinterpret = detail::reinterpret_if<T, unwrap<F>>::value;

//-------------------------------------------------------------------------------------------------
// bind_front / bind_back
//-------------------------------------------------------------------------------------------------
namespace detail {
template <bool Front, function auto F, type_list, type_list, auto...>
struct bind_f;
template <function auto F, typename... BoundArgs, typename... UnboundArgs, auto... Values>
struct bind_f<true, F, list<BoundArgs...>, list<UnboundArgs...>, Values...> {
  inline static constexpr decltype(auto)
  f(UnboundArgs... args) noexcept(noexcept(F(BoundArgs(Values)...,
                                             maybe_move<UnboundArgs>(args)...))) {
    return F(BoundArgs(Values)..., maybe_move<UnboundArgs>(args)...);
  }
};
template <function auto F, typename... BoundArgs, typename... UnboundArgs, auto... Values>
struct bind_f<false, F, list<BoundArgs...>, list<UnboundArgs...>, Values...> {
  inline static constexpr decltype(auto)
  f(UnboundArgs... args) noexcept(noexcept(F(maybe_move<UnboundArgs>(args)...,
                                             BoundArgs(Values)...))) {
    return F(maybe_move<UnboundArgs>(args)..., BoundArgs(Values)...);
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
  inline static constexpr decltype(auto)
  f(FArgs... fargs, GArgs... gargs) noexcept(noexcept(G(ComposedArg(F(maybe_move<FArgs>(fargs)...)),
                                                        maybe_move<GArgs>(gargs)...))) {
    return G(ComposedArg(F(maybe_move<FArgs>(fargs)...)), maybe_move<GArgs>(gargs)...);
  }
};

template <type_list, type_list, typename, function auto G, function auto F>
struct compose_back_f;
template <typename... GArgs, typename... FArgs, typename ComposedArg, function auto G,
          function auto F>
struct compose_back_f<list<GArgs...>, list<FArgs...>, ComposedArg, G, F> {
  inline static constexpr decltype(auto) f(GArgs... gargs, FArgs... fargs) noexcept(
      noexcept(G(maybe_move<GArgs>(gargs)..., ComposedArg(F(maybe_move<FArgs>(fargs)...))))) {
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
