// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "utilities.h"
#include "PwmLoadSensor.h"
#include <hardware/clocks.h>
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

	char tmp;
	ptr	 e = &tmp;
	ptr	 p = stack_bottom(get_core_num());
	assert(uint(e - p) < 4 kB); // stack pages are 4k

	memset(p, 0xe5, uint(e - p));
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


// note: drift-free definitions need static var initialized to current time:
// static uint32 timeout = time_us_32();
#define SLEEP_US(usec) \
  for (timeout += usec; int(timeout - time_us_32()) > 0;) WAIT()
#define SLEEP_MS(msec) SLEEP_US(msec * 1000)
// definitions which allow drift:
// static uint32 timeout;
#define SLEEPY_US(usec) \
  for (timeout = time_us_32() + usec; int(timeout - time_us_32()) > 0;) WAIT()
#define SLEEPY_MS(msec) SLEEPY_US(msec * 1000)


#define BEGIN             \
  static uint _state = 0; \
  switch (_state)         \
  {                       \
  default:
#define FINISH \
  return -1;   \
  }
#define WAIT()         \
  do {                 \
	_state = __LINE__; \
	return 0;          \
  case __LINE__:;      \
  }                    \
  while (0)


int sm_print_load()
{
	static uint32 timeout = time_us_32();

	BEGIN
	{
		loadsensor.start();

		printf("sys clock = %u Hz\n", loadsensor.sys_clock);
		printf("pwm clock = %u Hz\n", uint(loadsensor.pwm_freq));

		for (;;)
		{
			SLEEP_MS(10000);

			loadsensor.printLoad(0);
			loadsensor.printLoad(1);
		}
	}
	FINISH
}


// ===============================================================

uint32 system_clock = 125 MHz;

Error checkSystemClock(uint32 new_clock, uint* f_vco, uint* div1, uint* div2)
{
	// the system clock is derived from the crystal by scaling up for the vco and 2 3-bit dividers,
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
	// 16 .. 133 MHz	in 1 MHz steps	(f_out = 12 MHz * vco / 12)
	// 32 .. 266 MHz	in 2 MHz steps  (f_out = 12 MHz * vco / 6)
	// 48 .. 399 MHz	in 3 MHz steps	(f_out = 12 MHz * vco / 4)

	// 275 MHz is not possible ( 275 > 266 && 275%2 != 0 && 275%3 != 0 )
	// 280 MHz ok              ( 280 > 266 && 280%4 == 0 => vco = 280/4 && div by 12/4 )
	// 300 MHz freezes

	if (new_clock > SYSCLOCK_fMAX) return UNSUPPORTED_SYSTEM_CLOCK;
	bool success = check_sys_clock_khz(new_clock / 1000, f_vco, div1, div2);
	return success ? NO_ERROR : UNSUPPORTED_SYSTEM_CLOCK;
}

Error checkSystemClock(uint32 new_clock)
{
	uint f_vco, div1, div2;
	return checkSystemClock(new_clock, &f_vco, &div1, &div2);
}

Error setSystemClock(uint32 new_clock)
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

	//	uint zv = 85 + i * 5;	// centivolt
	//	printf("set system clock = %u MHz and Vcore = %u.%02u V\n", new_clock/1000000, zv/100, zv%100);

	uint32& old_clock = system_clock;
	if (new_clock == old_clock)
	{
		//		printf("system clock: no change\n");
		return NO_ERROR;
	}

	uint  f_vco, div1, div2;
	Error err = checkSystemClock(new_clock, &f_vco, &div1, &div2);
	if (err)
	{
		//		printf("system clock: %s\n",err);
		return err;
	}

	if (new_clock < old_clock)
	{
		set_sys_clock_pll(f_vco, div1, div2);
		sleep_ms(5);
	}

	stdio_flush();
	vreg_voltage voltage = vreg_voltage(VREG_VOLTAGE_0_85 + i);
	vreg_set_voltage(voltage);

	if (new_clock > old_clock)
	{
		sleep_ms(5);
		set_sys_clock_pll(f_vco, div1, div2);
	}

	old_clock = new_clock;

	// clk_peri has changed:
	setup_default_uart();
	loadsensor.recalibrate();
	//	printf("system clock: set to %u MHz\n",new_clock/1000000);

	return NO_ERROR;
}

} // namespace kio
