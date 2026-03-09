#include "mgit.h"

/* SHA1 implementation is adapted from the FIPS PUB 180-4 specification.
 * It is intentionally straightforward rather than micro-optimized so that
 * the logic is easy to audit for educational purposes.
 */

/* --------- Internal SHA1 helpers --------- */

/**
 * rol32
 *
 * Performs a 32-bit left rotation so that the SHA1 round function can
 * express its mixing operations using standard C.
 */
static uint32_t rol32(uint32_t value, unsigned int bits)
{
    return (value << bits) | (value >> (32 - bits));
}

/**
 * sha1_process_chunk
 *
 * Processes a single 512-bit chunk so that the running SHA1 state
 * can be updated incrementally across the input stream.
 */
static void sha1_process_chunk(const uint8_t *chunk, uint32_t h[5])
{
    uint32_t w[80];
    unsigned int t;

    for (t = 0; t < 16; ++t) {
        w[t] = ((uint32_t)chunk[t * 4] << 24) |
               ((uint32_t)chunk[t * 4 + 1] << 16) |
               ((uint32_t)chunk[t * 4 + 2] << 8) |
               ((uint32_t)chunk[t * 4 + 3]);
    }

    for (t = 16; t < 80; ++t) {
        w[t] = rol32(w[t - 3] ^ w[t - 8] ^ w[t - 14] ^ w[t - 16], 1);
    }

    uint32_t a = h[0];
    uint32_t b = h[1];
    uint32_t c = h[2];
    uint32_t d = h[3];
    uint32_t e = h[4];

    for (t = 0; t < 80; ++t) {
        uint32_t f, k;
        if (t < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999U;
        } else if (t < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1U;
        } else if (t < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDCU;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6U;
        }

        uint32_t temp = rol32(a, 5) + f + e + k + w[t];
        e = d;
        d = c;
        c = rol32(b, 30);
        b = a;
        a = temp;
    }

    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
}

/**
 * sha1
 *
 * Computes a full SHA1 digest of the provided buffer so that mgit can
 * name objects by their content.
 */
void sha1(const uint8_t *data, size_t len, uint8_t out[HASH_SIZE])
{
    uint32_t h[5];
    uint8_t block[64];
    size_t i;

    h[0] = 0x67452301U;
    h[1] = 0xEFCDAB89U;
    h[2] = 0x98BADCFEU;
    h[3] = 0x10325476U;
    h[4] = 0xC3D2E1F0U;

    size_t full_chunks = len / 64;
    for (i = 0; i < full_chunks; ++i) {
        sha1_process_chunk(data + i * 64, h);
    }

    size_t remaining = len % 64;
    memset(block, 0, sizeof(block));
    if (remaining > 0) {
        memcpy(block, data + full_chunks * 64, remaining);
    }

    block[remaining] = 0x80;

    if (remaining >= 56) {
        sha1_process_chunk(block, h);
        memset(block, 0, sizeof(block));
    }

    uint64_t total_bits = (uint64_t)len * 8U;
    block[56] = (uint8_t)((total_bits >> 56) & 0xFF);
    block[57] = (uint8_t)((total_bits >> 48) & 0xFF);
    block[58] = (uint8_t)((total_bits >> 40) & 0xFF);
    block[59] = (uint8_t)((total_bits >> 32) & 0xFF);
    block[60] = (uint8_t)((total_bits >> 24) & 0xFF);
    block[61] = (uint8_t)((total_bits >> 16) & 0xFF);
    block[62] = (uint8_t)((total_bits >> 8) & 0xFF);
    block[63] = (uint8_t)(total_bits & 0xFF);

    sha1_process_chunk(block, h);

    for (i = 0; i < 5; ++i) {
        out[i * 4]     = (uint8_t)((h[i] >> 24) & 0xFF);
        out[i * 4 + 1] = (uint8_t)((h[i] >> 16) & 0xFF);
        out[i * 4 + 2] = (uint8_t)((h[i] >> 8) & 0xFF);
        out[i * 4 + 3] = (uint8_t)(h[i] & 0xFF);
    }
}

/**
 * hash_to_hex
 *
 * Converts binary hash bytes into a hex string so that hash identifiers
 * can be written into the filesystem and shown to users.
 */
void hash_to_hex(uint8_t hash[HASH_SIZE], char hex[HASH_HEX_SIZE + 1])
{
    static const char *digits = "0123456789abcdef";
    size_t i;
    for (i = 0; i < HASH_SIZE; ++i) {
        hex[i * 2]     = digits[(hash[i] >> 4) & 0x0F];
        hex[i * 2 + 1] = digits[hash[i] & 0x0F];
    }
    hex[HASH_HEX_SIZE] = '\0';
}

/**
 * hex_to_hash
 *
 * Converts a hex string back into binary bytes so that mgit can read
 * hashes from text files and use them internally.
 */
int hex_to_hash(const char *hex, uint8_t hash[HASH_SIZE])
{
    size_t len = strlen(hex);
    size_t i;

    if (len < HASH_HEX_SIZE) {
        return MGIT_ERROR;
    }

    for (i = 0; i < HASH_SIZE; ++i) {
        char c1 = hex[i * 2];
        char c2 = hex[i * 2 + 1];
        int v1, v2;

        if (c1 >= '0' && c1 <= '9') v1 = c1 - '0';
        else if (c1 >= 'a' && c1 <= 'f') v1 = c1 - 'a' + 10;
        else if (c1 >= 'A' && c1 <= 'F') v1 = c1 - 'A' + 10;
        else return MGIT_ERROR;

        if (c2 >= '0' && c2 <= '9') v2 = c2 - '0';
        else if (c2 >= 'a' && c2 <= 'f') v2 = c2 - 'a' + 10;
        else if (c2 >= 'A' && c2 <= 'F') v2 = c2 - 'A' + 10;
        else return MGIT_ERROR;

        hash[i] = (uint8_t)((v1 << 4) | v2);
    }
    return 0;
}

/**
 * hash_file
 *
 * Hashes the entire contents of a file so that mgit can detect when a
 * file's content changes independently of its path.
 */
char *hash_file(const char *filepath)
{
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        perror("fopen");
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(f);
        return NULL;
    }
    long size = ftell(f);
    if (size < 0) {
        perror("ftell");
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        perror("fseek");
        fclose(f);
        return NULL;
    }

    uint8_t *buf = (uint8_t *)malloc((size_t)size);
    if (!buf) {
        fprintf(stderr, "mgit: out of memory\n");
        fclose(f);
        return NULL;
    }

    size_t read_len = fread(buf, 1, (size_t)size, f);
    fclose(f);

    if (read_len != (size_t)size) {
        fprintf(stderr, "mgit: failed to read file '%s'\n", filepath);
        free(buf);
        return NULL;
    }

    uint8_t digest[HASH_SIZE];
    sha1(buf, (size_t)size, digest);
    free(buf);

    char *hex = (char *)malloc(HASH_HEX_SIZE + 1);
    if (!hex) {
        fprintf(stderr, "mgit: out of memory\n");
        return NULL;
    }
    hash_to_hex(digest, hex);
    return hex;
}

