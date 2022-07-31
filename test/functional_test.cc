#include <sfun/functional.h>

namespace sfun {
namespace {

struct A {
  constexpr int f() const {
    return 1;
  }
  constexpr void g(int) {}
  constexpr void g2() const {}
  void h(int);
};
struct ConvertsToA {
  constexpr operator A() const {
    return A{};
  }
};
struct NotDefaultConstructible {
  NotDefaultConstructible(int) {}
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
constexpr int int_identity(int x) {
  return x;
}

template <typename T, typename U>
inline constexpr bool equal = std::is_same_v<T, U>;

static_assert(member_function<decltype(&A::f)>);
static_assert(member_function<decltype(&A::g)>);
static_assert(member_function<decltype(&A::h)>);
static_assert(!member_function<decltype(&f)>);
static_assert(!member_function<decltype(&g)>);
static_assert(!member_function<int>);

static_assert(function<decltype(&A::f)>);
static_assert(function<decltype(&f)>);
static_assert(function<decltype(&g)>);
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

static_assert(same_parameters<int(int, A), int(int, A)>);
static_assert(same_parameters<void(int, A), int(int, A), A(int, A)>);
static_assert(!same_parameters<int(int, A), int(A, int)>);
static_assert(!same_parameters<int(int, A), int(A)>);
static_assert(!same_parameters<int(int, A), int(int)>);
static_assert(!same_parameters<int(int), int()>);

static_assert(castable_to<void(int, int), void(int, int, int)>);
static_assert(castable_to<void(int, int), void(int)>);
static_assert(castable_to<int(), void()>);
static_assert(!castable_to<void(), int()>);
static_assert(castable_to<void(int, A), void(int)>);
static_assert(!castable_to<void(int, NotDefaultConstructible), void(int)>);
static_assert(castable_to<ConvertsToA(), A()>);
static_assert(!castable_to<A(), ConvertsToA()>);
static_assert(castable_to<void(A), void(ConvertsToA)>);
static_assert(!castable_to<void(ConvertsToA), void(A)>);

static_assert(equal<function_type_of<decltype(unwrap<&f>)>, int()>);
static_assert(equal<function_type_of<decltype(unwrap<&A::f>)>, int(const A&)>);
static_assert(equal<function_type_of<decltype(unwrap<&A::g>)>, void(A&, int)>);
static_assert(unwrap<&A::f>(A{}) == 1);
static_assert(unwrap<&f>() == 2);
static_assert(unwrap<+h>(0) == 3);

static_assert(equal<function_type_of<decltype(sequence<&g, +h>)>, int(int)>);
static_assert(equal<function_type_of<decltype(sequence<+h, &g>)>, void(int)>);
static_assert(equal<function_type_of<decltype(sequence<&A::g2, &A::f>)>, int(const A&)>);
static_assert(equal<function_type_of<decltype(sequence<&A::f, &A::g2>)>, void(const A&)>);
static_assert(sequence<&f2, +h>(0) == 3);
static_assert(sequence<+h, f2>(0) == 2);
static_assert(sequence<&A::g2, &A::f>(A{}) == 1);

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
static_assert(cast<float(), &f>() == 2.f);
static_assert(cast<int(int), &f>(0) == 2);
static_assert(cast<float(float), &int_identity>(1.1f) == 1.f);
static_assert(cast<A(), &make_converts_to_a>().f() == 1);
static_assert(cast<int(ConvertsToA), &accepts_a>(ConvertsToA{}) == 4);
static_assert(cast<int(const ConvertsToA&), &accepts_a>(ConvertsToA{}) == 4);

}  // namespace
}  // namespace sfun
