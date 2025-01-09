#pragma once
// Copyright (c) 2018 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include <type_traits>

namespace kio
{ 

// select type T1 or T2:
template<bool, typename T1, typename T2> struct _select_type { typedef T2 type; };
template<typename T1, typename T2> struct _select_type<true, T1, T2> { typedef T1 type; };
template<bool f, typename T1, typename T2>
using select_type = typename _select_type<f,T1,T2>::type;


template<typename T>
struct _has_operator_star
{
	// test whether type T has member function operator*():
	// has_oper_star<T>::value = true|false

	struct Foo
	{};
	template<typename C>
	static Foo test(...);
	template<typename C>
	static decltype(*std::declval<C>()) test(int);

	static constexpr bool value = !std::is_same<Foo, decltype(test<T>(99))>::value;
};

template<typename T>
constexpr bool has_operator_star = _has_operator_star<T>::value;

static_assert(has_operator_star<int*>, "");
static_assert(!has_operator_star<int>, "");
static_assert(!has_operator_star<int&>, "");

template<typename T>
struct _has_operator_lt
{
	// test whether function lt() for type T exists:
	// has_operator_lt<T>::value = true|false

	struct Foo
	{};
	template<typename C>
	static Foo test(...);
	template<typename C>
	static decltype(lt(std::declval<C>(), std::declval<C>())) test(int);

	static constexpr bool value = !std::is_same<Foo, decltype(test<T>(99))>::value;
};
 
template<typename T>
constexpr bool has_operator_lt = _has_operator_lt<T>::value;

template<typename T>
struct _has_operator_eq
{
	// test whether function eq() for type T exists:
	// has_operator_eq<T>::value = true|false

	struct Foo
	{};
	template<typename C>
	static Foo test(...);
	template<typename C>
	static decltype(eq(std::declval<C>(), std::declval<C>())) test(int);

	static constexpr bool value = !std::is_same<Foo, decltype(test<T>(99))>::value;
};

template<typename T>
constexpr bool has_operator_eq = _has_operator_eq<T>::value;

}; // namespace kio
