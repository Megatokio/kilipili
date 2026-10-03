
// lsbit is left => revert bits
#undef r
#undef _r

#define _r(b)                                                                                                  \
	((b & 128) >> 7) + ((b & 64) >> 5) + ((b & 32) >> 3) + ((b & 16) >> 1) + ((b & 8) << 1) + ((b & 4) << 3) + \
		((b & 2) << 5) + ((b & 1) << 7)

#define r(b) (_r(b)) | uint8((_r(b)) << 1)
