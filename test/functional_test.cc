#include <sfn/functional.h>

namespace sfn {
namespace {

struct NotDefaultConstructible {
  NotDefaultConstructible(int) {}
};
struct MoveOnly {
  constexpr MoveOnly() = default;
  constexpr MoveOnly(MoveOnly&&) noexcept = default;
  constexpr MoveOnly& operator=(MoveOnly&&) noexcept = default;
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;
};
struct A {
  constexpr A() = default;
  constexpr int f() const {
    return 1;
  }
  constexpr void g(int) {}
  constexpr void g2() const {}
  constexpr int g3(int x) const {
    return x;
  }
  void h(int);
  constexpr int accepts_move_only(MoveOnly&&) const {
    return 4;
  }
  constexpr int no_except(int) const noexcept {
    return 5;
  }
};
struct ConvertsToA {
  constexpr operator A() const {
    return A{};
  }
};
constexpr int f() {
  return 2;
}
void g(int) {}
inline constexpr auto h = [](int) constexpr {
  return 3;
};
constexpr int f2(int) {
  return 2;
}
constexpr int f_no_except(int) noexcept {
  return 6;
}
constexpr ConvertsToA make_converts_to_a() {
  return {};
}
const ConvertsToA& const_ref_converts_to_a() {
  static const ConvertsToA converts_to_a;
  return converts_to_a;
}
constexpr int accepts_a(A) {
  return 4;
}
constexpr int accepts_move_only(MoveOnly) {
  return 5;
}
constexpr MoveOnly make_move_only() {
  return {};
}
constexpr int int_identity(int x) {
  return x;
}
constexpr int sum(int a, int b) {
  return a + b;
}
constexpr int minus(int a, int b) {
  return a - b;
}

template <typename T, typename U>
inline constexpr bool equal = std::is_same_v<T, U>;

static_assert(member_function<decltype(&A::f)>);
static_assert(member_function<decltype(&A::g)>);
static_assert(member_function<decltype(&A::h)>);
static_assert(member_function<decltype(&A::no_except)>);
static_assert(!member_function<decltype(&f)>);
static_assert(!member_function<decltype(&g)>);
static_assert(!member_function<int>);

static_assert(function<decltype(&A::f)>);
static_assert(function<decltype(&f)>);
static_assert(function<decltype(&g)>);
static_assert(function<decltype(&f_no_except)>);
static_assert(function<void (&)(int)>);
static_assert(function<int (*)(void)>);
static_assert(function<int (*const)(int, int)>);
static_assert(!function<int>);
static_assert(function<decltype(+h)>);

static_assert(function_type<void(int, int)>);
static_assert(function_type<int()>);
static_assert(!function_type<void (*)(int)>);
static_assert(!function_type<void (&)(int)>);
static_assert(!function_type<int>);

static_assert(functional<decltype(&A::f)>);
static_assert(functional<decltype(&A::g)>);
static_assert(functional<decltype(&A::h)>);
static_assert(functional<decltype(&f)>);
static_assert(functional<decltype(&g)>);
static_assert(functional<void(int, int)>);
static_assert(functional<int()>);
static_assert(functional<void (*)(int)>);
static_assert(functional<void (&)(int)>);
static_assert(!functional<int>);

static_assert(equal<function_type_of<void()>, void()>);
static_assert(equal<function_type_of<int(int, int)>, int(int, int)>);
static_assert(equal<function_type_of<void (*)()>, void()>);
static_assert(equal<function_type_of<void (*const)()>, void()>);
static_assert(equal<function_type_of<void (&)()>, void()>);
static_assert(equal<function_type_of<sfn::ptr<int(float)>>, int(float)>);
static_assert(equal<function_type_of<sfn::ref<float(int)>>, float(int)>);
static_assert(equal<function_type_of<decltype(&f)>, int()>);
static_assert(equal<function_type_of<decltype(&g)>, void(int)>);
static_assert(equal<function_type_of<decltype(&A::f)>, int(const A&)>);
static_assert(equal<function_type_of<decltype(&A::g)>, void(A&, int)>);
static_assert(equal<function_type_of<decltype(&A::h)>, void(A&, int)>);

static_assert(equal<return_type_of<void()>, void>);
static_assert(equal<return_type_of<int(int, int)>, int>);
static_assert(equal<return_type_of<int (*)(int, int)>, int>);
static_assert(equal<return_type_of<void (*const)(int)>, void>);
static_assert(equal<return_type_of<int (&)(int)>, int>);
static_assert(equal<return_type_of<decltype(&f)>, int>);
static_assert(equal<return_type_of<decltype(&A::f)>, int>);

static_assert(equal<parameter_types_of<void()>, list<>>);
static_assert(equal<parameter_types_of<int(int, int)>, list<int, int>>);
static_assert(equal<parameter_types_of<int(const int&)>, list<const int&>>);
static_assert(equal<parameter_types_of<void (*)(int)>, list<int>>);
static_assert(equal<parameter_types_of<void (*const)(int)>, list<int>>);
static_assert(equal<parameter_types_of<void (&)(int)>, list<int>>);
static_assert(equal<parameter_types_of<decltype(&f)>, list<>>);
static_assert(equal<parameter_types_of<decltype(&g)>, list<int>>);
static_assert(equal<parameter_types_of<decltype(&A::f)>, list<const A&>>);
static_assert(equal<parameter_types_of<decltype(&A::g)>, list<A&, int>>);
static_assert(equal<parameter_types_of<decltype(&A::h)>, list<A&, int>>);

static_assert(is_noexcept<void(int) noexcept>);
static_assert(is_noexcept<decltype(&f_no_except)>);
static_assert(is_noexcept<decltype(&A::no_except)>);
static_assert(!is_noexcept<int()>);
static_assert(!is_noexcept<decltype(&f)>);

static_assert(equal<function_type_of<decltype(unwrap<&f>)>, int()>);
static_assert(equal<function_type_of<decltype(unwrap<&A::f>)>, int(const A&)>);
static_assert(equal<function_type_of<decltype(unwrap<&A::g>)>, void(A&, int)>);
static_assert(unwrap<&f> == &f);
static_assert(unwrap<&A::f>(A{}) == 1);
static_assert(unwrap<&f>() == 2);
static_assert(unwrap<+h>(0) == 3);
static_assert(unwrap<&A::accepts_move_only>(A{}, MoveOnly{}) == 4);
static_assert(unwrap<&A::no_except>(A{}, 1) == 5);

static_assert(sequencable<void()>);
static_assert(sequencable<int(int, A), int(int, A)>);
static_assert(sequencable<void(int, A), int(int, A), A(int, A)>);
static_assert(!sequencable<int(int, A), int(A, int)>);
static_assert(!sequencable<int(int, A), int(A)>);
static_assert(!sequencable<int(int, A), int(int)>);
static_assert(!sequencable<int(int), int()>);
static_assert(!sequencable<int(MoveOnly), int(MoveOnly)>);
static_assert(equal<function_type_of<decltype(sequence<&g, +h>)>, int(int)>);
static_assert(equal<function_type_of<decltype(sequence<+h, &g>)>, void(int)>);
static_assert(equal<function_type_of<decltype(sequence<&A::g2, &A::f>)>, int(const A&)>);
static_assert(equal<function_type_of<decltype(sequence<&A::f, &A::g2>)>, void(const A&)>);
static_assert(sequence<&f2> == &f2);
static_assert(sequence<&f2>(0) == 2);
static_assert(sequence<&f2, +h>(0) == 3);
static_assert(sequence<+h, &f2>(0) == 2);
static_assert(sequence<&f_no_except, &f2>(0) == 2);
static_assert(sequence<&A::g2, &A::f>(A{}) == 1);
static_assert(sequence<&A::g3, &A::no_except>(A{}, 0) == 5);

static_assert(castable_to<void(int, int), void(int, int, int)>);
static_assert(castable_to<void(int, int), void(int)>);
static_assert(castable_to<int(), void()>);
static_assert(!castable_to<void(), int()>);
static_assert(castable_to<void(), void() noexcept>);
static_assert(castable_to<void(int, A), void(int)>);
static_assert(!castable_to<void(int, NotDefaultConstructible), void(int)>);
static_assert(castable_to<ConvertsToA(), A()>);
static_assert(!castable_to<A(), ConvertsToA()>);
static_assert(castable_to<void(A), void(ConvertsToA)>);
static_assert(!castable_to<void(ConvertsToA), void(A)>);
static_assert(equal<function_type_of<decltype(cast<void(), &g>)>, void()>);
static_assert(equal<function_type_of<decltype(cast<void(int, int), &g>)>, void(int, int)>);
static_assert(equal<function_type_of<decltype(cast<void(), &f>)>, void()>);
static_assert(equal<function_type_of<decltype(cast<A(), &make_converts_to_a>)>, A()>);
static_assert(equal<function_type_of<decltype(cast<A(), &const_ref_converts_to_a>)>, A()>);
static_assert(
    equal<function_type_of<decltype(cast<int(ConvertsToA), &accepts_a>)>, int(ConvertsToA)>);
static_assert(
    equal<function_type_of<decltype(cast<int(ConvertsToA&), &accepts_a>)>, int(ConvertsToA&)>);
static_assert(equal<function_type_of<decltype(cast<int(const ConvertsToA&), &accepts_a>)>,
                    int(const ConvertsToA&)>);
static_assert(is_noexcept<decltype(cast<int() noexcept, &f>)>);
static_assert(!is_noexcept<decltype(cast<float(), &f>)>);
static_assert(cast<int(), &f> == &f);
static_assert(cast<float(), &f>() == 2.f);
static_assert(cast<int(int), &f>(0) == 2);
static_assert(cast<float(float), &int_identity>(1.1f) == 1.f);
static_assert(cast<A(), &make_converts_to_a>().f() == 1);
static_assert(cast<int(ConvertsToA), &accepts_a>(ConvertsToA{}) == 4);
static_assert(cast<int(const ConvertsToA&), &accepts_a>(ConvertsToA{}) == 4);
static_assert(cast<float(MoveOnly), &accepts_move_only>(MoveOnly{}) == 5.f);
static_assert(accepts_move_only(cast<MoveOnly(int), &make_move_only>(0)) == 5);
static_assert(cast<int() noexcept, &f>() == 2);

static_assert(bindable_front<void()>);
static_assert(bindable_front<void(int)>);
static_assert(bindable_front<void(int), int>);
static_assert(bindable_front<int(int, A), int>);
static_assert(bindable_front<void(A), ConvertsToA>);
static_assert(bindable_front<void(A, float, bool, bool), ConvertsToA, int>);
static_assert(!bindable_front<void(), int>);
static_assert(!bindable_front<void(ConvertsToA), A>);
static_assert(!bindable_front<void(int, A), A>);
static_assert(!bindable_front<void(A, float, bool), int, ConvertsToA>);
static_assert(!bindable_front<void(MoveOnly), MoveOnly>);
static_assert(bindable_back<void()>);
static_assert(bindable_back<void(int)>);
static_assert(bindable_back<void(int), int>);
static_assert(bindable_back<int(A, int), int>);
static_assert(bindable_back<void(A), ConvertsToA>);
static_assert(bindable_back<void(bool, bool, A, float), ConvertsToA, int>);
static_assert(!bindable_back<void(), int>);
static_assert(!bindable_back<void(ConvertsToA), A>);
static_assert(!bindable_back<void(A, int), A>);
static_assert(!bindable_back<void(bool, A, float), int, ConvertsToA>);
static_assert(equal<function_type_of<decltype(bind_front<&int_identity>)>, int(int)>);
static_assert(equal<function_type_of<decltype(bind_front<&int_identity, 42>)>, int()>);
static_assert(equal<function_type_of<decltype(bind_front<&sum, 1>)>, int(int)>);
static_assert(equal<function_type_of<decltype(bind_front<&A::f, A{}>)>, int()>);
static_assert(equal<function_type_of<decltype(bind_back<&int_identity>)>, int(int)>);
static_assert(equal<function_type_of<decltype(bind_back<&int_identity, 42>)>, int()>);
static_assert(equal<function_type_of<decltype(bind_back<&sum, 1>)>, int(int)>);
static_assert(equal<function_type_of<decltype(bind_back<&A::g, 1>)>, void(A&)>);
static_assert(bind_front<&f> == &f);
static_assert(bind_front<&int_identity>(42) == 42);
static_assert(bind_front<&int_identity, 42>() == 42);
static_assert(bind_front<&sum, 2>(1) == 3);
static_assert(bind_front<&sum, 4, 5>() == 9);
static_assert(bind_front<&minus, 4>(1) == 3);
static_assert(bind_front<&A::f, A{}>() == 1);
static_assert(bind_back<&f> == &f);
static_assert(bind_back<&int_identity>(42) == 42);
static_assert(bind_back<&int_identity, 42>() == 42);
static_assert(bind_back<&sum, 2>(1) == 3);
static_assert(bind_back<&sum, 4, 5>() == 9);
static_assert(bind_back<&minus, 4>(1) == -3);
static_assert(bind_back<&A::g3, 3>(A{}) == 3);

static_assert(composable_front<void(int), int(int)>);
static_assert(composable_front<void(int, A), int(void)>);
static_assert(!composable_front<void(int, A), A(void)>);
static_assert(composable_back<void(int), int(int)>);
static_assert(composable_back<void(int, A), A(void)>);
static_assert(composable_back<void(int, A), ConvertsToA(void)>);
static_assert(!composable_back<void(int, A), int(void)>);
static_assert(composable<void(int), int(void)>);
static_assert(!composable<void(int, A), int(void)>);
static_assert(!composable<void(int, A), A(void)>);
static_assert(equal<function_type_of<decltype(compose_front<&int_identity, &f>)>, int()>);
static_assert(equal<function_type_of<decltype(compose_back<&A::g, &f>)>, void(A&)>);
static_assert(equal<function_type_of<decltype(compose<&int_identity, &int_identity>)>, int(int)>);
static_assert(equal<function_type_of<decltype(compose<&g, &f>)>, void()>);
static_assert(compose<&int_identity, &f>() == 2);
static_assert(compose<&int_identity, &int_identity>(4) == 4);
static_assert(compose<&int_identity, &A::f>(A{}) == 1);
static_assert(compose_front<&sum, &minus>(4, 3, 2) == 3);
static_assert(compose_back<&sum, &minus>(4, 3, 2) == 5);
static_assert(compose_front<&minus, &sum>(4, 3, 2) == 5);
static_assert(compose_back<&minus, &sum>(4, 3, 2) == -1);
static_assert(compose<&accepts_move_only, &make_move_only>() == 5);

}  // namespace
}  // namespace sfn

int main() {
  sfn::sequence<sfn::bind_back<sfn::cast<void(unsigned long), &sfn::g>, 1ul>,
                sfn::bind_front<&sfn::A::f, sfn::A{}>>();
  return 0;
}
