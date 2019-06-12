#ifndef MM_COMMON_H
#define MM_COMMON_H
#include <stdlib.h>
#include <stdint.h>
#include <complex.h>

typedef float complex sample_t;
typedef uint8_t bit_t; // TODO: choose type and representation for soft decisions

struct frame_output_code {
	void *(*init)   (const void *conf);
	int   (*frame)  (void *arg, bit_t *bits, size_t nbits);
};

struct receiver_code {
	void *(*init)        (const void *conf);
	int (*set_callbacks) (void *, const struct frame_output_code *, void *frame_output_arg);
	int (*execute)       (void *, sample_t *samp, size_t nsamp);
};

struct decoder_code {
	void *(*init)    (const void *conf);
	int   (*decode)  (void *, bit_t *bits, size_t nbits, uint8_t *decoded, size_t nbytes);
};

#endif
