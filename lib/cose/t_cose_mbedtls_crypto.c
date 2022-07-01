/*
 *  t_cose_openssl_crypto.c
 *
 * Copyright 2019-2022, Laurence Lundblade
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * See BSD-3-Clause license in README.md
 *
 * Created 3/31/2019.
 */


#include "t_cose_crypto.h" /* The interface this code implements */

#include <stdlib.h>
#include "mbedtls/ecp.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/md.h"
#include "mbedtls/error.h"

/**
 * \file t_cose_openssl_crypto.c
 *
 * \brief Crypto Adaptation for t_cose to use OpenSSL ECDSA and hashes.
 *
 * This connects up the abstracted crypto services defined in
 * t_cose_crypto.h to the OpenSSL implementation of them.
 *
 * Having this adapter layer doesn't bloat the implementation as everything here
 * had to be done anyway -- the mapping of algorithm IDs, the data format
 * rearranging, the error code translation.
 *
 * This code should just work out of the box if compiled and linked
 * against OpenSSL and with the T_COSE_USE_OPENSSL_CRYPTO preprocessor
 * define set for the build.
 *
 * This works with OpenSSL 1.1.1 and 3.0. It uses the APIs common
 * to these two and that are not marked for future deprecation.
 *
 * A few complaints about OpenSSL in comparison to Mbed TLS:
 *
 * OpenSSL mallocs for various things where MBed TLS does not.
 * This makes the OpenSSL code more complex because checks for malloc
 * failures are necessary.
 *
 * There's a lot of APIs in OpenSSL, but there's a needle to thread to
 * get the APIS that are in 1.1.1, 3.0 and not slated for future
 * deprecation.
 *
 * The APIs that fit the above only work for DER-encoded signatures.
 * t_cose encodes signatures in a more simple way. This difference
 * requires the code here to do conversion which increases its size
 * and complexity and requires intermediate buffers and requires more
 * use of malloc.
 *
 * An older version of t_cose (anything from 2021) uses simpler
 * OpenSSL APIs. They still work but may be deprecated in the
 * future. They could be used in use cases where a particular version
 * of the OpenSSL library is selected and reduce code size
 * a llittle.
 */

/**
 * \brief Common checks and conversions for signing and verification key.
 *
 * \param[in] t_cose_key                 The key to check and convert.
 * \param[out] return_ossl_ec_key        The OpenSSL key in memory.
 * \param[out] return_key_size_in_bytes  How big the key is.
 *
 * \return Error or \ref T_COSE_SUCCESS.
 *
 * It pulls the OpenSSL key out of \c t_cose_key and checks
 * it and figures out the number of bytes in the key rounded up. This
 * is also the size of r and s in the signature.
 */
static enum t_cose_err_t
key_convert_and_size(struct t_cose_key     t_cose_key,
                     mbedtls_ecp_keypair **return_mbedtls_ec_key,
                     unsigned             *return_key_size_in_bytes)
{
    enum t_cose_err_t  return_value;
    unsigned           key_len_bytes; /* type unsigned is conscious choice */
    mbedtls_ecp_keypair *mbedtls_ec_key;

    /* Check the signing key and get it out of the union */
    if(t_cose_key.crypto_lib != T_COSE_CRYPTO_LIB_UNIDENTIFIED) {
        return_value = T_COSE_ERR_INCORRECT_KEY_FOR_LIB;
        goto Done;
    }
    if(t_cose_key.k.key_ptr == NULL) {
        return_value = T_COSE_ERR_EMPTY_KEY;
        goto Done;
    }
    mbedtls_ec_key = (mbedtls_ecp_keypair *)t_cose_key.k.key_ptr;

    key_len_bytes = mbedtls_mpi_size(&mbedtls_ec_key->grp.P);

    *return_key_size_in_bytes = key_len_bytes;
    *return_mbedtls_ec_key = mbedtls_ec_key;

    return_value = T_COSE_SUCCESS;

Done:
    return return_value;
}


/*
 * Public Interface. See documentation in t_cose_crypto.h
 */
enum t_cose_err_t t_cose_crypto_sig_size(int32_t           cose_algorithm_id,
                                         struct t_cose_key signing_key,
                                         size_t           *sig_size)
{
    enum t_cose_err_t return_value;
    unsigned          key_len_bytes;
    mbedtls_ecp_keypair   *keypair;

    if(!t_cose_algorithm_is_ecdsa(cose_algorithm_id)) {
        return_value = T_COSE_ERR_UNSUPPORTED_SIGNING_ALG;
        goto Done;
    }

    return_value = key_convert_and_size(signing_key, &keypair, &key_len_bytes);
    if(return_value != T_COSE_SUCCESS) {
        goto Done;
    }

    /* Double because signature is made of up r and s values */
    *sig_size = key_len_bytes * 2;

    return_value = T_COSE_SUCCESS;

Done:
    return return_value;
}


/*
 * See documentation in t_cose_crypto.h
 */
enum t_cose_err_t
t_cose_crypto_sign(const int32_t                cose_algorithm_id,
                   const struct t_cose_key      signing_key,
                   const struct q_useful_buf_c  hash_to_sign,
                   const struct q_useful_buf    signature_buffer,
                   struct q_useful_buf_c       *signature)
{
    mbedtls_ecp_keypair   *keypair;
    enum t_cose_err_t      return_value;
    unsigned               key_size;
    mbedtls_md_type_t      md_type;
    mbedtls_mpi r;
    mbedtls_mpi s;

    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

    if(!t_cose_algorithm_is_ecdsa(cose_algorithm_id)) {
        return_value = T_COSE_ERR_UNSUPPORTED_SIGNING_ALG;
        goto Done;
    }

    switch(cose_algorithm_id) {

    case T_COSE_ALGORITHM_ES256:
        md_type = MBEDTLS_MD_SHA256;
        break;

#ifndef T_COSE_DISABLE_ES384
    case T_COSE_ALGORITHM_ES384:
        md_type = MBEDTLS_MD_SHA256;
        break;
#endif

#ifndef T_COSE_DISABLE_ES512
    case T_COSE_ALGORITHM_ES512:
        md_type = MBEDTLS_MD_SHA256;
        break;
#endif

    default:
        return T_COSE_ERR_UNSUPPORTED_SIGNING_ALG;
    }

    /* Get the verification key in an EVP_PKEY structure which is what
     * is needed for sig verification. This also gets the key size
     * which is needed to convert the format of the signature. */
    return_value = key_convert_and_size(signing_key,
                                        &keypair,
                                        &key_size);
    if(return_value != T_COSE_SUCCESS) {
        goto Done;
    }

    if (mbedtls_ecdsa_sign_det(&keypair->grp,
            &r, &s, &keypair->d,
            hash_to_sign.ptr, hash_to_sign.len, md_type) != 0) {
        return_value = T_COSE_ERR_SIG_FAIL;
        goto Done;
    }

    if (signature_buffer.len < key_size * 2) {
        return_value = T_COSE_ERR_SIG_FAIL;
        goto Done;
    }

    if (mbedtls_mpi_write_binary(&r, signature_buffer.ptr, key_size) != 0) {
        return_value = T_COSE_ERR_SIG_FAIL;
        goto Done;
    }

    if (mbedtls_mpi_write_binary(&r, signature_buffer.ptr + key_size, key_size) != 0) {
        return_value = T_COSE_ERR_SIG_FAIL;
        goto Done;
    }

    *signature = (UsefulBufC){ signature_buffer.ptr, key_size * 2 };

Done:
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);

    return return_value;
}



/*
 * See documentation in t_cose_crypto.h
 */
enum t_cose_err_t
t_cose_crypto_verify(const int32_t                cose_algorithm_id,
                     const struct t_cose_key      verification_key,
                     const struct q_useful_buf_c  kid,
                     const struct q_useful_buf_c  hash_to_verify,
                     const struct q_useful_buf_c  cose_signature)
{
    mbedtls_ecp_keypair   *keypair;
    enum t_cose_err_t      return_value;
    unsigned               key_size;
    mbedtls_mpi r;
    mbedtls_mpi s;

    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

    /* This implementation doesn't use any key store with the ability
     * to look up a key based on kid. */
    (void)kid;

    if(!t_cose_algorithm_is_ecdsa(cose_algorithm_id)) {
        return_value = T_COSE_ERR_UNSUPPORTED_SIGNING_ALG;
        goto Done;
    }

    /* Get the verification key in an EVP_PKEY structure which is what
     * is needed for sig verification. This also gets the key size
     * which is needed to convert the format of the signature. */
    return_value = key_convert_and_size(verification_key,
                                        &keypair,
                                        &key_size);
    if(return_value != T_COSE_SUCCESS) {
        goto Done;
    }

    /* Check the signature length against expected */
    if(cose_signature.len != key_size * 2) {
        return_value = T_COSE_ERR_SIG_VERIFY;
        goto Done;
    }

    if (mbedtls_mpi_read_binary(&r, cose_signature.ptr, key_size) != 0) {
        return_value = T_COSE_ERR_INSUFFICIENT_MEMORY;
        goto Done;
    }

    if (mbedtls_mpi_read_binary(&s, (const uint8_t *)cose_signature.ptr + key_size, key_size) != 0) {
        return_value = T_COSE_ERR_INSUFFICIENT_MEMORY;
        goto Done;
    }

    if (mbedtls_ecdsa_verify(&keypair->grp,
            hash_to_verify.ptr, hash_to_verify.len,
            &keypair->Q, &r, &s) != 0) {
        return_value = T_COSE_ERR_SIG_VERIFY;
        goto Done;
    }

    /* Everything succeeded */
    return_value = T_COSE_SUCCESS;

Done:
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);

    return return_value;
}

struct hash_context_mbedtls {
    mbedtls_md_context_t md_ctx;
    int          update_error; /* Used to track error return by SHAXXX_Update() */
};


/*
 * See documentation in t_cose_crypto.h
 */
enum t_cose_err_t
t_cose_crypto_hash_start(struct t_cose_crypto_hash *hash_ctx,
                         int32_t                    cose_hash_alg_id)
{
    mbedtls_md_type_t md_type;

    switch(cose_hash_alg_id) {

    case COSE_ALGORITHM_SHA_256:
        md_type = MBEDTLS_MD_SHA256;
        break;

#ifndef T_COSE_DISABLE_ES384
    case COSE_ALGORITHM_SHA_384:
        md_type = MBEDTLS_MD_SHA384;
        break;
#endif

#ifndef T_COSE_DISABLE_ES512
    case COSE_ALGORITHM_SHA_512:
        md_type = MBEDTLS_MD_SHA512;
        break;
#endif

    default:
        return T_COSE_ERR_UNSUPPORTED_HASH;
    }

    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(md_type);
    if(md_info == NULL){
        return T_COSE_ERR_UNSUPPORTED_HASH;
    }

    struct hash_context_mbedtls *ctx = malloc(sizeof *ctx);
    if (ctx == NULL) {
        return T_COSE_ERR_INSUFFICIENT_MEMORY;
    }

    mbedtls_md_init(&ctx->md_ctx);
    int ret = mbedtls_md_setup(&ctx->md_ctx, md_info, 0);
    if (ret != 0) {
        free(ctx);
        return T_COSE_ERR_HASH_GENERAL_FAIL;
    }
    ret = mbedtls_md_starts(&ctx->md_ctx);
    if (ret != 0) {
        free(ctx);
        return T_COSE_ERR_HASH_GENERAL_FAIL;
    }

    ctx->update_error = 0;
    hash_ctx->context.ptr = ctx;

    return T_COSE_SUCCESS;
}


/*
 * See documentation in t_cose_crypto.h
 */
void
t_cose_crypto_hash_update(struct t_cose_crypto_hash *hash_ctx,
                          struct q_useful_buf_c data_to_hash)
{
    struct hash_context_mbedtls *ctx = hash_ctx->context.ptr;
    if (ctx->update_error == 0) {
        if (data_to_hash.ptr) {
            ctx->update_error = mbedtls_md_update(&ctx->md_ctx, data_to_hash.ptr, data_to_hash.len);
        }
    }
}

/*
 * See documentation in t_cose_crypto.h
 */
enum t_cose_err_t
t_cose_crypto_hash_finish(struct t_cose_crypto_hash *hash_ctx,
                          struct q_useful_buf        buffer_to_hold_result,
                          struct q_useful_buf_c     *hash_result)
{
    struct hash_context_mbedtls *ctx = hash_ctx->context.ptr;
    enum t_cose_err_t ret = T_COSE_ERR_HASH_GENERAL_FAIL;

    if (ctx->update_error != 0) {
        goto err;
    }

    size_t hash_len = mbedtls_md_get_size(ctx->md_ctx.md_info);
    if (buffer_to_hold_result.len < hash_len) {
        goto err;
    }

    int r = mbedtls_md_finish(&ctx->md_ctx, buffer_to_hold_result.ptr);
    if (r != 0) {
        goto err;
    }

    *hash_result = (UsefulBufC){buffer_to_hold_result.ptr, hash_len};

    ret = T_COSE_SUCCESS;
err:
    mbedtls_md_free(&ctx->md_ctx);
    free(ctx);

    return ret;
}

