/* pfx cracker patch for JtR. Hacked together during  June of 2012 by
 * Dhiru Kholia <dhiru.kholia at gmail.com>.
 *
 * This software is Copyright © 2021, Dhiru Kholia <dhiru.kholia at gmail.com>,
 * and it is hereby released to the general public under the following terms:
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted.
 *
 * Generating pfx files:
 *
 * keytool -genkeypair -alias my_certificate -keystore my_keystore.pfx -storepass
 * my_password -validity 365 -keyalg RSA -keysize 2048 -storetype pkcs12 */

#include <openssl/opensslv.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/pkcs12.h>
#include <openssl/ssl.h>

#undef MEM_FREE
#include "options.h"
#ifdef _OPENMP
#include <omp.h>
#define OMP_SCALE               64
#endif
#include <string.h>
#include "arch.h"
#include "common.h"
#include "formats.h"
#include "params.h"
#include "misc.h"

#define FORMAT_LABEL        "pfx"
#define FORMAT_NAME         "pfx"
#define ALGORITHM_NAME      "32/" ARCH_BITS_STR
#define BENCHMARK_COMMENT   ""
#define BENCHMARK_LENGTH    -1001
#define PLAINTEXT_LENGTH    32
#define BINARY_SIZE         0
#define SALT_SIZE           sizeof(struct custom_salt)
#define MIN_KEYS_PER_CRYPT  1
#define MAX_KEYS_PER_CRYPT  1

static char (*saved_key)[PLAINTEXT_LENGTH + 1];
static int any_cracked, *cracked;
static size_t cracked_size;

static struct custom_salt {
	int len;
	PKCS12 pfx;
} *cur_salt;

static struct fmt_tests pfx_tests[] = {
	{"$pfx$*2604*30820a28020103308209e206092a864886f70d010701a08209d3048209cf308209cb3082057806092a864886f70d010701a082056904820565308205613082055d060b2a864886f70d010c0a0102a08204fa308204f63028060a2a864886f70d010c0103301a0414e9a49f4190a3084e02ceba2f049303750f6646da02020400048204c8cd40bb89c287b9fe70a88825e33a648c76aa1b35d93131d445e48943ee50ff8a0aee6a0483a289fbacf21290a8553e3414ea6bd6b305407d709bbaf915a99430c998d9ba68e71f4036d42feb386061d645433390658df91bd4e9750a39f9288f7cf8001e2adc8e4d7480f1a5e2d63799df20d9eb956f86b33330ec2c206b1ae47cf54d9cf2cdd970664c251e64cc725456e2c14506cfd7d9ff1d2894a50093fff4f29d5967a0f788ed707ade93cb3ad7e87d96dad844d2037f4d5e863ec5170c0f1d4514d752a266cd4db49b63c5d86646e54a68187ddc99b00499286f79e2e7c54e30d3a1b1f035d7113180d072c7218d399f8b5427dc2d6fcb42518bd6bb97f74c97ea2358ef39fb176397fe7729cd5c3a474423f0a0e74a91c77bb27b24f82463081fed53bdf216840b2c60482846010b654e2c74db4abfb07936e0cc9d0d133ac7a4baa03091de25f6eea70d85fe9376349731ecc03fe437175101fd6698929f43a94835c6453b68335f478cfa1fab1ddf0236570ca5a07cebf1aa3c36d7804654a5eac8328377abba3b81627fcac7f1dbdb56ba1f0f861af9967c5d245459a81891fb5dd833f0bca633eb616cf10397b295d857c63501e85fb9f11f1fd3dd80baac425ecf0efa012817ca9b23e06575a3942613fad67b4bda4fabfd29bd1897b0623d6d47ec000bd656f5b7c78b9a4808ac022524b17a8df676b86dc29b6d008d09cb1148110bd07464c071504d7dae5803602247da1e4cd5d490771322d7eb568d0ad0293f4d2626ac0f60f568a92eccd097f6d5247e043b7cdb52ddfef0516e7053fb42b7d1b16564f1c862c1bf45436290a5dab1f0e90b24bdd4433ce0cbcc7b0eafc445dcc6fe8a52e606d3977ce6d9e44f037ea8dbf36bce63a877aaafde13b1bb5005856d315f30fd4feaf26ef8eeef899802aa2442364c147b074c64878a696a1f2cadd9bacb187b62c239c16f163d6c44e157dd8daa4610142eb40dadbc3405c4ade7d127db20bc4384bd1d4c2a2a5dc907aa0468c2485654bceeee3d4011d74e6e85ed88811ccf1cd6b3d5540c5709b8e14fb9e610b552502343ec739e8c9c6d6459062f76275de1fa1b24ed8a9924ea9176dfb89520b7fbec9e9968bd0320afc513e560966b524a82ef5a206f1823742e820bbbe6dca6b0a33c8f04208376bfd01f049f666c735b1efe2550a8601b1839bf045c56a9772a3e25235d2fb61f9007713ff57ae47f6335a44e6730bdaaebe833996aaaa78138ddb7d8719570a429debb8183fbd07f71a037335ec5b1d40c62f7163b85dc71d8db536c9092f155429b65ea81f8ff3c7892ebf881c107ea2c167df47d044ae7ed3fb5328d673753450c82d7049dfeaf1dde821a0ee0d6676a1656584cdbd4532f8d2493ea4794d88acacb147f19ca15777a67fe5031991ebc45ea43e87574f9d2f52de0722d6cc7f5b7a378a461148f1f7c5ee8bc7c7ae4fe80b4eed13b35d16906a084120c645812db0bd70e419c004512f284ab7635f17ee2ecc728aef2cda256b86fb4cc9d3e21736249735962d6ccd307a67fdbdb0815184f116eb1747de19449c6fb9410cb669fa2a3f2ab5ca16c3cca918555b583f61f2126aa0895ccdac7a5604ca1e84a76c15c508d620bb9037e5e5acf97e94438a059bc771d84dc1f63fd3f4780274a2f0a03f9b09a0cf4638e0c317f6ebb24f9062fe8c7023d4c06f3c67c9ac2008e8da33150302b06092a864886f70d010914311e1e1c006d0079005f00630065007200740069006600690063006100740065302106092a864886f70d0109153114041254696d6520313334303937373036353139303082044b06092a864886f70d010706a082043c308204380201003082043106092a864886f70d0107013028060a2a864886f70d010c0106301a04147d79e2d2b2986ea4d929b3ba8b956739a393b00802020400808203f82c0ebc2a236e5ffc4dff9e02344449f642fdf3b16da9b2e56d5a5e35f323b23b8ff915fbaf2ff70705465170ccd259a70bb1cde9f76e593f9a7a0d4764806dad2fa5c3b1ee2711e9dbbcaa874f8985f1b6c2ca1d55c919cf9e88aababe7826107cdb937e7cca57809b20a6351504ab688327e4df957a3c167772cf66aed6a2007ead81896465d4931efe7c3291a49761f1428c766fd82e1736218e90d9f8592475d164d9a79f3424cb6a543f7040d3f0dba6996d496f4f603b7d59527e5c9c89b3f96c55fa73b72385629cbd606cf9f88833db66bb1519dee62a0cd4989d93457fa1162b594b86bc7134c9aa530fe10d62b914f1818395f82d5224c3bc793a04b0ab41dc98694535f5bfbf2aa943d6c794f407e02248be842c55789091d1cc28bbfdf86bc1346142b057558ce1e64e38f8b2d7d68d539150f3de23f43d59637ae678f3687e69b52fdf46f54c32b84a658a2a69fb16da7ebb45ea84c9e38d6cedfc1227b86a6ea3094d0908d588213834192849fa5c25b2460bb22fdd9d9e317efaca646ea582ecb50f6a466f55ae38573afe904eadf42b6c596c8740dbf92cbd38c347624f3399ac2d20d0727f897f38417901dfdaa798631af8992fcad5d708882576036531d2deb867fe46d63921dc50b8c73fbc59586a861d7ae47c2a5ff892e9dffc6d8e6e8161506819ebc020cfb7bc4c1708832d53f8cc864012ab8379a1323e23b0edb5ffe48a942411cef6197f5545ae6822a3096db972f96d4d200ba600a1e95595d4532e7a9861b233f71ff37ea3c19143c87dd6d4a3f3186a7693dc11067c7b4c967984d4bbbf9d88acacb1ff3ba4536ea265a0503865d86af408748fe8191119cd7b570b5352f190265d5d468e911ba0020b526d3892119fda21243568cfa638251c9044c91a88d2f8a05dd0d90088b0b79ac2a2ca263aa108160a7f6943ce709a02743afb6e4ec9a7f7535635f839c2baf938418accec3d5c1ad2bbcec69ab337155bd0bb1b45c7e16e32f251d4da7796f013d6d502581853da6ab9736382115141886c14512fb5ca22e3e9e20366257579eb4225a6a3716457b9b1c0df63cb71a34b888de021f3520d62e96675ea8767e23d55b50e9aa40babafe398f5482c83f8caa57d7ed3486ce7dedace7158067194892defe38af28c1695cd6f14a1ddae959541fab3b59e72c17d2a67d980c749ef00b1f61ece68d81c79b4ec4f4d9eeaad43895a0dc9d86f4d7fe114f01189b3db72ee92963d4403c3aca8bf6d60ef7ee7fcd8102b3247048b4d517cd0ab76a0f8d68d33733934cb35a8e40d7de70c4f166c453fda74553069c51dd33f6f513bb9ef0a983187fc7d896c668590577a4e269688cc7b9fbd1f3fe77d3f431cf002043c43e1cae82b22018931f1337ee276d49c19163a866ef10a64ac5b013db1cb1c303d3021300906052b0e03021a05000414501f5cd8e454e44b6925715c4d2605a8d4ce70d00414456a2344e138862de7ad2e0b274952ef566e2b6302020400", "my_password"},
	{"$pfx$*1702*308206a20201033082066806092a864886f70d010701a082065904820655308206513082032f06092a864886f70d010706a08203203082031c0201003082031506092a864886f70d010701301c060a2a864886f70d010c0103300e04086c933ea5111fd24602020800808202e83c56ad18c45e54aaca4d170750cfbfb3059d6cf161e49d379eab15e722216cb479eee8da7b6e6ffc89e01fbf30f4eb5e1b88ca146c166c700a68473d25a0979344cc60d1e58230a12d24b8be6e9174d3afecdf111cd7d96527831ac9c8f4bf3817cda021f34b61899f2a75fe511e8dedfb70367fa9902d2d3e500f853cc5a99ec8672a44713d24ae49382a20db6349bc48b23ad8d4be3aa31ba7e6d720590b5e4f6b0b5d84b7789ae9da7a80bfa3c27e507fc87e7bc943cff967db6b76f904ac52c1db5cfe9915fa3493cd42b8db6deae62bc01593e34bc8598f27a24cdfd242701ff72d997f959f3a933ab5a2762df33849c116715b78cb0d83267aff913619cbbdf003e13318e4b188a8a4f851b9f59ae2c71ab215c565f7872e5d54c06f92d6f59eaf19d95f9b4526d52d289cd17bc0c2079f9f13c20a70a566773d90ca6d888386d909b6362cb79e15cf547dceab1fe793c577b70f72463969f7b416fb5a6228053363558df18588b53406343ab320a1bbf1757b67ef8e3075f44dee4521f4a461d37ea894c940bc87f9bd33276f2843ff5922fd8e61d22a8619ad23154880fd7d957c0f151458fc4f686d96695a823b08c1795daaf79e41118a3c57ee065a693853a9c4b2004440662f51d63bb9973dc4bb8c541d424416c57d01a825be4d31dab7c7f4b2b738e4bbfdda1e3d3b95e026dadee4dfe155c0f4a24991693f679b452516bc19eab7cf7eb41b476358583d46630e8cda55974b8fcbe25b93e91e73f584a913149137c1c20d13f38826d8dba9bcf5504b8cee77e20a19d6fb050e9213b8aeb11c26a871c600701aea352ba2dcea15d8010d25034f64aa488b580b8282d226f8203bba6aa424b0a25bcceb9c7c718b6c276022d988ca063d2e88350d68903f95aa3265b44d909c07fa9477a5dfcfe3b5ed49b789d6e1c13aca012630343021dbc0c0f17dae6688eae495b76d21be49ced2c2e98e1068d8725d8a581958fb2530871dff1b3f910ae8beb3bc07bfb4b1d2d73fc5d440dc9bcd32ba656c32e357051bef3082031a06092a864886f70d010701a082030b0482030730820303308202ff060b2a864886f70d010c0a0102a08202a6308202a2301c060a2a864886f70d010c0103300e0408749558ace83617660202080004820280ef790b9cd427ec99a350a6e3afb1727cf3dd859d5377897805a7093e1ca42ab8cccc6c52d2b86d61ed55b5bd743fb2a4ec556b438933a9d97a55e5ad1fb3f9967e550be3d708feb5c7287e31afed165a4a91bd5a80292a1e061f97a8c11339963843348badf3fd898e89fd92bda5ad0195d8d4f75e7bce9f0518eeb85365860cd32ad5cea0958efef02bfb74aec0af0765729dae079f5eb08b099d3b06a9b9c6cd6f1e1e4170208ebec3c61ae3421e90cef0f2b5cd2187e43cc4ceecf4aec06340f886efb94f517e578d13659392246a69505de3914b719fba74709ef0f03f010429f899dbddab950f6e58462b2fe2663986a5e0c8ff235e89bca3bb6e41fcd602a0277a83822ac1a14101c83fd1cafdc45c1980ecf54ef092deb2fea736b428158e0847256fc1211f94ea8075145be5a5fb26206e125d55f45500844f1a83f063d0be19b60427dadbd89109bb9ee31a1ac79c863204e8e80c044b8b6bc45c756c26be514e4270a293faf4608065a27b4a51253cb9f831614d5c7f25ec1d4e36063e68e4e405c1f4deb98a786c57a376609441f2dcbe6393487b884624570f6cbb02b53f58ea4acb0faedd2931293dc87664a0c589322480686f6613ffb794c3b3b1872cd7a418712a35666b53bd8383f2e7aa6e8a9e20dd3d46cc3aaaaf17841732dde708ba5611ebcc8777fb3f7b65f2cf95992fdf4f5a17ddf01f3ebe5fb6c9cd58cb74553865cbec3c9d391dcc3e96e654faf7be7fdc8d5fb5dff98799e740147d2ca4b6df47560a4a20bd8f30cf5b495f4e919c9efad3aa59491a3e2ba4e53606e2016ce13e8271e70ccd5b57eec99a8604caf5997e648f3eb541769267f9cdf76aa84917ebd8a1f60a973ed22cca9fa0d3589bb77dafed82ea4f8cd19d3146301f06092a864886f70d01091431121e10006f00700065006e00770061006c006c302306092a864886f70d01091531160414a38a6be4b090be5e29259879b75e0e482f4a4dd830313021300906052b0e03021a05000414a790274918578289d80aa9fd0d526923f7b8f4d40408e861d3357729c35f02020800", "openwall"},
	{NULL}
};

struct fmt_main fmt_pfx;

static void init(struct fmt_main *pFmt)
{
	/* OpenSSL init, cleanup part is left to OS */
	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();

#if defined(_OPENMP) && OPENSSL_VERSION_NUMBER >= 0x10000000
	if (SSLeay() < 0x10000000) {
		fprintf(stderr, "Warning: compiled against OpenSSL 1.0+, "
		    "but running with an older version -\n"
		    "disabling OpenMP for pfx because of thread-safety issues "
		    "of older OpenSSL\n");
		fmt_pfx.params.min_keys_per_crypt =
		    fmt_pfx.params.max_keys_per_crypt = 1;
		fmt_pfx.params.flags &= ~FMT_OMP;
	}
	else {
		int omp_t = 1;
		omp_t = omp_get_max_threads();
		pFmt->params.min_keys_per_crypt *= omp_t;
		omp_t *= OMP_SCALE;
		pFmt->params.max_keys_per_crypt *= omp_t;
	}
#endif
	saved_key = mem_calloc_tiny(sizeof(*saved_key) *
			pFmt->params.max_keys_per_crypt, MEM_ALIGN_NONE);
	any_cracked = 0;
	cracked_size = sizeof(*cracked) * pFmt->params.max_keys_per_crypt;
	cracked = mem_calloc_tiny(cracked_size, MEM_ALIGN_WORD);
}

static int valid(char *ciphertext, struct fmt_main *pFmt)
{
	return !strncmp(ciphertext, "$pfx$", 5);
}

static void *get_salt(char *ciphertext)
{
	char *decoded_data;
	int i;
	char *ctcopy = strdup(ciphertext);
	char *keeptr = ctcopy;
	char *p;
	static struct custom_salt cs;
	PKCS12 *p12 = NULL;
	BIO *bp;
	ctcopy += 6;	/* skip over "$pfx$*" */
	p = strtok(ctcopy, "*");
	cs.len = atoi(p);
	decoded_data = (char *) malloc(cs.len + 1);
	p = strtok(NULL, "*");
	for (i = 0; i < cs.len; i++)
		decoded_data[i] = atoi16[ARCH_INDEX(p[i * 2])] * 16 +
    			atoi16[ARCH_INDEX(p[i * 2 + 1])];
	decoded_data[cs.len] = 0;
	/* load decoded data into OpenSSL structures */
	bp = BIO_new(BIO_s_mem());
	if (!bp) {
		fprintf(stderr, "OpenSSL BIO allocation failure\n");
		exit(-2);
	}
	BIO_write(bp, decoded_data, cs.len);
	if(!(p12 = d2i_PKCS12_bio(bp, NULL))) {
		perror("Unable to create PKCS12 object from bio\n");
		exit(-3);
	}
	/* save custom_salt information */
	memset(&cs, 0, sizeof(cs));
	memcpy(&cs.pfx, p12, sizeof(PKCS12));
	BIO_free(bp);
	if (decoded_data)
		free(decoded_data);
	free(keeptr);
	return (void *) &cs;
}

static void set_salt(void *salt)
{
	cur_salt = (struct custom_salt *) salt;
	if (any_cracked) {
		memset(cracked, 0, cracked_size);
		any_cracked = 0;
	}
}

static void pfx_set_key(char *key, int index)
{
	int len = strlen(key);
	if (len > PLAINTEXT_LENGTH)
		len = PLAINTEXT_LENGTH;
	memcpy(saved_key[index], key, len);
	saved_key[index][len] = 0;
}

static char *get_key(int index)
{
	return saved_key[index];
}

static void crypt_all(int count)
{
	int index = 0;
#if defined(_OPENMP) && OPENSSL_VERSION_NUMBER >= 0x10000000
#pragma omp parallel for
	for (index = 0; index < count; index++)
#endif
	{
		if(PKCS12_verify_mac(&cur_salt->pfx, saved_key[index], -1))
			any_cracked = cracked[index] = 1;
	}
}

static int cmp_all(void *binary, int count)
{
	return any_cracked;
}

static int cmp_one(void *binary, int index)
{
	return cracked[index];
}

static int cmp_exact(char *source, int index)
{
	return cracked[index];
}

struct fmt_main fmt_pfx = {
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
#if defined(_OPENMP) && OPENSSL_VERSION_NUMBER >= 0x10000000
		FMT_OMP |
#endif
		FMT_CASE | FMT_8_BIT,
		pfx_tests
	}, {
		init,
		fmt_default_prepare,
		valid,
		fmt_default_split,
		fmt_default_binary,
		get_salt,
		{
			fmt_default_binary_hash
		},
		fmt_default_salt_hash,
		set_salt,
		pfx_set_key,
		get_key,
		fmt_default_clear_keys,
		crypt_all,
		{
			fmt_default_get_hash
		},
		cmp_all,
		cmp_one,
		cmp_exact
	}
};