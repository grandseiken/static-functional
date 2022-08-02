# static_functional

This is a standalone header-only C++20 library providing compile-time functional operators (composition, bind, and so on) that work with raw function pointers.

A C++20 compile-time type list library is also included, mostly just to avoid the need for any transitive dependencies&mdash;the functional part requires such a library for its implementation&mdash;but it's there, and should be fairly nice to use if you want to.

## Motivating example 1

```cpp
#include <sfn/functional.h>

int add_one(int x) {
  return x + 1;
}

int square(int x) {
  return x * x;
}

// sfn::ptr<int(int)> is an alias for int (*)(int)
constexpr sfn::ptr<int(int)> fp = sfn::compose<&square, &add_one>;
fp(2);  // returns 9
```

## Motivating example 2

```cpp
#include <sfn/functional.h>

struct Foo {
  int f(int x) const {
    return x;
  }
};

// sfn::ptr<int(const Foo&)> is an alias for int (*)(const Foo&)
constexpr sfn::ptr<int(const Foo&)> fp = sfn::bind_back<&Foo::f, 42>;
fp(Foo{});  // returns 42
```

## Why?

Function pointers are simple. If you want to store them somewhere, you never have to deal with type-erasure, memory allocation, or lifetimes of lambda-captures. Sometimes they're all you need. This library makes working with them feel modern and nice.

## Setup

All you need is the two header files in `include/sfn`.

Bazel users can use the following:

```python
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
http_archive(
  name = "static_functional",
  sha256 = "",
  strip_prefix = "static-functional-1.0",
  url = "https://github.com/grandseiken/static-functional/archive/v0.1.zip",
)
# now depend on @static_functional
```

## Compiler support

Should work with any compiler that supports C++20 language features.

Tested on:
* gcc 11.2.0 (Linux)
* clang 14.0.0 (Linux)
* clang-cl 14.0.6 (Windows)
* MSVC 2022 17.2.6 (Windows)

If your compiler can compile the files in the `test` directory, everything should work fine. You don't need to run anything, the tests are all done at compile time.

## Troubleshooting

Please file an issue if something doesn't work as expected. Pull requests are also welcome.

# Contents

* [&lt;sfn/functional.h&gt;](#sfnfunctionalh)
   * [Basics](#basics)
   * [`unwrap`](#sfnunwrap)
   * [`sequence`](#sfnsequence)
   * [`cast`](#sfncast)
   * [`bind_front` and `bind_back`](#sfnbindfront-and-sfnbindback)
   * [`compose_front` and `compose_back`](#sfncomposefront-and-sfncomposeback)
   * [Notes](#notes)
* [&lt;sfn/type_list.h&gt;](#sfntypelisth)
   * [`sfn::list`](#sfnlist)
   * [Basic operations](#basic-operations)
   * [Algorithms](#algorithms)

# <sfn/functional.h>

## Basics

The library defines several concepts, used throughout for checking inputs:

```cpp
template <typename T>
concept member_function = /* satisfied if T is a pointer-to-member-function type */;
template <typename T>
concept function =        /* satisifed if T is a function pointer type, function reference
                             type, or pointer-to-member-function type */;
template <typename T>
concept function_type =   /* satisfied if T is a bare function type, e.g. void(int) */;
template <typename T>
concept functional =      /* satisfied if T is any of the above */;
```

For any type satisfying any of the above concepts, the following definitions allow inspection of its function type:

```cpp
template <functional T>
using function_type_of =   /* bare function type of T, e.g. void(int),
                              always without noexcept specifier */;
template <functional T>
using return_type_of =     /* return type of (function type of) T */;
template <functional T>
using parameter_types_of = /* type list containing parameter types of
                              (function type of) T, i.e. sfn::list<Args...> */;
template <functional T>
inline constexpr bool is_noexcept = /* true if the function type of T
                                       has the noexcept specifier */;
```

Note that pointer-to-member-function types are transparently converted to regular function pointer types, so e.g. with `struct A { void f(); };`, `sfn::parameter_types_of<&A::f>` would be `sfn::list<A&>` (see `sfn::unwrap` below for more).

Since function pointer types can be awkward to write, aliases are provided for convenience. You can use them if you don't already have something similar:

```cpp
template <function_type T>
using ptr = T*;  /* e.g. sfn::ptr<void(int)> is an alias for void (*)(int) */
template <function_type T>
using ref = T&;  /* similarly for references */
```

## `sfn::unwrap`

Pointer-to-member-function types are awkward, and nobody likes them. `sfn::unwrap` makes them go away:

```cpp
template <function auto F>
inline constexpr auto unwrap = /* ... */;
```

 * If `f` is a pointer-to-member-function of some type `T`, `sfn::unwrap<f>` is a (nonmember) function pointer with converted signature (i.e. the implicit first `T&`, `const T&` or `T&&` parameter is made explicit). Calling it has the same behaviour as invoking `f`.
 * Otherwise, `sfn::unwrap<f>` is equal to `f`.

### Example

```cpp
struct Foo {
  void hello() const { std::cout << "Hello, world!" << std::endl; }
  int f(int);
};

constexpr void (*foo_hello_fp)(const Foo&) = sfn::unwrap<&Foo::hello>;
foo_hello_fp(Foo{});                         // prints "Hello, world!"
constexpr auto* fp = sfn::unwrap<&Foo::f>;   // type is int (*)(Foo&, int)
```

All of the other functional operators below implicitly `sfn::unwrap` their arguments, so any pointer-to-member-function `&T::f` of type `R (T::*)(Args...)` is transparently handled by the library as if it were a regular function pointer of type `R (*)(T&, Args...)`; similarly for `const T&` and `T&&`.

## `sfn::sequence`

```cpp
template <typename T, typename... Rest>
concept sequencable = functional<T> && (functional<Rest> && ...) && /* ... */;

template <function auto F, function auto... Rest>
requires sequencable<decltype(F), decltype(Rest)...>
inline constexpr auto sequence = /* ... */;
```

Given one or more functions with identical lists of parameter types, `sfn::sequence<f, g, ...>` is a function pointer with behaviour equivalent to calling all of `f`, `g`, ... in sequence with the same list of arguments, and returning the value returned by the last function, if any. In this way, `sfn::sequence` works much like the comma operator.

Since the same set of arguments is reused for each call, none of the parameter types involved may be rvalue-references and all of them must be copy-constructible, unless only a single function is being sequenced, in which case `sfn::sequence<f>` is just `f`.

The concept `sfn::sequencable<F, G...>` checks that an instantiation of `sfn::sequence` would be valid; it is satisfied for `sfn::functional` types `F`, `G` ... such that values of corresponding `sfn::function` types obey the rules above.

If you need to sequence functions that have _similar_ rather than _identical_ parameter lists, you can use `sfn::cast` (below) first to convert them to a compatible signature.

### Example

```cpp
void f(int, int);
int g(int, int);
void h(int);

constexpr int (*)(int, int) fp = sfn::sequence<&f, &g>;
fp(1, 2);                  // equivalent to (f(1, 2), g(1, 2)), returns result of g
// sfn::sequence<&f, &h>;  // error: constraint not satisfied, parameter lists differ

struct Foo {
  void f() const;
  void g() const;
};

constexpr auto* foo_fp = sfn::sequence<&Foo::f, &Foo::g>;
Foo foo;
foo_fp(foo);  // calls foo.f() and foo.g()
```

## `sfn::cast`

```cpp
template <typename Source, typename Target>
concept castable_to = functional<Source> && function_type<Target> && /* ... */;

template <function_type T, function auto F>
requires castable_to<decltype(F), T>
inline constexpr auto cast = /* ... */;
```

`sfn::cast` lets you cast a function pointer from one type to another, as long as the types are reasonably compatible, obtaining a new function pointer that just does "the right thing".

More concretely, consider casting a function pointer `fp` with source function type `R(Args...)` to target type `RT(ArgsT...)` using `sfn::cast<RT(ArgsT...), fp>`:
  * `R` must be convertible to `RT` (`R` can be anything if `RT` is `void`).
  * `ArgsT` may have more elements than `Args`. The casted function will simply not use the additional arguments it receives.
  * `ArgsT` may have fewer elements than `Args`, as long as the missing types are default-constructible. The casted function will fill in missing values by value-initialising them.
  * Each matching element of `ArgsT` must be convertible to the corresponding element of `Args`.

The concept `sfn::castable_to<F, T>` encodes these rules for `sfn::functional` type `F` and `sfn::function_type` target `T`.

Adding the `noexcept` specifier to the the target type forces the type of the resulting function pointer to also have the `noexcept` specificer, regardless of whether it is present on the source type and conversions involved.

If the source and target types are identical modulo `noexcept` specifier, and the target type does not have the `noexcept` specifier, `sfn::cast<T, f>` is just `f`.

### Example

```cpp
int sum(int x, int y) {
  return x + y;
}

constexpr void (*fp)(int, int) = sfn::cast<void(int, int), &sum>;
fp(sum);                                           // calls sum(1, 2), discards result
sfn::cast<int(int), &sum>(1);                      // calls sum(1, 0), returns 1
sfn::cast<int(int, int, int), &sum>(1, 2, 42);     // calls sum(1, 2), returns 3
sfn::cast<float(float, float), &sum>(1.5f, 2.7f);  // calls sum(1, 2), returns 3.f

struct Foo {
  void f();
};
// sfn::cast<void(const Foo&), &Foo::f>;  // error: constraint not satisfied, const Foo&
                                          // not convertible to Foo&
```

## `sfn::bind_front` and `sfn::bind_back`

```cpp
template <typename F, typename... Args>
concept bindable_front = functional<F> && /* ... */;
template <typename F, typename... Args>
concept bindable_back = functional<F> && /* ... */;

template <function auto F, auto... Values>
requires bindable_front<decltype(F), decltype(Values)....>
inline constexpr auto bind_front = /* ... */;

template <function auto F, auto... Values>
requires bindable_back<decltype(F), decltype(Values)....>
inline constexpr auto bind_back = /* ... */;
```

These are compile-time analogues of `std::bind_front` and `std::bind_back` for raw function pointers. `sfn::bind_front<f, values...>` is a pointer to a function whose behaviour is equivalent to invoking `f` with its first N parameters bound to `values`; `sfn::bind_back<f, values...>` is the same for the last N parameters.

As usual, the concepts `sfn::bindable_front<F, Args...>` and `sfn::bindable_back<F, Args...>`
check that instantiations of `sfn::bind_front` and `sfn::bind_back` respectively would be valid; namely, that the function type of `F` has at least as many parameters as there are values to be bound, and that the types of the provided values are convertible to the corresponding parameter types of `F`.

### Example

```cpp
int subtract(int x, int y) {
  return x - y;
}

sfn::bind_front<&subtract, 3>(1);       // calls subtract(3, 1), returns 2
sfn::bind_back<&subtract, 3>(1);        // calls subtract(1, 3), returns -2
// sfn::bind_back<&subtract, 1, 2, 3>;  // error: constaint not satisfied, too many args

struct Foo {
  constexpr Foo() = default;
  void f() const;
};

sfn::bind_front<&Foo::f, Foo{}>();   // equivalent to Foo{}.f()
// sfn::bind_front<&Foo::f, 1>;      // error: constraint not satisfied,
                                     // int not convertible to const Foo&
```

## `sfn::compose_front` and `sfn::compose_back`

```cpp
template <typename G, typename F>
concept composable_front = functional<G> && functional<F> && /* ... */;
template <typename G, typename F>
concept composable_back = functional<G> && functional<F> && /* ... */;
template <typename G, typename F>
concept composable = functional<G> && functional<F> && /* ... */;

template <function auto G, function auto F>
requires composable_front<decltype(G), decltype(F)>
inline constexpr auto compose_front = /* ... */;

template <function auto G, function auto F>
requires composable_back<decltype(G), decltype(F)>
inline constexpr auto compose_back = /* ... */;

template <function auto G, function auto F>
requires composable<decltype(G), decltype(F)>
inline constexpr auto compose = /* ... */;
```

`sfn::compose_front<g, f>` is a pointer to a function that calls `g`, by first calling `f` to obtain the first argument for `g`, converting if necessary, and forwarding the remaining arguments. That is, `sfn::compose_front<g, f>(xs..., ys...)` is equivalent to `g(T(f(xs...)), ys...)` (where the number of elements in `xs` is equal to the number of parameters of `f`, the number of elements in `ys` is equal to the number of parameters of `g` minus one, and `T` is the type of the first parameter of `g`).

Similarly, `sfn::compose_back<g, f>` calls `f` to get the last argument for `g`, so `sfn::compose_back<g, f>(ys..., xs...)` is equivalent to `g(ys..., T(f(xs...)))`.

Constraints `sfn::composable_front<G, F>` and `sfn::composable_back<G, F>` check that the function type of `G` has at least one parameter, and that the return type of the function type of `F` is convertible to the appropriate parameter type of the function type of `G`.

The constraint `sfn::composable<G, F>` is similar, but satisfied only when `G` has _exactly_ one parameter. In this case, `sfn::compose_front<g, f>` and `sfn::compose_back<g, f>` are equivalent, and you can just write `sfn::compose<g, f>`.

### Example

```cpp
int add(int x, int y) {
  return x + y;
}
int subtract(int x, int y) {
  return x - y;
}

constexpr int (*sub_add_left)(int, int, int) = sfn::compose_front<&subtract, &add>;
constexpr int (*sub_add_right)(int, int, int) = sfn::compose_back<&subtract, &add>;
sub_add_left(1, 2, 3);   // returns (1 + 2) - 3 = 0
sub_add_right(1, 2, 3);  // returns 1 - (2 + 3) = -4

int f();
void g(int);
constexpr sfn::ptr<void()> fp = sfn::compose<g, f>;  // fp() is equivalent to g(f())
```

## Notes

### Overhead

The function pointers produced by `sfn` operators are ultimately pointers to `static` member functions of template type instantiations. The template arguments of such an instantiation include the values of the original function pointers passed as input to the operator and so, if definitions are available, e.g. `compose<g, f>` can inline the definitions of `g` and `f` just as a manually-written equivalent function could.

### Move-only types and perfect forwarding

The wrapper functions produced by `sfn` operators `std::move` their arguments into the target whenever it makes sense to do so (more or less, if the parameter type to be forwarded is not an lvalue-reference), so this should all work fine.

### `noexcept` wrappers

The wrapper functions produced by `sfn` operators will be automatically be declared `noexcept` if all of the move constructors, converting constructors and actual input functions involved are themselves declared `noexcept`. `sfn::cast` can also be used to explicitly produce a `noexcept` wrapper, e.g. by `sfn::cast<void(int) noexcept, &f>`.

### `constexpr` functions

The function _pointers_ produced by `sfn` operators are `constexpr` and can be used in constant-evaluated contexts. The functions they _point_ to are also `constexpr` and can be used (i.e. the pointers can be invoked) in constant-evaluated contexts if all of the the move constructors, converting constructors and actual input functions involved are themselves also `constexpr`. For example:

```cpp
constexpr int add(int x, int y) {
  return x + y;
}
static_assert(sfn::bind_front<&add, 1, 2>() == 3);  // OK
```

# <sfn/type_list.h>

## `sfn::list`

```cpp
template <typename...>
struct list {};
template <typename T>
concept type_list = /* satisfied if T is any list<Ts...> */;
```

## Basic operations

```cpp
template <type_list A>
inline constexpr std::size_t size = /* ... */;  // size<list<T, U>> == 2
template <type_list A>
inline constexpr bool empty = !size<A>;         // empty<list<>> == true

template <type_list A, type_list B>
using concat = /* ... */;   // concat<list<T, U>, list<V, W>> == list<T, U, V, W>
template <typename T, type_list A>
using append = /* ... */;   // append<T, list<U, V>> == list<U, V, T>
template <typename T, type_list A>
using prepend = /* ... */;  // prepend<T, list<U, V>> == list<T, U, V>

template <type_list A> requires(!empty<A>)
using drop_front = /* ... */;  // drop_front<list<T, U>> == list<U>
template <type_list A> requires(!empty<A>)
using drop_back = /* ... */;   // drop_back<list<T, U>> == list<T>
template <type_list A> requires(!empty<A>)
using front = /* ... */;       // front<list<T, U>> == T
template <type_list A> requires(!empty<A>)
using back = /* ... */;        // back<list<T, U>> == U

template <type_list A, std::size_t Index>
requires(Index < size<A>)
using get = /* ... */;      // get<list<T, U, V>, 1> == U
template <type_list A, std::size_t Index, std::size_t Size = npos>
requires(Index <= size<A>)
using sublist = /* ... */;  // sublist<list<T, U, V, W>, 1, 2> == list<U, V>
                            // sublist<list<T, U, V, W>, 1> == list<U, V, W>
template <type_list A, std::size_t Index, std::size_t Size = npos>
requires(Index <= size<A>)
using erase = /* ... */;    // erase<list<T, U, V, W>, 1, 2> == list<T, W>
                            // erase<list<T, U, V, W>, 1> == list<T>
template <type_list A, std::size_t... Indices>
requires((Indices < size<A>) && ...)
using select = /* ... */;   // select<list<T, U, V, W>, 3, 0, 1> == list<W, T, U>

template <type_list A, template <typename....> typename Template>
using to = /* ... */;       // to<list<T, U, V>, F> = F<T, U, V>
```

## Algorithms

```cpp
template <template <typename...> typename F, typename... Ts>
using apply = /* ... */;
```

Works with both nested-typedef-style metafunctions and alias-style metafunctions. i.e. `sfn::apply<F, Ts...>` is `F<Ts...>::type` if the member type `type` present, `F<Ts...>` otherwise.

```cpp
template <type_list A, template <typename...> typename P>
inline constexpr bool all_of = /* ... */;
template <type_list A, template <typename...> typename P>
inline constexpr bool any_of = /* ... */;
template <type_list A, template <typename...> typename P>
inline constexpr bool none_of = /* ... */;

template <type_list A, typename T>
inline constexpr std::size_t find = /* ... */;
template <type_list A, template <typename...> typename P>
inline constexpr std::size_t find_if = /* ... */;
template <type_list A, template <typename...> typename P>
inline constexpr std::size_t find_if_not = /* ... */;

template <type_list A, typename T>
inline constexpr std::size_t count = /* ... */;
template <type_list A, template <typename...> typename P>
inline constexpr std::size_t count_if = /* ... */;

template <type_list A, typename T>
using remove = /* ... */;
template <type_list A, template <typename...> typename P>
using remove_if = /* ... */;
```

Should feel familiar to users of `<algorithm>`. For example:
* `sfn::all_of<list<int, long, long long>, std::is_integral>` is `true`
* `sfn::find<list<char, float, int>, float>` is `1`
* `sfn::find_if<list<float, double>, std::is_integral>` is `2`
* `sfn::count<list<int, float, long>, std::is_integral>` is `2`
* `sfn::remove<list<int, float, long, float>, float>` is `list<int, long>`
* `sfn::remove_if<list<float, int, double>, std::is_integral>` is `list<float, double>`

```cpp
template <type_list A, template <typename...> typename P>
using filter = /* ... */;
template <type_list A, template <typename...> typename F>
using map = /* ... */;
```

`sfn::filter` is really just the opposite of `sfn::remove_if`.

`sfn::map` calls `sfn::apply` on each element of the list, for example:
* `sfn::map<list<const int, int, const float>, std::remove_const>` is `list<int, int, float>`
* `sfn::map<list<int, float>, std::vector>` is `list<std::vector<int>, std::vector<float>>`
