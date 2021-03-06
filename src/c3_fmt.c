/*
 * This file is part of John the Ripper password cracker,
 * Copyright (c) 2009-2011 by Solar Designer
 *
 * Generic crypt(3) support, as well as support for glibc's crypt_r(3) and
 * Solaris' MT-safe crypt(3C) with OpenMP parallelization.
 */

#define _XOPEN_SOURCE 4 /* for crypt(3) */
#define _XOPEN_SOURCE_EXTENDED
#define _XOPEN_VERSION 4
#define _XPG4_2
#define _GNU_SOURCE /* for crypt_r(3) */
#include <string.h>
#if defined(_OPENMP) && defined(__GLIBC__)
#include <crypt.h>
#include <omp.h> /* for omp_get_thread_num() */
#else
#include <unistd.h>
#endif

#include "options.h"
#include "arch.h"
#include "misc.h"
#include "params.h"
#include "memory.h"
#include "common.h"
#include "formats.h"
#include "loader.h"
#ifdef HAVE_MPI
#include "john-mpi.h"
#endif

#define FORMAT_LABEL			"crypt"
#define FORMAT_NAME			"generic crypt(3)"
#define ALGORITHM_NAME			"?/" ARCH_BITS_STR

#define BENCHMARK_COMMENT		" DES"
#define BENCHMARK_LENGTH		0

#define PLAINTEXT_LENGTH		72

#define BINARY_SIZE			128
#define SALT_SIZE			BINARY_SIZE

#define MIN_KEYS_PER_CRYPT		96
#define MAX_KEYS_PER_CRYPT		96

static struct fmt_tests tests[] = {
	{"CCNf8Sbh3HDfQ", "U*U*U*U*"},
	{"CCX.K.MFy4Ois", "U*U***U"},
	{"CC4rMpbg9AMZ.", "U*U***U*"},
	{"XXxzOu6maQKqQ", "*U*U*U*U"},
	{"SDbsugeBiC58A", ""},
	{NULL}
};

static char saved_key[MAX_KEYS_PER_CRYPT][PLAINTEXT_LENGTH + 1];
static char saved_salt[SALT_SIZE];
static char crypt_out[MAX_KEYS_PER_CRYPT][BINARY_SIZE];

#if defined(_OPENMP) && defined(__GLIBC__)
#define MAX_THREADS			MAX_KEYS_PER_CRYPT

/* We assume that this is zero-initialized (all NULL pointers) */
static struct crypt_data *crypt_data[MAX_THREADS];
#endif

static void init(struct fmt_main *self)
{
	if (options.subformat) {
		if (strcasecmp(options.subformat, "md5crypt")==0 ||
		    strcasecmp(options.subformat, "md5")==0) {
			self->params.benchmark_comment = " MD5";
			tests[0].ciphertext = "$1$dXc3I7Rw$ctlgjDdWJLMT.qwHsWhXR1";
			tests[1].ciphertext = "$1$dXc3I7Rw$94JPyQc/eAgQ3MFMCoMF.0";
			tests[2].ciphertext = "$1$dXc3I7Rw$is1mVIAEtAhIzSdfn5JOO0";
			tests[3].ciphertext = "$1$eQT9Hwbt$XtuElNJD.eW5MN5UCWyTQ0";
			tests[4].ciphertext = "$1$Eu.GHtia$CFkL/nE1BYTlEPiVx1VWX0";
		} else if ((strcasecmp(options.subformat, "sha256crypt")==0) ||
		           (strcasecmp(options.subformat, "sha-256")==0) ||
		           (strcasecmp(options.subformat, "sha256")==0)) {
			self->params.benchmark_comment = " SHA-256 rounds=5000";
			tests[0].ciphertext = "$5$LKO/Ute40T3FNF95$U0prpBQd4PloSGU0pnpM4z9wKn4vZ1.jsrzQfPqxph9";
			tests[1].ciphertext = "$5$LKO/Ute40T3FNF95$fdgfoJEBoMajNxCv3Ru9LyQ0xZgv0OBMQoq80LQ/Qd.";
			tests[2].ciphertext = "$5$LKO/Ute40T3FNF95$8Ry82xGnnPI/6HtFYnvPBTYgOL23sdMXn8C29aO.x/A";
			tests[3].ciphertext = "$5$9mx1HkCz7G1xho50$O7V7YgleJKLUhcfk9pgzdh3RapEaWqMtEp9UUBAKIPA";
			tests[4].ciphertext = "$5$kc7lRD1fpYg0g.IP$d7CMTcEqJyTXyeq8hTdu/jB/I6DGkoo62NXbHIR7S43";
		} else if ((strcasecmp(options.subformat, "sha512crypt")==0) ||
		           (strcasecmp(options.subformat, "sha-512")==0) ||
		           (strcasecmp(options.subformat, "sha512")==0)) {
			self->params.benchmark_comment = " SHA-512 rounds=5000";
			tests[0].ciphertext = "$6$LKO/Ute40T3FNF95$6S/6T2YuOIHY0N3XpLKABJ3soYcXD9mB7uVbtEZDj/LNscVhZoZ9DEH.sBciDrMsHOWOoASbNLTypH/5X26gN0";
			tests[1].ciphertext = "$6$LKO/Ute40T3FNF95$wK80cNqkiAUzFuVGxW6eFe8J.fSVI65MD5yEm8EjYMaJuDrhwe5XXpHDJpwF/kY.afsUs1LlgQAaOapVNbggZ1";
			tests[2].ciphertext = "$6$LKO/Ute40T3FNF95$YS81pp1uhOHTgKLhSMtQCr2cDiUiN03Ud3gyD4ameviK1Zqz.w3oXsMgO6LrqmIEcG3hiqaUqHi/WEE2zrZqa/";
			tests[3].ciphertext = "$6$OmBOuxFYBZCYAadG$WCckkSZok9xhp4U1shIZEV7CCVwQUwMVea7L3A77th6SaE9jOPupEMJB.z0vIWCDiN9WLh2m9Oszrj5G.gt330";
			tests[4].ciphertext = "$6$ojWH1AiTee9x1peC$QVEnTvRVlPRhcLQCk/HnHaZmlGAAjCfrAN0FtOsOnUk5K5Bn/9eLHHiRzrTzaIKjW9NTLNIBUCtNVOowWS2mN.";
		} else if ((strcasecmp(options.subformat, "bf")==0) ||
		           (strcasecmp(options.subformat, "blowfish")==0) ||
		           (strcasecmp(options.subformat, "bcrypt")==0)) {
			self->params.benchmark_comment = " BF x32";
			tests[0].ciphertext = "$2a$05$c92SVSfjeiCD6F2nAD6y0uBpJDjdRkt0EgeC4/31Rf2LUZbDRDE.O";
			tests[1].ciphertext = "$2a$05$WY62Xk2TXZ7EvVDQ5fmjNu7b0GEzSzUXUh2cllxJwhtOeMtWV3Ujq";
			tests[2].ciphertext = "$2a$05$Fa0iKV3E2SYVUlMknirWU.CFYGvJ67UwVKI1E2FP6XeLiZGcH3MJi";
			tests[3].ciphertext = "$2a$05$.WRrXibc1zPgIdRXYfv.4uu6TD1KWf0VnHzq/0imhUhuxSxCyeBs2";
			tests[4].ciphertext = "$2a$05$Otz9agnajgrAe0.kFVF9V.tzaStZ2s1s4ZWi/LY4sw2k/MTVFj/IO";
		} else if (strcasecmp(options.subformat, "descrypt") &&
		           strcasecmp(options.subformat, "des")) {
			fprintf(stderr, "Subformat unknown to John. Currently supported: descrypt, md5crypt, bcrypt, sha256crypt, sha512crypt\n\n");
			error();
		}
	}
}

static int valid(char *ciphertext, struct fmt_main *self)
{
	int length, count_base64, id, pw_length;
	char pw[PLAINTEXT_LENGTH + 1], *new_ciphertext;
/* We assume that these are zero-initialized */
	static char sup_length[BINARY_SIZE], sup_id[0x80];

	length = count_base64 = 0;
	while (ciphertext[length]) {
		if (atoi64[ARCH_INDEX(ciphertext[length])] != 0x7F &&
		    (ciphertext[0] == '_' || length >= 2))
			count_base64++;
		length++;
	}

	if (length < 13 || length >= BINARY_SIZE)
		return 0;

	id = 0;
	if (length == 13 && count_base64 == 11)
		id = 1;
	else
	if (length >= 13 &&
	    count_base64 >= length - 2 && /* allow for invalid salt */
	    (length - 2) % 11 == 0)
		id = 2;
	else
	if (length == 20 && count_base64 == 19 && ciphertext[0] == '_')
		id = 3;
	else
	if (ciphertext[0] == '$') {
		id = (unsigned char)ciphertext[1];
		if (id <= 0x20 || id >= 0x80)
			id = 9;
	} else
	if (ciphertext[0] == '*' || ciphertext[0] == '!') /* likely locked */
		id = 10;

/* Previously detected as supported */
	if (sup_length[length] > 0 && sup_id[id] > 0)
		return 1;

/* Previously detected as unsupported */
	if (sup_length[length] < 0 && sup_id[id] < 0)
		return 0;

	pw_length = ((length - 2) / 11) << 3;
	if (pw_length >= sizeof(pw))
		pw_length = sizeof(pw) - 1;
	memcpy(pw, ciphertext, pw_length); /* reuse the string, why not? */
	pw[pw_length] = 0;

#if defined(_OPENMP) && defined(__GLIBC__)
/*
 * Let's use crypt_r(3) just like we will in crypt_all() below.
 * It is possible that crypt(3) and crypt_r(3) differ in their supported hash
 * types on a given system.
 */
	{
		struct crypt_data **data = &crypt_data[0];
		if (!*data) {
/*
 * **data is not exactly tiny, but we use mem_alloc_tiny() for its alignment
 * support and error checking.  We do not need to free() this memory anyway.
 *
 * The page alignment is to keep different threads' data on different pages.
 */
			*data = mem_alloc_tiny(sizeof(**data), MEM_ALIGN_PAGE);
			memset(*data, 0, sizeof(**data));
		}
		new_ciphertext = crypt_r(pw, ciphertext, *data);
	}
#else
	new_ciphertext = crypt(pw, ciphertext);
#endif

	if (strlen(new_ciphertext) == length &&
	    !strncmp(new_ciphertext, ciphertext, 2)) {
		sup_length[length] = 1;
		sup_id[id] = 1;
		return 1;
	}

	if (id != 10 && !ldr_in_pot)
#ifdef HAVE_MPI
	if (mpi_id == 0)
#endif
		fprintf(stderr, "Generic crypt(3) module: "
		    "hash encoding string length %d, type id %c%c\n"
		    "appears to be unsupported on this system; "
		    "will not load such hashes.\n",
		    length, id > 0x20 ? '$' : '#', id > 0x20 ? id : '0' + id);

	if (!sup_length[length])
		sup_length[length] = -1;
	if (!sup_id[id])
		sup_id[id] = -1;
	return 0;
}

static void *binary(char *ciphertext)
{
	static char out[BINARY_SIZE];
	strncpy(out, ciphertext, sizeof(out)); /* NUL padding is required */
	return out;
}

static void *salt(char *ciphertext)
{
	static char out[SALT_SIZE];
	int cut = sizeof(out);

#if 1
/* This piece is optional, but matching salts are not detected without it */
	int length = strlen(ciphertext);

	switch (length) {
	case 13:
	case 24:
		cut = 2;
		break;

	case 20:
		if (ciphertext[0] == '_') cut = 9;
		break;

	case 35:
	case 46:
	case 57:
		if (ciphertext[0] != '$') cut = 2;
		/* fall through */

	default:
		if ((length >= 26 && length <= 34 &&
		    !strncmp(ciphertext, "$1$", 3)) ||
		    (length >= 47 && !strncmp(ciphertext, "$5$", 3)) ||
		    (length >= 90 && !strncmp(ciphertext, "$6$", 3))) {
			char *p = strrchr(ciphertext + 3, '$');
			if (p) cut = p - ciphertext;
		} else
		if (length == 59 && !strncmp(ciphertext, "$2$", 3))
			cut = 28;
		else
		if (length == 60 &&
		    (!strncmp(ciphertext, "$2a$", 4) ||
		    !strncmp(ciphertext, "$2x$", 4) ||
		    !strncmp(ciphertext, "$2y$", 4)))
			cut = 29;
		else
		if (length >= 27 &&
		    (!strncmp(ciphertext, "$md5$", 5) ||
		    !strncmp(ciphertext, "$md5,", 5))) {
			char *p = strrchr(ciphertext + 4, '$');
			if (p) {
				/* NUL padding is required */
				memset(out, 0, sizeof(out));
				memcpy(out, ciphertext, ++p - ciphertext);
/*
 * Workaround what looks like a bug in sunmd5.c: crypt_genhash_impl() where it
 * takes a different substring as salt depending on whether the optional
 * existing hash encoding is present after the salt or not.  Specifically, the
 * last '$' delimiter is included into the salt when there's no existing hash
 * encoding after it, but is omitted from the salt otherwise.
 */
				out[p - ciphertext] = 'x';
				return out;
			}
		}
	}
#endif

	/* NUL padding is required */
	memset(out, 0, sizeof(out));
	memcpy(out, ciphertext, cut);

	return out;
}

#define H(s, i) \
	((int)(unsigned char)(atoi64[ARCH_INDEX((s)[(i)])] ^ (s)[(i) - 1]))

#define H0(s) \
	int i = strlen(s) - 2; \
	return i > 0 ? H((s), i) & 0xF : 0
#define H1(s) \
	int i = strlen(s) - 2; \
	return i > 2 ? (H((s), i) ^ (H((s), i - 2) << 4)) & 0xFF : 0
#define H2(s) \
	int i = strlen(s) - 2; \
	return i > 2 ? (H((s), i) ^ (H((s), i - 2) << 6)) & 0xFFF : 0
#define H3(s) \
	int i = strlen(s) - 2; \
	return i > 4 ? (H((s), i) ^ (H((s), i - 2) << 5) ^ \
	    (H((s), i - 4) << 10)) & 0xFFFF : 0
#define H4(s) \
	int i = strlen(s) - 2; \
	return i > 6 ? (H((s), i) ^ (H((s), i - 2) << 5) ^ \
	    (H((s), i - 4) << 10) ^ (H((s), i - 6) << 15)) & 0xFFFFF : 0

static int binary_hash_0(void *binary)
{
	H0((char *)binary);
}

static int binary_hash_1(void *binary)
{
	H1((char *)binary);
}

static int binary_hash_2(void *binary)
{
	H2((char *)binary);
}

static int binary_hash_3(void *binary)
{
	H3((char *)binary);
}

static int binary_hash_4(void *binary)
{
	H4((char *)binary);
}

static int get_hash_0(int index)
{
	H0(crypt_out[index]);
}

static int get_hash_1(int index)
{
	H1(crypt_out[index]);
}

static int get_hash_2(int index)
{
	H2(crypt_out[index]);
}

static int get_hash_3(int index)
{
	H3(crypt_out[index]);
}

static int get_hash_4(int index)
{
	H4(crypt_out[index]);
}

static int salt_hash(void *salt)
{
	int i, h;

	i = strlen((char *)salt) - 1;
	if (i > 1) i--;

	h = (unsigned char)atoi64[ARCH_INDEX(((char *)salt)[i])];
	h ^= ((unsigned char *)salt)[i - 1];
	h <<= 6;
	h ^= (unsigned char)atoi64[ARCH_INDEX(((char *)salt)[i - 1])];
	h ^= ((unsigned char *)salt)[i];

	return h & (SALT_HASH_SIZE - 1);
}

static void set_salt(void *salt)
{
	strcpy(saved_salt, salt);
}

static void set_key(char *key, int index)
{
	strnzcpy(saved_key[index], key, PLAINTEXT_LENGTH + 1);
}

static char *get_key(int index)
{
	return saved_key[index];
}

static void crypt_all(int count)
{
	int index;

#if defined(_OPENMP) && defined(__GLIBC__)
#pragma omp parallel for default(none) private(index) shared(count, crypt_out, saved_key, saved_salt, crypt_data)
	for (index = 0; index < count; index++) {
		char *hash;
		int t = omp_get_thread_num();
		if (t < MAX_THREADS) {
			struct crypt_data **data = &crypt_data[t];
			if (!*data) {
/* Stagger the structs to reduce their competition for the same cache lines */
				size_t mask = MEM_ALIGN_PAGE, shift = 0;
				while (t) {
					mask >>= 1;
					if (mask < MEM_ALIGN_CACHE)
						break;
					if (t & 1)
						shift += mask;
					t >>= 1;
				}
				*data = (void *)((char *)
				    mem_alloc_tiny(sizeof(**data) +
				    shift, MEM_ALIGN_PAGE) + shift);
				memset(*data, 0, sizeof(**data));
			}
			hash = crypt_r(saved_key[index], saved_salt, *data);
		} else { /* should not happen */
			struct crypt_data data;
			memset(&data, 0, sizeof(data));
			hash = crypt_r(saved_key[index], saved_salt, &data);
		}
		strnzcpy(crypt_out[index], hash, BINARY_SIZE);
	}
#else
#if defined(_OPENMP) && defined(__sun)
/*
 * crypt(3C) is MT-safe on Solaris.  For traditional DES-based hashes, this is
 * implemented with locking (hence there's no speedup from the use of multiple
 * threads, and the per-thread performance is extremely poor anyway).  For
 * modern hash types, the function is actually able to compute multiple hashes
 * in parallel by different threads (and the performance for some hash types is
 * reasonable).  Overall, this code is reasonable to use for SHA-crypt and
 * SunMD5 hashes, which are not yet supported by John natively.
 */
#pragma omp parallel for default(none) private(index) shared(count, crypt_out, saved_key, saved_salt)
#endif
	for (index = 0; index < count; index++)
		strnzcpy(crypt_out[index], crypt(saved_key[index], saved_salt),
		    BINARY_SIZE);
#endif
}

static int cmp_all(void *binary, int count)
{
	int index;

	for (index = 0; index < count; index++)
		if (!strcmp((char *)binary, crypt_out[index]))
			return 1;

	return 0;
}

static int cmp_one(void *binary, int index)
{
	return !strcmp((char *)binary, crypt_out[index]);
}

static int cmp_exact(char *source, int index)
{
	return 1;
}

struct fmt_main fmt_crypt = {
	{
		FORMAT_LABEL,
		FORMAT_NAME,
		ALGORITHM_NAME,
		BENCHMARK_COMMENT,
		BENCHMARK_LENGTH,
		PLAINTEXT_LENGTH,
		BINARY_SIZE,
		SALT_SIZE,
		MIN_KEYS_PER_CRYPT,
		MAX_KEYS_PER_CRYPT,
		FMT_CASE | FMT_8_BIT | FMT_OMP,
		tests
	}, {
		init,
		fmt_default_prepare,
		valid,
		fmt_default_split,
		binary,
		salt,
		{
			binary_hash_0,
			binary_hash_1,
			binary_hash_2,
			binary_hash_3,
			binary_hash_4,
			NULL,
			NULL
		},
		salt_hash,
		set_salt,
		set_key,
		get_key,
		fmt_default_clear_keys,
		crypt_all,
		{
			get_hash_0,
			get_hash_1,
			get_hash_2,
			get_hash_3,
			get_hash_4,
			NULL,
			NULL
		},
		cmp_all,
		cmp_one,
		cmp_exact
	}
};
