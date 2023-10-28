// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "utilities.h"
#include "LoadSensor.h"
#include "kilipili_common.h"
#include <hardware/clocks.h>
#include <hardware/pll.h>
#include <hardware/vreg.h>

// defined by linker:
extern char end;
extern char __HeapLimit;
extern char __StackLimit;
extern char __StackTop;

extern char __scratch_x_start__;
extern char __scratch_x_end__;
extern char __scratch_y_start__;
extern char __scratch_y_end__;
extern char __StackOneTop;	  // = ORIGIN(SCRATCH_X) + LENGTH(SCRATCH_X);
extern char __StackTop;		  // = ORIGIN(SCRATCH_Y) + LENGTH(SCRATCH_Y);
extern char __StackOneBottom; // = __StackOneTop - SIZEOF(.stack1_dummy);
extern char __StackBottom;	  // = __StackTop - SIZEOF(.stack_dummy);

extern char __flash_binary_start;
extern char __flash_binary_end;

// ram from ram_vector_table to end / __HeapLimit / __StackLimit / __StackTop
extern char __data_start__; // TODO: ram_vector_table


// =========================================

namespace kio
{

size_t flash_size() { return PICO_FLASH_SIZE_BYTES; }

size_t flash_used() { return size_t(&__flash_binary_end - &__flash_binary_start); }

size_t flash_free() { return flash_size() - flash_used(); }

size_t heap_size() { return size_t(&__StackLimit - &end); }

size_t calc_heap_free()
{
	// calculate the size of the biggest free chunk on the heap:

	size_t lo = 0, hi = 256 kB;
	while (hi - lo >= 4)
	{
		size_t m		 = (hi + lo) / 2;
		void* volatile p = malloc(m);
		if (p) lo = m;
		else hi = m;
		free(p);
	}
	return lo;
}

ptr core0_stack_bottom()
{
	//printf("__scratch_y_start__= 0x%08x\n", uint(&__scratch_y_start__));
	//printf("__scratch_y_end__  = 0x%08x\n", uint(&__scratch_y_end__));
	//printf("__StackZeroBottom  = 0x%08x\n", uint(&__StackBottom));

	return &__scratch_y_end__;
}

ptr core1_stack_bottom()
{
	//printf("__scratch_x_start__= 0x%08x\n", uint(&__scratch_x_start__));
	//printf("__scratch_x_end__  = 0x%08x\n", uint(&__scratch_x_end__));
	//printf("__StackOneBottom   = 0x%08x\n", uint(&__StackOneBottom));

	return &__scratch_x_end__;
}

ptr stack_bottom(uint core) { return core == 0 ? core0_stack_bottom() : core1_stack_bottom(); }

ptr core0_stack_end() { return &__StackTop; }

ptr core1_stack_end() { return &__StackOneTop; }

ptr stack_end(uint core) { return core == 0 ? core0_stack_end() : core1_stack_end(); }

size_t stack_free()
{
	// calc remaining free size on stack of current core

	char tmp;
	return size_t(&tmp - stack_bottom(get_core_num()));
}


void print_core() { printf("### Hello core%u ###\n", get_core_num()); }

void print_heap_free(int r)
{
	// print list of all free chunks on the heap:

	uint32 sz = calc_heap_free();
	if (sz == 0) return;

	printf("%s: %u bytes\n", r ? "+fragment" : "heap free", sz);

	void* volatile p = malloc(sz);

	// stdclib malloc sometimes fails to return available memory:
	//if (!p)
	//{
	//	printf("  alloc %u bytes failed\n",sz);
	//	sz = calc_heap_free();
	//	printf("  now available: %u bytes (recalculated)\n", sz);
	//	while (!p && sz) p = malloc(sz--);
	//	printf("  now available: %u bytes (trying hard)\n", ++sz);
	//}

	assert(p);

	print_heap_free(1);
	free(p);
}

void print_stack_free() { printf("core%u stack free: %u bytes\n", get_core_num(), stack_free()); }

void print_core0_scratch_y_usage()
{
	const uintptr_t xa = uintptr_t(&__scratch_y_start__);
	const uintptr_t xe = uintptr_t(&__scratch_y_end__);

	if (xa != xe) printf("0x%08x to 0x%08x: core0 scratch_y\n", xa, xe);
	else printf("core0 scratch_y not used\n");
	printf("0x%08x to 0x%08x: core0 stack\n", uint(core0_stack_bottom()), uint(core0_stack_end()));
}

void print_core1_scratch_x_usage()
{
	const uintptr_t xa = size_t(&__scratch_x_start__);
	const uintptr_t xe = size_t(&__scratch_x_end__);

	if (xa != xe) printf("0x%08x to 0x%08x: core1 scratch_x\n", xa, xe);
	else printf("core1 scratch_x not used\n");
	printf("0x%08x to 0x%08x: core1 stack\n", uint(core1_stack_bottom()), uint(core1_stack_end()));
}

void print_flash_usage()
{
	const size_t fa = uintptr_t(&__flash_binary_start);
	const size_t fe = uintptr_t(&__flash_binary_end);

	printf("0x%08x to 0x%08x: flash, used %u, free %u\n", fa, fe, flash_used(), flash_free());
}

void init_stack_guard()
{
	// fill free stack of current core with magic number:

	uint tmp = 0xa5a5a5a5;
	ptr	 e	 = ptr(&tmp);
	ptr	 p	 = stack_bottom(get_core_num());
	assert(uint(e - p) < 4 kB); // stack pages are 4k

	// don't use memset because the call to memset may push some values which are then overwritten!
	for (; p < e; p += 4) { *reinterpret_cast<volatile uint*>(p) = 0xe5e5e5e5; }
}

void test_stack_guard(uint core)
{
	// test whether a core overflowed the stack recently:

	ptr				 p			= stack_bottom(core);
	constexpr uint32 deadbeef[] = {0xe5e5e5e5, 0xe5e5e5e5, 0xe5e5e5e5, 0xe5e5e5e5};
	if (memcmp(p, deadbeef, sizeof(deadbeef))) panic("core %u: stack overflow", core);
}

uint calc_stack_guard_min_free(uint core)
{
	// calculate the minimum amount of free memory on stack at any time
	// since the stack guard was initialized:

	ptr	 p = stack_bottom(core);
	uint n = 0;
	while (*p++ == 0xe5) { n++; }
	return n;
}

void print_system_info(uint)
{
	print_core();
	printf("total heap size = %u\n", heap_size());
	print_heap_free();
	print_stack_free();
	print_core0_scratch_y_usage();
	print_core1_scratch_x_usage();
	print_flash_usage();

	// ram    @ 0x20000000 - 0x20040000
	// stack1 @ 0x20040000 - 0x20041000
	// stack0 @ 0x20041000 - 0x20042000
	// flash  @ 0x10000000 - 0x10200000 (address of cached access, 2MB)

	//printf("core0 stack base @ 0x%08x\n", uint(get_stack_base(0)));
	//printf("core1 stack base @ 0x%08x\n", uint(get_stack_base(1)));

	printf("system clock = %u MHz\n", clock_get_hz(clk_sys) / 1000000);
}


// ===============================================================

struct sysclock_params
{
	uint vco, div1, div2, err;
};

constexpr sysclock_params calc_sysclock_params(uint f)
{
	// the system clock is derived from the crystal by scaling up for the vco and two 3-bit dividers,
	// thus system_clock = crystal * vco_cnt / div1 / div2
	// with vco_cnt = 16 .. 320
	// limited by min_vco and max_vco frequency 750 MHz and 1600 MHz resp.
	// => vco_cnt = 63 .. 133
	//      div1 = 1 .. 7
	//      div2 = 1 .. 7
	// the crystal on the Pico board is 12 MHz
	// lowest  possible sys_clock = 12*63/7/7 ~ 15.428 MHz
	// highest possible sys_clock = 12*133/1/1 ~ 1596 MHz
	//
	// possible full MHz clocks:
	//  63 .. 133 MHz	in 1 MHz steps	(f_out = 12 MHz * vco / 12)
	// 126 .. 266 MHz	in 2 MHz steps  (f_out = 12 MHz * vco / 6)
	// 189 .. 399 MHz	in 3 MHz steps	(f_out = 12 MHz * vco / 4)
	// 252 .. 532 MHz   in 4 MHz steps  (f_out = 12 MHz * vco / 3)

	// 275 MHz is not possible ( 275 > 266 && 275%3 != 0 && 275%4 != 0 )
	// 280 MHz ok              ( 280 > 266 && 280%4 == 0 => vco = 280/4 && div by 12/4 )
	// 300 MHz freezes

	constexpr uint	xtal = XOSC_KHZ / PLL_COMMON_REFDIV * 1000; // 12 MHz
	sysclock_params best {0, 0, 0, 666 MHz};

	uint div_min = (PICO_PLL_VCO_MIN_FREQ_KHZ * 1000 + f - 1) / f; // round up
	uint div_max = (PICO_PLL_VCO_MAX_FREQ_KHZ * 1000) / f;		   // round down

	for (uint div1 = 2; div1 <= 7; div1++)
		for (uint div2 = 1; div2 <= div1; div2++)
		{
			uint div = div1 * div2;
			if (div < div_min) continue;
			if (div > div_max) break;

			uint vco = (f * div + xtal / 2) / xtal * xtal; // ~ 1 GHz
			uint err = abs(int(f) - int(vco / div));

			if (err >= best.err) continue;

			best.div1 = div1;
			best.div2 = div2;
			best.vco  = vco;
			best.err  = err;
			if (err == 0) return best;
		}
	return best;
}

static_assert(calc_sysclock_params(10 MHz).err == 666 MHz);
static_assert(calc_sysclock_params(15300 kHz).err == 666 MHz);

static_assert(calc_sysclock_params(15400 kHz).err > 0);
static_assert(calc_sysclock_params(15400 kHz).err < 30 kHz);
static_assert(calc_sysclock_params(15400 kHz).vco <= 12 MHz * 63);
static_assert(calc_sysclock_params(15400 kHz).div1 == 7);
static_assert(calc_sysclock_params(15400 kHz).div2 == 7);

static_assert(calc_sysclock_params(16 MHz).err < 100 kHz);
static_assert(calc_sysclock_params(16 MHz).vco == 16 MHz * 7 * 7 - 4 MHz);
static_assert(calc_sysclock_params(16 MHz).div1 == 7);
static_assert(calc_sysclock_params(16 MHz).div2 == 7);

static_assert(calc_sysclock_params(20 MHz).err == 0);
static_assert(calc_sysclock_params(20 MHz).vco == 20 MHz * 7 * 6);
static_assert(calc_sysclock_params(20 MHz).div1 == 7);
static_assert(calc_sysclock_params(20 MHz).div2 == 6);

static_assert(calc_sysclock_params(125 MHz).err == 0);
static_assert(calc_sysclock_params(125 MHz).vco == 125 MHz * 12);
static_assert(calc_sysclock_params(125 MHz).div1 == 4);
static_assert(calc_sysclock_params(125 MHz).div2 == 3);

static_assert(calc_sysclock_params(250 MHz).err == 0);
static_assert(calc_sysclock_params(250 MHz).vco == 250 MHz * 6);
static_assert(calc_sysclock_params(250 MHz).div1 == 3);
static_assert(calc_sysclock_params(250 MHz).div2 == 2);

static_assert(calc_sysclock_params(280 MHz).err == 0);
static_assert(calc_sysclock_params(280 MHz).vco == 280 MHz * 3);
static_assert(calc_sysclock_params(280 MHz).div1 == 3);
static_assert(calc_sysclock_params(280 MHz).div2 == 1);

static_assert(calc_sysclock_params(273 MHz).err == 0);
static_assert(calc_sysclock_params(273 MHz).vco == 273 MHz * 4);
static_assert(calc_sysclock_params(273 MHz).div1 == 2);
static_assert(calc_sysclock_params(273 MHz).div2 == 2);

static_assert(calc_sysclock_params(274 MHz).err > 0);
static_assert(calc_sysclock_params(274 MHz).err == 400 kHz);
static_assert(calc_sysclock_params(274 MHz).vco == 274 MHz * 5 - 2 MHz);
static_assert(calc_sysclock_params(274 MHz).div1 == 5);
static_assert(calc_sysclock_params(274 MHz).div2 == 1);

static_assert(calc_sysclock_params(275 MHz).err > 0);
static_assert(calc_sysclock_params(275 MHz).err == 1 MHz);
static_assert(calc_sysclock_params(275 MHz).vco == 275 MHz * 4 + 4 MHz);
static_assert(calc_sysclock_params(275 MHz).div1 == 2);
static_assert(calc_sysclock_params(275 MHz).div2 == 2);

static_assert(calc_sysclock_params(276 MHz).err == 0);
static_assert(calc_sysclock_params(276 MHz).vco == 276 MHz * 4);
static_assert(calc_sysclock_params(276 MHz).div1 == 2);
static_assert(calc_sysclock_params(276 MHz).div2 == 2);

static_assert(calc_sysclock_params(300 MHz).err == 0);
static_assert(calc_sysclock_params(300 MHz).vco == 300 MHz * 4);
static_assert(calc_sysclock_params(300 MHz).div1 == 2);
static_assert(calc_sysclock_params(300 MHz).div2 == 2);

static_assert(calc_sysclock_params(325 MHz).err == 1 MHz);
static_assert(calc_sysclock_params(325 MHz).vco == 325 MHz * 4 - 4 MHz);
static_assert(calc_sysclock_params(325 MHz).div1 == 2);
static_assert(calc_sysclock_params(325 MHz).div2 == 2);


Error set_system_clock(uint32 new_clock, uint32 max_error)
{
	static constexpr uint32 maxf[] = {
		20 MHz,	 // VREG 0.85	min
		40 MHz,	 // VREG 0.90
		60 MHz,	 // VREG 0.95
		80 MHz,	 // VREG 1.00
		100 MHz, // VREG 1.05
		130 MHz, // VREG 1.10	default on power up
		160 MHz, // VREG 1.15
		190 MHz, // VREG 1.20
		220 MHz, // VREG 1.25
		999 MHz, // VREG 1.30	max
	};

	uint i = 0; // idx in maxf[]
	while (new_clock > maxf[i]) i++;

	uint cv = 85 + i * 5; // centivolt
	printf("set system clock = %u MHz and Vcore = %u.%02u V\n", new_clock / 1000000, cv / 100, cv % 100);

	uint32 old_clock = get_system_clock();
	if (new_clock == old_clock) return NO_ERROR;
	if (new_clock > SYSCLOCK_fMAX) return UNSUPPORTED_SYSTEM_CLOCK;

	auto params = calc_sysclock_params(new_clock);
	if (params.err)
		printf(
			"new system clock = %u kHz, error = %i kHz (0.%03i%%)\n", params.vco / params.div1 / params.div2,
			(params.err + 500) / 1000, (params.err * 1000 + new_clock / 200) / (new_clock / 100));
	if (params.err > max_error) return UNSUPPORTED_SYSTEM_CLOCK;
	stdio_flush();

	if (new_clock < old_clock)
	{
		sleep_ms(5);
		set_sys_clock_pll(params.vco, params.div1, params.div2);
		sleep_ms(1);
	}

	vreg_voltage voltage = vreg_voltage(VREG_VOLTAGE_0_85 + i);
	vreg_set_voltage(voltage);

	if (new_clock > old_clock)
	{
		sleep_ms(5);
		set_sys_clock_pll(params.vco, params.div1, params.div2);
		sleep_ms(1);
	}

	clock_set_reported_hz(clk_sys, new_clock); // in case actual frequency != requested

	// clk_peri has changed:
	setup_default_uart();
	LoadSensor::recalibrate();

	return NO_ERROR;
}


void sleep_us(int delay_usec) noexcept
{
	if (delay_usec > 0)
	{
		idle_start();
		::sleep_us(uint(delay_usec));
		idle_end();
	}
}

void wfe_or_timeout(int timeout_usec) noexcept
{
	if (timeout_usec > 0)
	{
		idle_start();
		::best_effort_wfe_or_timeout(from_us_since_boot(time_us_64() + uint(timeout_usec)));
		idle_end();
	}
}


} // namespace kio
