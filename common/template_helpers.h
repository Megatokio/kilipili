// Copyright (c) 2018 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"
#include <type_traits>


namespace kilipili
{

// ######################################
// integer with size:

// clang-format off
template<int sz> struct _int_with_size{using type=int8;};
template<> struct _int_with_size<2>{using type=int16;};
template<> struct _int_with_size<4>{using type=int32;};
template<> struct _int_with_size<8>{using type=int64;};

template<int sz> struct _uint_with_size{using type=uint8;};
template<> struct _uint_with_size<2>{using type=uint16;};
template<> struct _uint_with_size<4>{using type=uint32;};
template<> struct _uint_with_size<8>{using type=uint64;};
// clang-format on

template<int sz>
using int_with_size = _int_with_size<sz>::type;
template<int sz>
using uint_with_size = _uint_with_size<sz>::type;

static_assert(sizeof(int_with_size<8>) == 8);
static_assert(sizeof(uint_with_size<4>) == 4);


// ######################################
// select type T1 or T2:

template<bool, typename T1, typename T2>
struct _select_type
{
	typedef T2 type;
};
template<typename T1, typename T2>
struct _select_type<true, T1, T2>
{
	typedef T1 type;
};
template<bool f, typename T1, typename T2>
using select_type = typename _select_type<f, T1, T2>::type;


// ######################################

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


// ######################################

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


// ######################################

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

}; // namespace kilipili


/*









*/
