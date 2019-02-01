/** License of PBKDF2 */
/*-
 * Copyright 2005,2007,2009 Colin Percival
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/** store32_be license */
/*
 * ISC License
 *
 * Copyright (c) 2013-2018
 * Frank Denis <j at pureftpd dot org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
 * Modified for use with ceema:
 * - Removed static compiler check
 * - replace STORE32_BE with libsodiums LE inline definition
 * - Adjust include paths
 */

#include "pbkdf2-sha256.h"

#include <sodium/core.h>
#include <sodium/crypto_auth_hmacsha256.h>
#include <sodium/crypto_pwhash_scryptsalsa208sha256.h>
#include <sodium/utils.h>

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static inline void store32_be(uint8_t dst[4], uint32_t w) {
    dst[3] = (uint8_t) w; w >>= 8;
    dst[2] = (uint8_t) w; w >>= 8;
    dst[1] = (uint8_t) w; w >>= 8;
    dst[0] = (uint8_t) w;
}

/**
 * PBKDF2_SHA256(passwd, passwdlen, salt, saltlen, c, buf, dkLen):
 * Compute PBKDF2(passwd, salt, c, dkLen) using HMAC-SHA256 as the PRF, and
 * write the output to buf.  The value dkLen must be at most 32 * (2^32 - 1).
 */
void
PBKDF2_SHA256(const uint8_t *passwd, size_t passwdlen, const uint8_t *salt,
              size_t saltlen, uint64_t c, uint8_t *buf, size_t dkLen)
{
    crypto_auth_hmacsha256_state PShctx, hctx;
    size_t                       i;
    uint8_t                      ivec[4];
    uint8_t                      U[32];
    uint8_t                      T[32];
    uint64_t                     j;
    int                          k;
    size_t                       clen;

    crypto_auth_hmacsha256_init(&PShctx, passwd, passwdlen);
    crypto_auth_hmacsha256_update(&PShctx, salt, saltlen);

    for (i = 0; i * 32 < dkLen; i++) {
        store32_be(ivec, (uint32_t)(i + 1));
        memcpy(&hctx, &PShctx, sizeof(crypto_auth_hmacsha256_state));
        crypto_auth_hmacsha256_update(&hctx, ivec, 4);
        crypto_auth_hmacsha256_final(&hctx, U);

        memcpy(T, U, 32);
        /* LCOV_EXCL_START */
        for (j = 2; j <= c; j++) {
            crypto_auth_hmacsha256_init(&hctx, passwd, passwdlen);
            crypto_auth_hmacsha256_update(&hctx, U, 32);
            crypto_auth_hmacsha256_final(&hctx, U);

            for (k = 0; k < 32; k++) {
                T[k] ^= U[k];
            }
        }
        /* LCOV_EXCL_STOP */

        clen = dkLen - i * 32;
        if (clen > 32) {
            clen = 32;
        }
        memcpy(&buf[i * 32], T, clen);
    }
    sodium_memzero((void *) &PShctx, sizeof PShctx);
}
