#include "configure.h"
#include "modem/simple_receiver.h"
#include "modem/simple_transmitter.h"
#include "coding/basic_decoder.h"
#include "coding/basic_encoder.h"
#include "signal-io/soapysdr_io.h"
#include "frame-io/zmq_interface.h"
#include "frame-io/test_interface.h"
#include <string.h>
#include <stdio.h>


/* Read a section of a configuration file and initialize
 * a given suo module accordingly.
 * If f == NULL, initialize with the default configuration. */
void *read_conf_and_init(const struct any_code *code, FILE *f)
{
	void *conf = code->init_conf();

	/* Parsing strings is C is not nice :( */
	char line[80], param[80], value[80];
	while (f != NULL && fgets(line, sizeof(line), f) != NULL) {
		// Skip comments
		if (line[0] == '#')
			continue;

		// - marks end of the configuration section, so stop there
		if (line[0] == '-')
			break;

		// Find the end of the line
		char *p_end = strchr(line, '\r');
		if (p_end == NULL)
			p_end = strchr(line, '\n');
		if (p_end == NULL)
			p_end = line + strlen(line);

		// Skip empty lines
		if (p_end == line)
			continue;

		// Find the delimiter. Stop reading if missing.
		char *p_del = strchr(line, ' ');
		if (p_del == NULL)
			continue;

		strncpy(param, line, p_del - line);
		param[p_del - line] = '\0';
		strncpy(value, p_del + 1, p_end - (p_del + 1));
		value[p_end - (p_del + 1)] = '\0';

		if (code->set_conf(conf, param, value) < 0) {
			fprintf(stderr, "Invalid configuration %s %s\n", param, value);
		}
	}
	return code->init(conf);
}


int read_configuration(struct suo *suo, FILE *f)
{
	// TODO: make the choice of functions configurable
	suo->receiver        = &simple_receiver_code;
	suo->receiver_arg    = read_conf_and_init((const struct any_code*)suo->receiver, f);
	suo->decoder         = &basic_decoder_code;
	suo->decoder_arg     = read_conf_and_init((const struct any_code*)suo->decoder, f);
	suo->rx_output       = &zmq_rx_output_code;
	suo->rx_output_arg   = read_conf_and_init((const struct any_code*)suo->rx_output, f);

	suo->transmitter     = &simple_transmitter_code;
	suo->transmitter_arg = read_conf_and_init((const struct any_code*)suo->transmitter, f);
	suo->encoder         = &basic_encoder_code;
	suo->encoder_arg     = read_conf_and_init((const struct any_code*)suo->encoder, f);
	suo->tx_input        = &zmq_tx_input_code;
	suo->tx_input_arg    = read_conf_and_init((const struct any_code*)suo->tx_input, f);

	suo->signal_io       = &soapysdr_io_code;
	suo->signal_io_arg   = read_conf_and_init((const struct any_code*)suo->signal_io, f);
	return 0;
}


int configure(struct suo *suo, int argc, char *argv[])
{
	memset(suo, 0, sizeof(*suo));

	FILE *f = NULL;
	if (argc >= 2)
		f = fopen(argv[1], "r");
	read_configuration(suo, f);
	if (f != NULL)
		fclose(f);

	if (suo->receiver != NULL) {
		suo->rx_output  ->set_callbacks(suo->rx_output_arg, suo->decoder, suo->decoder_arg);
		suo->receiver   ->set_callbacks(suo->receiver_arg, suo->rx_output, suo->rx_output_arg);
	}

	if (suo->transmitter != NULL) {
		suo->tx_input   ->set_callbacks(suo->tx_input_arg, suo->encoder, suo->encoder_arg);
		suo->transmitter->set_callbacks(suo->transmitter_arg, suo->tx_input, suo->tx_input_arg);
	}

	suo->signal_io->set_callbacks(suo->signal_io_arg, suo->receiver, suo->receiver_arg, suo->transmitter, suo->transmitter_arg);

	return 0;
}


int deinitialize(struct suo *suo)
{
	if (suo->rx_output != NULL)
		suo->rx_output->destroy(suo->rx_output_arg);

	if (suo->tx_input != NULL)
		suo->tx_input->destroy(suo->tx_input_arg);

	return 0;
}
