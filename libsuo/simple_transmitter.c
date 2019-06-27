#include "simple_transmitter.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <liquid/liquid.h>

#define FRAMELEN_MAX 0x900
static const float pi2f = 6.283185307179586f;

struct simple_transmitter {
	/* Configuration */
	struct simple_transmitter_conf c;
	uint32_t symrate;
	float freq0, freq1;

	/* State */
	bool transmitting;
	unsigned framelen, framepos;
	uint32_t symphase;

	/* liquid-dsp objects */
	nco_crcf l_nco;

	/* Callbacks */
	struct tx_input_code input;
	void *input_arg;

	/* Buffers */
	struct tx_metadata metadata;
	bit_t framebuf[FRAMELEN_MAX];
};


static void *init(const void *conf_v)
{
	struct simple_transmitter *self = malloc(sizeof(struct simple_transmitter));
	if(self == NULL) return NULL;
	memset(self, 0, sizeof(struct simple_transmitter));

	self->c = *(const struct simple_transmitter_conf*)conf_v;
	const float samplerate = self->c.samplerate;

	self->symrate = 4294967296.0f * self->c.symbolrate / samplerate;

	const float deviation = pi2f * self->c.modindex * 0.5f * self->c.symbolrate / samplerate, cf = pi2f * self->c.centerfreq / samplerate;
	self->freq0 = cf - deviation;
	self->freq1 = cf + deviation;

	self->l_nco = nco_crcf_create(LIQUID_NCO);

	return self;
}


static int destroy(void *arg)
{
	/* TODO (low priority since memory gets freed in the end anyway) */
	(void)arg;
	return 0;
}


static int set_callbacks(void *arg, const struct tx_input_code *input, void *input_arg)
{
	struct simple_transmitter *self = arg;
	self->input = *input;
	self->input_arg = input_arg;
	return 0;
}


static int execute(void *arg, sample_t *samples, size_t maxsamples, timestamp_t *timestamp)
{
	struct simple_transmitter *self = arg;
	size_t nsamples = 0;

	const uint32_t symrate = self->symrate;
	const float freq0 = self->freq0, freq1 = self->freq1;
	bit_t *framebuf = self->framebuf;

	bool transmitting = self->transmitting;
	unsigned framelen = self->framelen, framepos = self->framepos;
	uint32_t symphase = self->symphase;

	if(!transmitting) {
		int ret;
		ret = self->input.get_frame(self->input_arg, framebuf, FRAMELEN_MAX, *timestamp, &self->metadata);
		if(ret > 0) {
			assert(ret <= FRAMELEN_MAX);
			transmitting = 1;
			framelen = ret;
			framepos = 0;
		}
	}

	if(transmitting) {
		size_t si;
		for(si = 0; si < maxsamples; si++) {
			float f_in = freq0;
			if(framepos < framelen) {
				if(framebuf[framepos]) f_in = freq1;
			} else {
				transmitting = 0;
				break;
			}

			nco_crcf_set_frequency(self->l_nco, f_in);
			nco_crcf_step(self->l_nco);
			nco_crcf_cexpf(self->l_nco, &samples[si]);

			uint32_t symphase1 = symphase;
			symphase = symphase1 + symrate;
			if(symphase < symphase1) { // wrapped around?
				framepos++;
			}
		}
		nsamples = si;
	}

	self->transmitting = transmitting;
	self->framelen = framelen;
	self->framepos = framepos;
	self->symphase = symphase;
	return nsamples;
}


const struct transmitter_code simple_transmitter_code = { init, destroy, set_callbacks, execute };
