// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include <hardware/gpio.h>


namespace kio
{

template<unsigned int pin>
class Led
{
public:
	Led() noexcept
	{
		gpio_init(pin);
		gpio_set_dir(pin, GPIO_OUT);
	}

	void set(bool f) noexcept { gpio_put(pin, f); }

	void toggle() noexcept { gpio_xor_mask(1 << pin); }
};


} // namespace kio
