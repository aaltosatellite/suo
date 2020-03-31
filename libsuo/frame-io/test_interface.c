#include "test_interface.h"
#include "suo_macros.h"
#include <stdio.h>
#include <string.h>

struct test_output {
	struct decoder_code decoder;
	void *decoder_arg;
};

void *test_output_init(const void *conf)
{
	(void)conf;
	struct test_output *self = malloc(sizeof(struct test_output));
	memset(self, 0, sizeof(*self));
	return self;
}


int test_output_frame(void *arg, const bit_t *bits, size_t nbits, struct rx_metadata *metadata)
{
	struct test_output *self = arg;
	uint8_t decoded[0x200];
	size_t i;
	int ret;

	for(i = 0; i < nbits; i++)
		printf("%d", bits[i]);
	printf("\n\n");

	ret = self->decoder.decode(self->decoder_arg, bits, nbits, decoded, 0x200, metadata);
	if(ret >= 0) {
		for(i = 0; i < (size_t)ret; i++)
			printf("%02x ", decoded[i]);
		printf("\n");
		/* Print those which are valid ASCII characters */
		for(i = 0; i < (size_t)ret; i++) {
			char c = (char)decoded[i];
			if(c >= 32 && c <= 126)
				putchar(c);
		}
		printf("\n");
	} else {
		printf("Decode failed (%d)\n", ret);
	}
	printf("Timestamp: %lld ns   CFO: %E Hz  CFOD: %E Hz  RSSI: %6.2f dB  SNR: %6.2f dB  BER: %E  OER: %E  Mode: %u\n\n",
		(long long)metadata->timestamp,
		(double)metadata->cfo, (double)metadata->cfod,
		(double)metadata->rssi, (double)metadata->snr,
		(double)metadata->ber, (double)metadata->oer,
		metadata->mode);
	return 0;
}


int test_output_destroy(void *arg)
{
	free(arg);
	return 0;
}


int test_output_set_callbacks(void *arg, const struct decoder_code *decoder, void *decoder_arg)
{
	struct test_output *self = arg;
	self->decoder = *decoder;
	self->decoder_arg = decoder_arg;
	return 0;
}


CONFIG_NONE()

const struct rx_output_code test_rx_output_code = { "test_output", test_output_init, test_output_destroy, init_conf, set_conf, test_output_set_callbacks, test_output_frame };



/* Transmitter testing things */

struct test_input {
	struct encoder_code encoder;
	void *encoder_arg;
};


void *test_input_init(const void *conf)
{
	(void)conf;
	struct test_input *self = malloc(sizeof(struct test_input));
	memset(self, 0, sizeof(*self));
	return self;
}


int test_input_destroy(void *arg)
{
	free(arg);
	return 0;
}


int test_input_set_callbacks(void *arg, const struct encoder_code *encoder, void *encoder_arg)
{
	struct test_input *self = arg;
	self->encoder = *encoder;
	self->encoder_arg = encoder_arg;
	return 0;
}


int test_input_get_frame(void *arg, bit_t *bits, size_t maxbits, timestamp_t timestamp, struct tx_metadata *metadata)
{
	struct test_input *self = arg;
	(void)self;
	(void)metadata;
	const timestamp_t frame_interval = 20000000;
	if(timestamp % 400000000LL < 100000000LL) {
		// round up to next multiple of frame_interval
		metadata->timestamp = (timestamp + frame_interval) / frame_interval * frame_interval;

#if 0
		const uint8_t packet[30] = "testidataa";
		return self->encoder.encode(self->encoder_arg, bits, maxbits, packet, 30);
#endif
#define TESTLEN 30
		if (maxbits < TESTLEN)
			return -1;
		memcpy(bits, (const uint8_t[TESTLEN]){
			0,0, 0,0,
			1,1, 0,1, 0,0, 0,0, 1,1, 1,0, 1,0, 0,1, 1,1, 0,1, 0,0,
			0,0, 0,0
		}, TESTLEN);
		return TESTLEN;
	}
	return -1;
}


const struct tx_input_code test_tx_input_code = { "test_input", test_input_init, test_input_destroy, init_conf, set_conf, test_input_set_callbacks, test_input_get_frame };
