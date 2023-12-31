// Copyright (c) 2022 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once


#define DEFAULT_COPY(T)             \
  T(const T&)			 = default; \
  T& operator=(const T&) = default

#define DEFAULT_MOVE(T)                 \
  T(T&&) noexcept			 = default; \
  T& operator=(T&&) noexcept = default

#define NO_COPY(T)                 \
  T(const T&)			 = delete; \
  T& operator=(const T&) = delete

#define NO_MOVE(T)            \
  T(T&&)			= delete; \
  T& operator=(T&&) = delete

#define NO_COPY_MOVE(T) \
  NO_COPY(T);           \
  NO_MOVE(T)
