// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "AudioSource.h"
#include <cmath>

namespace kio::Audio
{

constexpr double steppi = 3.1415926535898 / 2 / nelem_quarter_sine;

#define QS(i) uint16(sin(i* steppi) * 0xffff)

const uint16 quarter_sine[nelem_quarter_sine + 1] = {
	QS(0), QS(1),  QS(2),  QS(3),  QS(4),  QS(5),  QS(6),  QS(7),  QS(8),
	QS(9), QS(10), QS(11), QS(12), QS(13), QS(14), QS(15), QS(16),
};


} // namespace kio::Audio

/*


















*/
