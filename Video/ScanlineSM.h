// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Scanline.h"
#include "ScanlinePioProgram.h"
#include "VgaMode.h"
#include "VideoQueue.h"
#include "errors.h"
#include "scanvideo_options.h"
#include <atomic>
#include <pico/sem.h>


namespace kio::Video
{

class ScanlineSM
{
public:
	VgaMode							video_mode;
	const ScanlinePioProgram* const pio_program = &video_24mhz_composable;
	Scanline*						missing_scanline;

	uint wait_index; // address of PIO_WAIT_IRQ4 in pio program
	bool in_vblank;

	uint y_scale; // for line repetition in low-res modes
	uint y_repeat_countdown;

	Scanline*  current_scanline = nullptr;
	ScanlineID current_id;		  // frame,line: current (or the last) scanline displayed
	ScanlineID last_generated_id; // frame,line: last scanline handed out for generating

	semaphore_t vblank_begin;

	ScanlineSM() = default;

	Error setup(const VgaMode* mode, const VgaTiming* timing);
	void  start(); // start or restart
	void  stop();

	bool in_hblank();
	void waitForVBlank() { sem_acquire_blocking(&vblank_begin); }
	void waitForScanline(ScanlineID);


	// Get next scanline for generating. The `id` field indicates the scanline and frame number.
	// return nullptr if none available
	Scanline* getScanlineForGenerating();

	// Return a scanline that has been generated
	void pushGeneratedScanline() noexcept { video_queue.push_full(); }
	void pushGeneratedScanline(Scanline& s) noexcept { video_queue.push_full(s); } // with assert

	//private:
	void prepare_for_active_scanline() noexcept;
	void prepare_for_vblank_scanline() noexcept;
	void abort_all_scanline_sms() noexcept;
};


extern VideoQueue		   video_queue;
extern ScanlineSM		   scanline_sm;
extern std::atomic<uint32> scanlines_missed;

} // namespace kio::Video
