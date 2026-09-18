// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD 2-clause license
// https://spdx.org/licenses/BSD-2-Clause.html

// based on:
// Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
// SPDX-License-Identifier: BSD-3-Clause


// pull in board description file for targeted board:
#ifndef MAKE_TOOLS
  #include <pico.h>
#elif defined PICO_BOARD_H
  #define pico_board_cmake_set(...)
  #define pico_board_cmake_set_default(...)
  #include PICO_BOARD_H
#endif


#ifdef SDCARD_SPI

  #include "common/cdefs.h"
  #include "common/standard_types.h"
  #include "common/system_clock.h"
  #include "sdcard_spi.h"
  #include <cstdio>
  #include <hardware/gpio.h>
  #include <hardware/pio.h>
  #include <hardware/spi.h>

  #if !defined SDCARD_SPI_RX_PIN || !defined SDCARD_SPI_TX_PIN || !defined SDCARD_SPI_CS_PIN || \
	  !defined SDCARD_SPI_CLK_PIN
	#error "some settings for SDCard are missing"
  #endif
  #ifndef SDCARD_SPI_CLOCK
	#define SDCARD_SPI_CLOCK 20 MHz
  #endif


namespace kilipili::Devices
{

// clang-format off
  // assert unchanged definition, then replace c-style casting macros with c++ version:
  #define spi0_hw ((spi_hw_t *)SPI0_BASE)
  #define spi1_hw ((spi_hw_t *)SPI1_BASE)
  #define spi0 ((spi_inst_t *)spi0_hw)
  #define spi1 ((spi_inst_t *)spi1_hw)
// clang-format on
  #undef spi0
  #undef spi1
  #define spi0 reinterpret_cast<spi_inst_t*>(SPI0_BASE)
  #define spi1 reinterpret_cast<spi_inst_t*>(SPI1_BASE)


// ----------------------------------------------
// pio program spi_cpha0:

static constexpr uint16 spi_cpha0_program_instructions[] = {
	//		       .wrap_target
	0x6101, //  0: out    pins, 1         side 0 [1]
	0x5101, //  1: in     pins, 1         side 1 [1]
			//     .wrap
};

static constexpr pio_program spi_cpha0_program = {
	.instructions = spi_cpha0_program_instructions,
	.length		  = 2,
	.origin		  = -1,
	.pio_version  = 0,
  #if PICO_PIO_VERSION > 0
	.used_gpio_ranges = 0x0
  #endif
};

static inline pio_sm_config spi_cpha0_program_get_default_config(uint offset)
{
	constexpr uint spi_cpha0_wrap_target = 0;
	constexpr uint spi_cpha0_wrap		 = 1;
	pio_sm_config  c					 = pio_get_default_sm_config();
	sm_config_set_wrap(&c, offset + spi_cpha0_wrap_target, offset + spi_cpha0_wrap);
	sm_config_set_sideset(&c, 1, false, false);
	return c;
}


// --------------------------------------
// pio program spi_cpha1:

static constexpr uint16 spi_cpha1_program_instructions[] = {
	//		       .wrap_target
	0x6021, //  0: out    x, 1            side 0
	0xb101, //  1: mov    pins, x         side 1 [1]
	0x4001, //  2: in     pins, 1         side 0
			//     .wrap
};

static constexpr pio_program spi_cpha1_program = {
	.instructions = spi_cpha1_program_instructions,
	.length		  = 3,
	.origin		  = -1,
	.pio_version  = 0,
  #if PICO_PIO_VERSION > 0
	.used_gpio_ranges = 0x0
  #endif
};

static inline pio_sm_config spi_cpha1_program_get_default_config(uint offset)
{
	constexpr uint spi_cpha1_wrap_target = 0;
	constexpr uint spi_cpha1_wrap		 = 2;
	pio_sm_config  c					 = pio_get_default_sm_config();
	sm_config_set_wrap(&c, offset + spi_cpha1_wrap_target, offset + spi_cpha1_wrap);
	sm_config_set_sideset(&c, 1, false, false);
	return c;
}


// ----------------------------------
// find spi instance:

static constexpr int spi_rx_pin	 = SDCARD_SPI_RX_PIN;
static constexpr int spi_tx_pin	 = SDCARD_SPI_TX_PIN;
static constexpr int spi_clk_pin = SDCARD_SPI_CLK_PIN;

static constexpr int spi_instance_for_pins(int rx, int clk, int tx) noexcept
{
	// instance: pin & 8
	// function: rx & 3 = 0
	// function: clk& 3 = 2
	// function: tx & 3 = 3
	if (((rx & 11) == 0) && ((clk & 11) == 2) && ((tx & 11) == 3)) return 0;
	if (((rx & 11) == 8) && ((clk & 11) == 10) && ((tx & 11) == 11)) return 1;
	return -1;
}

static constexpr int spi_idx = spi_instance_for_pins(spi_rx_pin, spi_clk_pin, spi_tx_pin);
  #define spi (spi_idx >= 0 ? spi_idx ? spi1 : spi0 : nullptr)


// ----------------------------------------------
// pio_spi:

static constexpr uint32 spi_clock = SDCARD_SPI_CLOCK;
static_assert(spi_clock <= 25 MHz);

static constexpr spi_cpha_t			cpha		= SPI_CPHA_0;
static constexpr spi_cpol_t			cpol		= SPI_CPOL_0;
static constexpr uint				n_bits		= 8;
static constexpr const pio_program* spi_program = cpha ? &spi_cpha1_program : &spi_cpha0_program;


struct pio_spi_inst
{
	pio_hw_t* pio {nullptr};
	uint	  sm {0};

	void init(uint prog_offs, uint clk_pin, uint tx_pin, uint rx_pin);
	void write_blocking(const uint8* src, size_t len);
	void read_blocking(uint8* dst, size_t len);
	void write_read_blocking(const uint8* src, uint8* dst, size_t len);
};

static pio_spi_inst pio_spi;

static void debugstr_spi_clock(float clkdiv)
{
	constexpr uint cc_per_bit = 4;
	uint32_t	   div_int;
	uint8		   div_frac8;
	pio_calculate_clkdiv8_from_float(clkdiv, &div_int, &div_frac8);

	debugstr("pio_spi: clock divider = %u:%u\n", div_int, div_frac8);
	uint div = div_int * 256 + div_frac8;
	debugstr("pio_spi: clock = %u\n", get_system_clock() / div * 256 / cc_per_bit);
}

void pio_spi_inst::init(uint prog_offs, uint clk_pin, uint tx_pin, uint rx_pin)
{
	pio_sm_config c = cpha ? spi_cpha1_program_get_default_config(prog_offs) : //
							 spi_cpha0_program_get_default_config(prog_offs);

	sm_config_set_out_pins(&c, tx_pin, 1);
	sm_config_set_in_pins(&c, rx_pin);
	sm_config_set_sideset_pins(&c, clk_pin);
	// MSB-first: shift to left, auto push/pull, threshold=nbits
	sm_config_set_out_shift(&c, false, true, n_bits);
	sm_config_set_in_shift(&c, false, true, n_bits);
	constexpr uint cc_per_bit = 4;
	float		   clkdiv	  = max(float(get_system_clock()) / float(spi_clock * cc_per_bit), 1.0f);
	sm_config_set_clkdiv(&c, clkdiv);
	if constexpr (debug) debugstr_spi_clock(clkdiv);

	// TX, SCK output are low, RX is input
	pio_sm_set_pins_with_mask(pio, sm, 0, (1u << clk_pin) | (1u << tx_pin));
	pio_sm_set_pindirs_with_mask(
		pio, sm, (1u << clk_pin) | (1u << tx_pin), (1u << clk_pin) | (1u << tx_pin) | (1u << rx_pin));
	pio_gpio_init(pio, tx_pin);
	pio_gpio_init(pio, rx_pin);
	pio_gpio_init(pio, clk_pin);

	// set pullup resistor on input pin:
	gpio_pull_up(rx_pin);

	// invert CLK depending on CPOL:
	gpio_set_outover(clk_pin, cpol ? GPIO_OVERRIDE_INVERT : GPIO_OVERRIDE_NORMAL);

	// SPI is synchronous, so bypass input synchroniser to reduce input delay:
	hw_set_bits(&pio->input_sync_bypass, 1u << rx_pin);

	pio_sm_init(pio, sm, prog_offs, &c);
	pio_sm_set_enabled(pio, sm, true);
}

void sysclockChanged(uint32 new_clock) noexcept
{
	// callback for system_clock:
	// TODO: don't define sysclockChanged() when pio_spi is not used

	if constexpr (spi_idx < 0) // pio_spi ?
	{
		if (pio_spi.pio)
		{
			constexpr uint cc_per_bit = 4;
			float		   clkdiv	  = max(float(get_system_clock()) / float(spi_clock * cc_per_bit), 1.0f);
			pio_sm_set_clkdiv(pio_spi.pio, pio_spi.sm, clkdiv);
			if constexpr (debug) debugstr_spi_clock(clkdiv);
		}
	}
}

void pio_spi_inst::write_blocking(const uint8* src, size_t len)
{
	// Do 8 bit accesses on FIFO, so that write data is byte-replicated.
	// This gets us the left-justification for free (for MSB-first shift-out)

	size_t	 tx_remain = len, rx_remain = len;
	io_wo_8* txfifo = reinterpret_cast<io_wo_8*>(&pio->txf[sm]);
	io_ro_8* rxfifo = reinterpret_cast<io_ro_8*>(&pio->rxf[sm]);

	while (tx_remain || rx_remain)
	{
		if (tx_remain && !pio_sm_is_tx_fifo_full(pio, sm))
		{
			*txfifo = *src++;
			--tx_remain;
		}
		if (rx_remain && !pio_sm_is_rx_fifo_empty(pio, sm))
		{
			(void)*rxfifo;
			--rx_remain;
		}
	}
}

void pio_spi_inst::read_blocking(uint8* dst, size_t len)
{
	size_t	 tx_remain = len, rx_remain = len;
	io_wo_8* txfifo = reinterpret_cast<io_wo_8*>(&pio->txf[sm]);
	io_ro_8* rxfifo = reinterpret_cast<io_ro_8*>(&pio->rxf[sm]);

	while (tx_remain || rx_remain)
	{
		if (tx_remain && !pio_sm_is_tx_fifo_full(pio, sm))
		{
			*txfifo = 0xff;
			--tx_remain;
		}
		if (rx_remain && !pio_sm_is_rx_fifo_empty(pio, sm))
		{
			*dst++ = *rxfifo;
			--rx_remain;
		}
	}
}

void pio_spi_inst::write_read_blocking(const uint8* src, uint8* dst, size_t len)
{
	size_t	 tx_remain = len, rx_remain = len;
	io_wo_8* txfifo = reinterpret_cast<io_wo_8*>(&pio->txf[sm]);
	io_ro_8* rxfifo = reinterpret_cast<io_ro_8*>(&pio->rxf[sm]);

	while (tx_remain || rx_remain)
	{
		if (tx_remain && !pio_sm_is_tx_fifo_full(pio, sm))
		{
			*txfifo = *src++;
			--tx_remain;
		}
		if (rx_remain && !pio_sm_is_rx_fifo_empty(pio, sm))
		{
			*dst++ = *rxfifo;
			--rx_remain;
		}
	}
}

void sdcard_spi_read_blocking(uint8* data, size_t cnt) noexcept
{
	if constexpr (spi_idx >= 0) spi_read_blocking(spi, 0xff, data, cnt);
	else pio_spi.read_blocking(data, cnt);
}

void sdcard_spi_write_blocking(const uint8* data, size_t cnt) noexcept
{
	if constexpr (spi_idx >= 0) spi_write_blocking(spi, data, cnt);
	else pio_spi.write_blocking(data, cnt);
}

void sdcard_spi_write_read_blocking(const uint8* src, uint8* dest, size_t cnt) noexcept
{
	if constexpr (spi_idx >= 0) spi_write_read_blocking(spi, src, dest, cnt);
	else pio_spi.write_read_blocking(src, dest, cnt);
}

void sdcard_spi_init() noexcept
{
	if constexpr (spi_idx >= 0) // use regular spi:
	{
		static bool initialized = false;
		if (initialized) return;
		initialized = true;

		debugstr("sdcard: using spi%i\n", spi_idx);
		spi_init(spi, spi_clock);
		spi_set_format(spi, 8, cpol, cpha, SPI_MSB_FIRST);
		gpio_set_function(spi_rx_pin, GPIO_FUNC_SPI);
		gpio_pull_up(spi_rx_pin);
		gpio_set_function(spi_clk_pin, GPIO_FUNC_SPI);
		gpio_set_function(spi_tx_pin, GPIO_FUNC_SPI);
	}
	else // use pio_spi:
	{
		assert(pio_spi.pio == nullptr); // prevent dbl request of resources
		uint prog_offs;
		// set pio_spi.pio, pio_spi.sm and prog_offs:
		pio_claim_free_sm_and_add_program(spi_program, &pio_spi.pio, &pio_spi.sm, &prog_offs);
		if (pio_spi.pio == nullptr) panic("sdcard: pio = nullptr");
		debugstr("sdcard: using pio%u sm%u\n", pio_get_index(pio_spi.pio), pio_spi.sm);
		pio_spi.init(prog_offs, spi_clk_pin, spi_tx_pin, spi_rx_pin);
	}
}

} // namespace kilipili::Devices

#endif

/*

















































































*/
