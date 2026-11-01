// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "audio/AudioController.h"


namespace kilipili
{

// replacement API of AudioController:

extern void	 setAudioSampleFrequency(float f) noexcept;
extern void	 setAudioMaxLatency(uint32 msec) noexcept;
extern float getAudioSampleFrequency() noexcept;

//extern void	 beep(float frequency_hz, float volume, uint32 duration_ms);
//extern bool	 isAudioRunning() noexcept;

extern void	 startAudio(bool with_timer) noexcept;
extern void	 stopAudio(bool remove_audio_sources) noexcept;
extern int32 fillAudioBuffer(void*) noexcept;

} // namespace kilipili
