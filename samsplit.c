
/**
 * @file samsplit.c
 *
 * @brief split cocatenated sam stream into individuals
 *
 * @author Hajime Suzuki
 * @date 2016/12/9
 * @license MIT
 */
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void print_help(void)
{
	fprintf(stderr, "\n"
					"  samsplit - split sam stream into individuals\n"
					"\n"
					"minialign long-read aligner dumps concatenated sam file into stdout when\n"
					"run with all-versus-all alignment mode. The samsplit utility split the\n"
					"stream into correct individuals.\n"
					"\n");
	fprintf(stderr, "Usage:\n"
					"  $ minialign -X <reads.fa> [<reads.fa> ...] | samsplit out\n"
					"  ## results in out.0000.sam, out.0001.sam, ...\n"
					"\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  -i STR         input file name [stdin]\n");
	fprintf(stderr, "  -b INT         base count [0]\n");
	fprintf(stderr, "  -h             print help (this) message\n");
	fprintf(stderr, "  -v             print version number [0.0.1]\n");
	fprintf(stderr, "\n");
	return;
}

int main(int argc, char *argv[])
{
	int c, h = 0;
	uint64_t cnt = 0;
	char const *input = NULL, *prefix = NULL;
	FILE *fp = stdin;

#ifdef __linux__
	setenv("POSIXLY_CORRECT", "1", 0);
#endif
	while(optind < argc) {
		if((c = getopt(argc, argv, "i:vh")) < 0) {
			if(prefix != NULL) {
				fprintf(stderr, "[Warning] extra positional argument `%s' is discarded.\n", argv[optind]);
			} else {
				prefix = argv[optind];
			}
			optind++;
			continue;
		}
		switch(c) {
			case 'i': input = optarg; break;
			case 'b': cnt = atoi(optarg); break;
			case 'v': puts("0.0.1"); return 0;
			case 'h': h = 1; break;
		}
	}
	if(h) { print_help(); return 0; }
	if(isatty(fileno(stdin)) && (input == NULL || (fp = fopen(input, "rb")) == NULL)) {
		fprintf(stderr, "[ERROR] failed to open input file `%s'.\n", input != NULL ? input : "");
		print_help(); return 1;
	}
	prefix = prefix == NULL ? "samsplit" : prefix;
	if(strlen(prefix) > 240) { fprintf(stderr, "[ERROR] prefix must be shorter than 240.\n"); return 1; }

	uint32_t arr = 0;
	for(uint64_t i = 0; i < 4; i++) {
		arr = (arr>>8) | (getc(fp)<<24);
	}
	if(arr != 0x09444840) { return 0; }	// check "@HD\t"
	do {
		char filename[256];
		FILE *o = NULL;
		sprintf(filename, "%s%s%04lu.sam", strlen(prefix) > 0 ? prefix : "", strlen(prefix) > 0 ? "." : "", cnt++);
		if((o = fopen(filename, "w")) == NULL) {
			fprintf(stderr, "[ERROR] failed to open output file `%s'.\n", filename);
			return(-1);
		}

		while((c = getc(fp)) != EOF) {
			putc(arr & 0xff, o);
			if((arr = (arr>>8) | ((uint32_t)c<<24)) == 0x09444840) { break; }	/* "@HD\t" */
		}
		if(c == EOF) {
			while(arr != 0) { putc(arr & 0xff, o); arr >>= 8; }	/* flush */
		}
		fclose(o);
	} while(c != EOF);
	return 0;
}

/**
 * end of samsplit.c
 */
