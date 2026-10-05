// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "VideoPlane.h"

namespace kilipili::Video
{

/*	Template class FrameBuffer
	A FrameBuffer is a VideoPlane which typically renders the whole screen from a Pixmap, CharMap or similar.
	FrameBuffers are normally used as the first (or only) VideoPlane and typically cover the whole video screen.
	There are several files defining FrameBuffers for various types of backing store.
*/
template<class BackingStore, typename = void>
class FrameBuffer;


// _____________________________________________________________________
// deduction guides:

template<typename T>
FrameBuffer(T*) -> FrameBuffer<T>;

template<class T>
FrameBuffer(RCPtr<T>) -> FrameBuffer<T>;


} // namespace kilipili::Video
