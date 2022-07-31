#ifndef STATIC_FUNCTIONAL_INCLUDE_SFUN_TYPE_LIST_H
#define STATIC_FUNCTIONAL_INCLUDE_SFUN_TYPE_LIST_H
#include <cstddef>
#include <limits>
#include <type_traits>

namespace sfun {

template <typename...>
struct list {};

template <typename>
inline constexpr bool is_type_list = false;
template <typename... Ts>
inline constexpr bool is_type_list<list<Ts...>> = true;
template <typename T>
concept type_list = is_type_list<T>;

template <type_list>
inline constexpr std::size_t size = 0;
template <typename... Ts>
inline constexpr std::size_t size<list<Ts...>> = sizeof...(Ts);
template <type_list A>
inline constexpr bool empty = !size<A>;
inline constexpr std::size_t end = std::numeric_limits<std::size_t>::max();
template <template <typename> typename F, typename T>
using apply = typename F<T>::type;

namespace detail {
template <typename... As, typename... Bs>
consteval auto concat_impl(list<As...>, list<Bs...>) -> list<As..., Bs...>;
template <typename T, typename... Ts>
consteval auto append_impl(list<Ts...>) -> list<Ts..., T>;
template <typename T, typename... Ts>
consteval auto prepend_impl(list<Ts...>) -> list<T, Ts...>;
template <typename T, typename... Ts>
consteval auto drop_front_impl(list<T, Ts...>) -> list<Ts...>;
template <typename T, typename... Ts>
consteval auto drop_back_impl(list<Ts..., T>) -> list<Ts...>;
template <typename T, typename... Ts>
consteval auto front_impl(list<T, Ts...>) -> T;
template <typename T, typename... Ts>
consteval auto back_impl(list<Ts..., T>) -> T;
template <template <typename...> typename Template, typename... Ts>
consteval auto to_impl(list<Ts...>) -> Template<Ts...>;
}  // namespace detail

template <type_list A, type_list B>
using concat = decltype(detail::concat_impl(A{}, B{}));
template <typename T, type_list A>
using append = decltype(detail::append_impl<T>(A{}));
template <typename T, type_list A>
using prepend = decltype(detail::prepend_impl<T>(A{}));
template <type_list A>
requires(!empty<A>) using drop_front = decltype(detail::drop_front_impl(A{}));
template <type_list A>
requires(!empty<A>) using drop_back = decltype(detail::drop_back_impl(A{}));
template <type_list A>
requires(!empty<A>) using front = decltype(detail::front_impl(A{}));
template <type_list A>
requires(!empty<A>) using back = decltype(detail::back_impl(A{}));
template <type_list A, template <typename...> typename Template>
using to = decltype(detail::to_impl<Template>(A{}));

namespace detail {
template <std::size_t N, type_list A>
consteval auto get_impl(A) {
  if constexpr (N == 0) {
    return front<A>{};
  } else {
    return get_impl<N - 1>(drop_front<A>{});
  }
}

template <std::size_t Index, std::size_t Size, type_list A>
consteval auto sublist_impl(A) {
  if constexpr (empty<A> || Size == 0) {
    return list<>{};
  } else if constexpr (Index == 0) {
    return prepend<front<A>, decltype(sublist_impl<0, Size - 1>(drop_front<A>{}))>{};
  } else {
    return sublist_impl<Index - 1, Size>(drop_front<A>{});
  }
}
}  // namespace detail

template <type_list A, std::size_t N>
requires(N < size<A>) using get = decltype(detail::get_impl<N>(A{}));
template <type_list A, std::size_t Index, std::size_t Size = end>
requires(Index <= size<A>) using sublist = decltype(detail::sublist_impl<Index, Size>(A{}));
template <type_list A, std::size_t... N>
requires((N < size<A>)&&...) using select = list<get<A, N>...>;

namespace detail {
template <template <typename> typename P, typename... Ts>
consteval bool all_of_impl(list<Ts...>) {
  return (P<Ts>::value && ...);
}

template <template <typename> typename P, typename... Ts>
consteval bool any_of_impl(list<Ts...>) {
  return (P<Ts>::value || ...);
}

template <template <typename> typename P, type_list A>
consteval std::size_t find_if_impl(A) {
  if constexpr (empty<A>) {
    return 0;
  } else if constexpr (P<front<A>>::value) {
    return 0;
  } else {
    return 1 + find_if_impl<P>(drop_front<A>{});
  }
}

template <template <typename> typename P, typename... Ts>
consteval std::size_t count_if_impl(list<Ts...>) {
  return ((P<Ts>::value ? 1u : 0u) + ...);
}

template <template <typename> typename P, type_list A>
consteval auto filter_impl(A) {
  if constexpr (empty<A>) {
    return list<>{};
  } else if constexpr (P<front<A>>::value) {
    return prepend<front<A>, decltype(filter_impl<P>(drop_front<A>{}))>{};
  } else {
    return filter_impl<P>(drop_front<A>{});
  }
}

template <template <typename> typename F, type_list A>
consteval auto map_impl(A) {
  if constexpr (empty<A>) {
    return list<>{};
  } else {
    return prepend<apply<F, front<A>>, decltype(map_impl<F>(drop_front<A>{}))>{};
  }
}

template <typename T>
struct same_as_metafunction {
  template <typename U>
  struct predicate : std::is_same<T, U> {};
};

template <template <typename> typename T>
struct wrap_metafunction {
  template <typename U>
  struct functor : std::type_identity<T<U>> {};
};
}  // namespace detail

template <type_list A, template <typename> typename P>
inline constexpr bool all_of = detail::all_of_impl<P>(A{});
template <type_list A, template <typename> typename P>
inline constexpr bool any_of = detail::any_of_impl<P>(A{});
template <type_list A, template <typename> typename P>
inline constexpr bool none_of = !any_of<A, P>;
template <type_list A, template <typename> typename P>
inline constexpr std::size_t find_if = detail::find_if_impl<P>(A{});
template <type_list A, template <typename> typename P>
inline constexpr std::size_t count_if = detail::count_if_impl<P>(A{});
template <type_list A, template <typename> typename P>
using filter = decltype(detail::filter_impl<P>(A{}));
template <type_list A, template <typename> typename F>
using map = decltype(detail::map_impl<F>(A{}));

template <type_list A, typename T>
inline constexpr std::size_t find = find_if<A, detail::same_as_metafunction<T>::template predicate>;
template <type_list A, typename T>
inline constexpr std::size_t count =
    count_if<A, detail::same_as_metafunction<T>::template predicate>;
template <type_list A, template <typename> typename T>
using wrap = map<A, detail::wrap_metafunction<T>::template functor>;

}  // namespace sfun

#endif
