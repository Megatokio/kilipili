// Copyright (c) 2021 - 2023 kio@little-bat.de
// BSD 2-clause license
// https://spdx.org/licenses/BSD-2-Clause.html

#pragma once

using Error = const char*;

constexpr Error	   NO_ERROR = nullptr;
extern const Error OUT_OF_MEMORY;
extern const Error UNSUPPORTED_SYSTEM_CLOCK; // utilities

//extern const Error NOT_A_NUMBER;
//extern const Error UNEXPECTED_FUP;
//extern const Error TRUNCATED_CHAR;
//extern const Error NOT_IN_DEST_CHARSET;
//extern const Error BROKEN_ESCAPE_CODE;
//
//extern const Error VIDEO_SETUP_FAILED;
//extern const Error ILLEGAL_VIDEOMODE;
//extern const Error UNSUPPORTED_SCREENSIZE;
//extern const Error UNSUPPORTED_VIDEOMODE;
//extern const Error NOT_ENOUGH_RAM_FOR_VIDEOMODE;
