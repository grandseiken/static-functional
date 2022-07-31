#include <sfun/type_list.h>
#include <type_traits>

namespace sfun {
namespace {
struct A {
  A() = delete;
};
struct B {
  B() = delete;
};
struct C {
  C() = delete;
};
struct D {
  D() = delete;
};

template <typename T>
struct Wrapper {
  Wrapper() = delete;
};
template <typename T>
using wrapper_alias = Wrapper<T>;

template <typename T, typename U>
struct Pair {
  Pair() = delete;
};
template <typename T, typename U = T>
struct DefaultPair {
  DefaultPair() = delete;
};

template <typename T>
using is_a = std::is_same<A, T>;
template <typename T>
using not_a = std::negation<is_a<T>>;

template <typename T>
struct next;
template <>
struct next<A> : std::type_identity<B> {};
template <>
struct next<B> : std::type_identity<C> {};
template <>
struct next<C> : std::type_identity<D> {};
template <>
struct next<D> : std::type_identity<A> {};
template <typename T>
using next_t = typename next<T>::type;

template <typename T, typename U>
inline constexpr bool equal = std::is_same_v<T, U>;

static_assert(type_list<list<>>);
static_assert(type_list<list<A>>);
static_assert(!type_list<A>);
static_assert(size<list<>> == 0);
static_assert(size<list<A>> == 1);
static_assert(size<list<A, B>> == 2);
static_assert(empty<list<>>);
static_assert(!empty<list<A>>);
static_assert(equal<apply<std::type_identity, A>, A>);

static_assert(equal<concat<list<>, list<>>, list<>>);
static_assert(equal<concat<list<>, list<A>>, list<A>>);
static_assert(equal<concat<list<A>, list<>>, list<A>>);
static_assert(equal<concat<list<A>, list<B>>, list<A, B>>);
static_assert(equal<concat<list<A, B>, list<C, D>>, list<A, B, C, D>>);
static_assert(equal<append<A, list<>>, list<A>>);
static_assert(equal<append<C, list<A, B>>, list<A, B, C>>);
static_assert(equal<prepend<A, list<>>, list<A>>);
static_assert(equal<prepend<A, list<B, C>>, list<A, B, C>>);

static_assert(equal<drop_front<list<A>>, list<>>);
static_assert(equal<drop_front<list<A, B, C>>, list<B, C>>);
static_assert(equal<drop_back<list<A>>, list<>>);
static_assert(equal<drop_back<list<A, B, C>>, list<A, B>>);
static_assert(equal<front<list<A>>, A>);
static_assert(equal<front<list<A, B, C>>, A>);
static_assert(equal<back<list<A>>, A>);
static_assert(equal<back<list<A, B, C>>, C>);

static_assert(equal<to<list<A, B, C>, list>, list<A, B, C>>);
static_assert(equal<to<list<A, B>, Pair>, Pair<A, B>>);

static_assert(equal<get<list<A>, 0>, A>);
static_assert(equal<get<list<A, B>, 0>, A>);
static_assert(equal<get<list<A, B>, 1>, B>);
static_assert(equal<get<list<A, B, C>, 0>, A>);
static_assert(equal<get<list<A, B, C>, 1>, B>);
static_assert(equal<get<list<A, B, C>, 2>, C>);

static_assert(equal<sublist<list<>, 0>, list<>>);
static_assert(equal<sublist<list<A>, 0>, list<A>>);
static_assert(equal<sublist<list<A>, 1>, list<>>);
static_assert(equal<sublist<list<A>, 0, 0>, list<>>);
static_assert(equal<sublist<list<A, B, C>, 0>, list<A, B, C>>);
static_assert(equal<sublist<list<A, B, C>, 1>, list<B, C>>);
static_assert(equal<sublist<list<A, B, C>, 2>, list<C>>);
static_assert(equal<sublist<list<A, B, C>, 3>, list<>>);
static_assert(equal<sublist<list<A, B, C>, 0, 0>, list<>>);
static_assert(equal<sublist<list<A, B, C>, 1, 0>, list<>>);
static_assert(equal<sublist<list<A, B, C>, 2, 0>, list<>>);
static_assert(equal<sublist<list<A, B, C>, 3, 0>, list<>>);
static_assert(equal<sublist<list<A, B, C>, 0, 1>, list<A>>);
static_assert(equal<sublist<list<A, B, C>, 1, 1>, list<B>>);
static_assert(equal<sublist<list<A, B, C>, 2, 1>, list<C>>);
static_assert(equal<sublist<list<A, B, C>, 3, 1>, list<>>);
static_assert(equal<sublist<list<A, B, C>, 0, 2>, list<A, B>>);
static_assert(equal<sublist<list<A, B, C>, 1, 2>, list<B, C>>);
static_assert(equal<sublist<list<A, B, C>, 2, 2>, list<C>>);
static_assert(equal<sublist<list<A, B, C>, 3, 2>, list<>>);
static_assert(equal<sublist<list<A, B, C>, 0, 3>, list<A, B, C>>);
static_assert(equal<sublist<list<A, B, C>, 1, 3>, list<B, C>>);
static_assert(equal<sublist<list<A, B, C>, 2, 3>, list<C>>);
static_assert(equal<sublist<list<A, B, C>, 3, 3>, list<>>);

static_assert(equal<select<list<>>, list<>>);
static_assert(equal<select<list<A>, 0>, list<A>>);
static_assert(equal<select<list<A, B>, 0>, list<A>>);
static_assert(equal<select<list<A, B>, 1>, list<B>>);
static_assert(equal<select<list<A, B>, 0, 1>, list<A, B>>);
static_assert(equal<select<list<A, B>, 1, 0>, list<B, A>>);
static_assert(equal<select<list<A, B, C>, 2, 1, 0>, list<C, B, A>>);
static_assert(equal<select<list<A, B, C>, 0, 2>, list<A, C>>);

static_assert(equal<apply<next, A>, B>);
static_assert(equal<apply<next_t, A>, B>);
static_assert(equal<apply<Pair, A, B>, Pair<A, B>>);
static_assert(equal<apply<DefaultPair, A, B>, DefaultPair<A, B>>);
static_assert(equal<apply<DefaultPair, A>, DefaultPair<A, A>>);
static_assert(equal<apply<DefaultPair, A>, DefaultPair<A, A>>);
static_assert(equal<apply<std::type_identity, A>, A>);
static_assert(equal<apply<std::type_identity_t, A>, A>);

static_assert(all_of<list<>, is_a>);
static_assert(!any_of<list<>, is_a>);
static_assert(none_of<list<>, is_a>);
static_assert(all_of<list<A, A, A>, is_a>);
static_assert(!all_of<list<A, B, A>, is_a>);
static_assert(any_of<list<B, B, A>, is_a>);
static_assert(!any_of<list<B, C, B>, is_a>);
static_assert(none_of<list<B, C, D>, is_a>);
static_assert(!none_of<list<B, C, A>, is_a>);

static_assert(find_if<list<>, is_a> == 0);
static_assert(find_if<list<A, B, B>, is_a> == 0);
static_assert(find_if<list<B, A, B>, is_a> == 1);
static_assert(find_if<list<B, B, A>, is_a> == 2);
static_assert(find_if<list<B, B, B>, is_a> == 3);
static_assert(find<list<A, B, B>, A> == 0);
static_assert(find<list<B, A, B>, A> == 1);
static_assert(find<list<B, B, A>, A> == 2);
static_assert(find<list<B, B, B>, A> == 3);
static_assert(count_if<list<>, is_a> == 0);
static_assert(count_if<list<B, B, B>, is_a> == 0);
static_assert(count_if<list<A, B, C>, is_a> == 1);
static_assert(count_if<list<A, A, A>, is_a> == 3);
static_assert(count<list<>, A> == 0);
static_assert(count<list<B, B, B>, A> == 0);
static_assert(count<list<A, B, C>, A> == 1);
static_assert(count<list<A, A, A>, A> == 3);

static_assert(equal<filter<list<>, is_a>, list<>>);
static_assert(equal<filter<list<A>, is_a>, list<A>>);
static_assert(equal<filter<list<B>, is_a>, list<>>);
static_assert(equal<filter<list<A, B>, is_a>, list<A>>);
static_assert(equal<filter<list<B, A>, is_a>, list<A>>);
static_assert(equal<filter<list<A, A>, is_a>, list<A, A>>);
static_assert(equal<filter<list<B, B>, is_a>, list<>>);
static_assert(equal<filter<list<B, C, D>, not_a>, list<B, C, D>>);
static_assert(equal<filter<list<A, B, C>, not_a>, list<B, C>>);
static_assert(equal<filter<list<B, A, C>, not_a>, list<B, C>>);
static_assert(equal<filter<list<B, C, A>, not_a>, list<B, C>>);
static_assert(equal<filter<list<A, A, B>, not_a>, list<B>>);
static_assert(equal<filter<list<B, A, A>, not_a>, list<B>>);
static_assert(equal<filter<list<A, B, A>, not_a>, list<B>>);
static_assert(equal<filter<list<A, A, A>, not_a>, list<>>);

static_assert(equal<map<list<>, std::type_identity>, list<>>);
static_assert(equal<map<list<A>, std::type_identity>, list<A>>);
static_assert(equal<map<list<A, B, C>, std::type_identity>, list<A, B, C>>);
static_assert(equal<map<list<A, B, C, D>, next>, list<B, C, D, A>>);
static_assert(equal<map<list<A, B, C, D>, next_t>, list<B, C, D, A>>);
static_assert(equal<map<list<>, Wrapper>, list<>>);
static_assert(equal<map<list<A, B, C>, Wrapper>, list<Wrapper<A>, Wrapper<B>, Wrapper<C>>>);
static_assert(equal<map<list<A, B, C>, wrapper_alias>, list<Wrapper<A>, Wrapper<B>, Wrapper<C>>>);
static_assert(equal<map<list<A, B, C>, list>, list<list<A>, list<B>, list<C>>>);
static_assert(equal<map<list<A, B>, DefaultPair>, list<DefaultPair<A, A>, DefaultPair<B, B>>>);
}  // namespace
}  // namespace sfun
