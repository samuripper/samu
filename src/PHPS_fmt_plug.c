/*
 * PHPS_fmt.c
 *
 * Salted PHP on the form (php-code): $hash = MD5(MD5($pass).$salt);
 * Based on salted IPB2 mode (by regenrecht at o2.pl).
 *
 * albert veli gmail com, 2007
 *
 * Convert hashes to the form username:$PHPS$salt$hash
 * For instance, if the pw file has the form
 * 1234<::>luser<::>luser@hotmail.com<::><::>1ea46bf1f5167b63d12bd47c8873050e<::>C9%
 * it can be converted to the wanted form with the following perl script:
 *
 * #!/usr/bin/perl -w
 * while (<>) {
 *    my @fields = split(/<::>/, $_);
 *    my $a =  substr $fields[5], 0, 1;
 *    my $b =  substr $fields[5], 1, 1;
 *    my $c =  substr $fields[5], 2, 1;
 *    printf "%s:\$IPB2\$%02x%02x%02x\$%s\n", $fields[1], ord($a), ord($b), ord($c), $fields[4];
 * }
 *
 * BUGS: Can't handle usernames with ':' in them.
 *
 * NOTE the new code 'hooks' into the generic MD5 code.  The 'Convert' call
 * changes the data from the PHPS format, into $dynamic_6$ format, and then
 * linkes to the MD5-GEN functions.  MD5-GENERIC and 'linkage' by Jim Fougeron.
 * the 'original' PHPS_fmt.c is saved into PHPS_fmt_orig.c   If you want the
 * original code, simply replace this file with that PHPS_fmt_orig.c file.
 *
 */

#include <string.h>

#include "common.h"
#include "formats.h"
#include "dynamic.h"

#define FORMAT_LABEL		"phps"
#define FORMAT_NAME			"PHPS -- md5(md5($pass).$salt)"

#define ALGORITHM_NAME		"?" /* filled in by md5-gen */
#define BENCHMARK_COMMENT	""
#define BENCHMARK_LENGTH	0

#define MD5_BINARY_SIZE		16
#define MD5_HEX_SIZE		(MD5_BINARY_SIZE * 2)

#define BINARY_SIZE			MD5_BINARY_SIZE

#define SALT_SIZE			3
#define PROCESSED_SALT_SIZE	SALT_SIZE

#define PLAINTEXT_LENGTH	32
#define CIPHERTEXT_LENGTH	(1 + 4 + 1 + SALT_SIZE * 2 + 1 + MD5_HEX_SIZE)

#define MIN_KEYS_PER_CRYPT	1
#define MAX_KEYS_PER_CRYPT	1

static struct fmt_tests phps_tests[] = {
	{"$PHPS$433925$5d756853cd63acee76e6dcd6d3728447", "welcome"},
	{NULL}
};

static char Conv_Buf[80];
static struct fmt_main *pFmt_Dynamic_6;
static void phps_init(struct fmt_main *pFmt);

/* this function converts a 'native' phps signature string into a $dynamic_6$ syntax string */
static char *Convert(char *Buf, char *ciphertext)
{
	unsigned long val, i;
	char *cp;

	if (text_in_dynamic_format_already(pFmt_Dynamic_6, ciphertext))
		return ciphertext;

	cp = strchr(&ciphertext[7], '$');
	if (!cp)
		return "*";

	sprintf(Buf, "$dynamic_6$%s$", &cp[1]);
	for (i = 0; i < 3; ++i)
	{
		char bTmp[3];
		bTmp[0] = ciphertext[6+i*2];
		bTmp[1] = ciphertext[6+i*2+1];
		bTmp[2] = 0;
		val = strtoul(bTmp, 0, 16);
		sprintf(bTmp, "%c", (unsigned char)val);
		strcat(Buf, bTmp);
	}
	return Buf;
}

static char *our_split(char *ciphertext, int index)
{
	return Convert(Conv_Buf, ciphertext);
}

static int phps_valid(char *ciphertext, struct fmt_main *pFmt)
{
	int i;
	if (!ciphertext || strlen(ciphertext) < CIPHERTEXT_LENGTH)
		return 0;

	if (!pFmt_Dynamic_6)
		phps_init(pFmt);

	if (strlen(ciphertext) != CIPHERTEXT_LENGTH) {
		return pFmt_Dynamic_6->methods.valid(ciphertext, pFmt_Dynamic_6);
	}

	if (strncmp(ciphertext, "$PHPS$", 6) != 0)
		return 0;

 	if (ciphertext[12] != '$')
		return 0;

	for (i = 0;i < SALT_SIZE*2; ++i)
		if (atoi16[ARCH_INDEX(ciphertext[i+6])] == 0x7F)
			return 0;

	for (i = 0;i < MD5_HEX_SIZE; ++i)
		if (atoi16[ARCH_INDEX(ciphertext[i+6+1+SALT_SIZE*2])] == 0x7F)
			return 0;

	if (!pFmt_Dynamic_6)
		phps_init(pFmt);
	return pFmt_Dynamic_6->methods.valid(Convert(Conv_Buf, ciphertext), pFmt_Dynamic_6);
}


static void * our_salt(char *ciphertext)
{
	return pFmt_Dynamic_6->methods.salt(Convert(Conv_Buf, ciphertext));
}
static void * our_binary(char *ciphertext)
{
	return pFmt_Dynamic_6->methods.binary(Convert(Conv_Buf, ciphertext));
}

struct fmt_main fmt_PHPS =
{
	{
		// setup the labeling and stuff. NOTE the max and min crypts are set to 1
		// here, but will be reset within our init() function.
		FORMAT_LABEL, FORMAT_NAME, ALGORITHM_NAME, BENCHMARK_COMMENT, BENCHMARK_LENGTH,
		PLAINTEXT_LENGTH, BINARY_SIZE, SALT_SIZE+1, 1, 1, FMT_CASE | FMT_8_BIT, phps_tests
	},
	{
		/*  All we setup here, is the pointer to valid, and the pointer to init */
		/*  within the call to init, we will properly set this full object      */
		phps_init,
		fmt_default_prepare,
		phps_valid
	}
};


static void phps_init(struct fmt_main *pFmt)
{
	if (pFmt->private.initialized == 0) {
		pFmt_Dynamic_6 = dynamic_THIN_FORMAT_LINK(&fmt_PHPS, Convert(Conv_Buf, phps_tests[0].ciphertext), "phps");
		fmt_PHPS.methods.salt   = our_salt;
		fmt_PHPS.methods.binary = our_binary;
		fmt_PHPS.methods.split = our_split;
		fmt_PHPS.params.algorithm_name = pFmt_Dynamic_6->params.algorithm_name;
		pFmt->private.initialized = 1;
	}
}

/**
 * GNU Emacs settings: K&R with 1 tab indent.
 * Local Variables:
 * c-file-style: "k&r"
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
