#include "gtest/gtest.h"
#include "mbedtls/pk.h"
#include "mbedtls/error.h"
#include "teerng.h"
#include "t_cose/t_cose_common.h"
#include "t_cose/t_cose_sign1_sign.h"
#include "t_cose/t_cose_sign1_verify.h"
#include "t_cose/q_useful_buf.h"

TEST(SuitTest, sign_verify) {
    mbedtls_pk_context ctx;

    const unsigned char prv_key[] =
        "-----BEGIN EC PRIVATE KEY-----\n"
        "MHcCAQEEIELKcPPgZFmM5Ti7aYjUNJBrjvO/d7hi2mnluNN8XX5hoAoGCCqGSM49\n"
        "AwEHoUQDQgAExMJmSeV+ZMSbccfu7+o/Gsw+IhTcr9tM9MRNd+pnVh4mHrMozWjW\n"
        "aWFkMdpOEHZpSKLToh1UsIYXgf+PoPtiMw==\n"
        "-----END EC PRIVATE KEY-----\n";
    const unsigned char pub_key[] =
        "-----BEGIN PUBLIC KEY-----\n"
        "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAExMJmSeV+ZMSbccfu7+o/Gsw+IhTc\n"
        "r9tM9MRNd+pnVh4mHrMozWjWaWFkMdpOEHZpSKLToh1UsIYXgf+PoPtiMw==\n"
        "-----END PUBLIC KEY-----\n";

    mbedtls_pk_init(&ctx);
    int ret = mbedtls_pk_parse_key(&ctx, prv_key, sizeof prv_key, NULL, 0);
    EXPECT_EQ(0, ret);

    unsigned char sign[MBEDTLS_ECDSA_MAX_LEN];
    size_t sign_len = sizeof sign;
    unsigned char hash[32];
    for (int i = 0; i < 32; i++) {
        hash[i] = i;
    }

    ret = mbedtls_pk_sign(&ctx, MBEDTLS_MD_SHA256,
        hash, 32, sign, &sign_len, teerng_read, NULL);
    EXPECT_EQ(0, ret);

    mbedtls_pk_free(&ctx);

    ret = mbedtls_pk_parse_public_key(&ctx, pub_key, sizeof pub_key);
    EXPECT_EQ(0, ret);

    ret = mbedtls_pk_verify(&ctx, MBEDTLS_MD_SHA256, hash, 32, sign, sign_len);
    EXPECT_EQ(0, ret);
}

TEST(SuitTest, verify_signature_signed_by_suit_tool) {
    mbedtls_pk_context ctx;

    const unsigned char pub_key[] =
        "-----BEGIN PUBLIC KEY-----\n"
        "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAExMJmSeV+ZMSbccfu7+o/Gsw+IhTc\n"
        "r9tM9MRNd+pnVh4mHrMozWjWaWFkMdpOEHZpSKLToh1UsIYXgf+PoPtiMw==\n"
        "-----END PUBLIC KEY-----\n";

//         / authentication-wrapper / 2:<<[
//             digest: <<[
//                 / algorithm-id / -16 / "sha256" /,
//                 / digest-bytes / h'637090821cbbb2679542787b49f45e14af0cbfad9ef4a4f0b342b923355605af'
//             ]>>,
//             signature: <<18([
//                     / protected / <<{
//                         / alg / 1:-7 / "ES256" /,
//                     }>>,
//                     / unprotected / {
//                     },
//                     / payload / F6 / nil /,
//                     / signature / h'51996636b7b0ac47bf0a9e93b41b088363c83c782bd5145f0f660bede78fd3624f679012262417207f24aaf1a1261eb868f113227bee58ec11dd2b9c76b7f922'
//                 ])>>
//             ]
//         ]>>,

    int ret;

    mbedtls_md_context_t md_ctx;
    mbedtls_md_init(&md_ctx);

    ret = mbedtls_md_setup(&md_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    EXPECT_EQ(0, ret);
    ret = mbedtls_md_starts(&md_ctx);
    EXPECT_EQ(0, ret);

    unsigned char buf[32];
    buf[0] = 0x84; // array(4)
    ret = mbedtls_md_update(&md_ctx, buf, 1);
    EXPECT_EQ(0, ret);

    buf[0] = 0x6A; //   text(10) "Signature1"
    ret = mbedtls_md_update(&md_ctx, buf, 1);
    EXPECT_EQ(0, ret);
    ret = mbedtls_md_update(&md_ctx, (unsigned char *)"Signature1", 10);
    EXPECT_EQ(0, ret);

    buf[0] = 0x43; //   bstr(3)
    buf[1] = 0xA1; //     map(1)
    buf[2] = 0x01; //       uint(1)   // alg
    buf[3] = 0x26; //       nint(-7)  // ES256
    ret = mbedtls_md_update(&md_ctx, buf, 4);
    EXPECT_EQ(0, ret);

    buf[0] = 0x40; //   bstr(0)
    ret = mbedtls_md_update(&md_ctx, buf, 1);
    EXPECT_EQ(0, ret);

    buf[0] = 0x58; //
    buf[1] = 0x24; //   bstr(36)
    buf[2] = 0x82; //     array(2)
    buf[3] = 0x2F; //       nint(-16)  // SHA-256
    buf[4] = 0x58; //
    buf[5] = 0x20; //       bstr(32)
    ret = mbedtls_md_update(&md_ctx, buf, 6);
    EXPECT_EQ(0, ret);
    const unsigned char payload_digest[] = {
        0x63, 0x70, 0x90, 0x82, 0x1c, 0xbb, 0xb2, 0x67,
        0x95, 0x42, 0x78, 0x7b, 0x49, 0xf4, 0x5e, 0x14,
        0xaf, 0x0c, 0xbf, 0xad, 0x9e, 0xf4, 0xa4, 0xf0,
        0xb3, 0x42, 0xb9, 0x23, 0x35, 0x56, 0x05, 0xaf
    };
    ret = mbedtls_md_update(&md_ctx, payload_digest, 32);
    EXPECT_EQ(0, ret);

    unsigned char digest[32];
    ret = mbedtls_md_finish(&md_ctx, digest);

    const unsigned char signature[] = {
        0x51, 0x99, 0x66, 0x36, 0xb7, 0xb0, 0xac, 0x47,
        0xbf, 0x0a, 0x9e, 0x93, 0xb4, 0x1b, 0x08, 0x83,
        0x63, 0xc8, 0x3c, 0x78, 0x2b, 0xd5, 0x14, 0x5f,
        0x0f, 0x66, 0x0b, 0xed, 0xe7, 0x8f, 0xd3, 0x62,
        0x4f, 0x67, 0x90, 0x12, 0x26, 0x24, 0x17, 0x20,
        0x7f, 0x24, 0xaa, 0xf1, 0xa1, 0x26, 0x1e, 0xb8,
        0x68, 0xf1, 0x13, 0x22, 0x7b, 0xee, 0x58, 0xec,
        0x11, 0xdd, 0x2b, 0x9c, 0x76, 0xb7, 0xf9, 0x22
    };

    mbedtls_pk_init(&ctx);
    ret = mbedtls_pk_parse_public_key(&ctx, pub_key, sizeof pub_key);
    EXPECT_EQ(0, ret);

    mbedtls_ecdsa_context ecdsa;
    mbedtls_ecdsa_init( &ecdsa );

    mbedtls_ecdsa_from_keypair(&ecdsa, (mbedtls_ecp_keypair *)ctx.pk_ctx);
    EXPECT_EQ(0, ret);

    mbedtls_mpi r;
    mbedtls_mpi s;

    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

    size_t p_bytes = mbedtls_mpi_size(&ecdsa.grp.P);
    EXPECT_EQ(p_bytes * 2, sizeof signature);

    ret = mbedtls_mpi_read_binary(&r, signature, p_bytes);
    EXPECT_EQ(0, ret);
    ret = mbedtls_mpi_read_binary(&s, signature + p_bytes, p_bytes);
    EXPECT_EQ(0, ret);

    ret = mbedtls_ecdsa_verify(&ecdsa.grp, digest, 32,
        &ecdsa.Q, &r, &s
    );
    EXPECT_EQ(0, ret);

    mbedtls_ecdsa_free( &ecdsa );
}

TEST(SuitTest, t_cose_verify_signature_signed_by_suit_tool) {
    enum t_cose_err_t return_value;
    mbedtls_pk_context ctx;

    const unsigned char pub_key[] =
        "-----BEGIN PUBLIC KEY-----\n"
        "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAExMJmSeV+ZMSbccfu7+o/Gsw+IhTc\n"
        "r9tM9MRNd+pnVh4mHrMozWjWaWFkMdpOEHZpSKLToh1UsIYXgf+PoPtiMw==\n"
        "-----END PUBLIC KEY-----\n";

    int ret;

    mbedtls_pk_init(&ctx);
    ret = mbedtls_pk_parse_public_key(&ctx, pub_key, sizeof pub_key);
    EXPECT_EQ(0, ret);

    struct t_cose_key key;
    key.crypto_lib = T_COSE_CRYPTO_LIB_UNIDENTIFIED;
    key.k.key_ptr = (mbedtls_ecp_keypair *)ctx.pk_ctx;

//         / authentication-wrapper / 2:<<[
//             digest: <<[
//                 / algorithm-id / -16 / "sha256" /,
//                 / digest-bytes / h'637090821cbbb2679542787b49f45e14af0cbfad9ef4a4f0b342b923355605af'
//             ]>>,
//             signature: <<18([
//                     / protected / <<{
//                         / alg / 1:-7 / "ES256" /,
//                     }>>,
//                     / unprotected / {
//                     },
//                     / payload / F6 / nil /,
//                     / signature / h'51996636b7b0ac47bf0a9e93b41b088363c83c782bd5145f0f660bede78fd3624f679012262417207f24aaf1a1261eb868f113227bee58ec11dd2b9c76b7f922'
//                 ])>>
//             ]
//         ]>>,

    unsigned char detached_payload[] = {
        0x82, 0x2F, 0x58, 0x20, 0x63, 0x70, 0x90, 0x82,
        0x1C, 0xBB, 0xB2, 0x67, 0x95, 0x42, 0x78, 0x7B,
        0x49, 0xF4, 0x5E, 0x14, 0xAF, 0x0C, 0xBF, 0xAD,
        0x9E, 0xF4, 0xA4, 0xF0, 0xB3, 0x42, 0xB9, 0x23,
        0x35, 0x56, 0x05, 0xAF
    };
    unsigned char cose_signature[] = {
        0xD2, 0x84, 0x43, 0xA1, 0x01, 0x26, 0xA0, 0xF6,
        0x58, 0x40, 0x51, 0x99, 0x66, 0x36, 0xb7, 0xb0,
        0xac, 0x47, 0xbf, 0x0a, 0x9e, 0x93, 0xb4, 0x1b,
        0x08, 0x83, 0x63, 0xc8, 0x3c, 0x78, 0x2b, 0xd5,
        0x14, 0x5f, 0x0f, 0x66, 0x0b, 0xed, 0xe7, 0x8f,
        0xd3, 0x62, 0x4f, 0x67, 0x90, 0x12, 0x26, 0x24,
        0x17, 0x20, 0x7f, 0x24, 0xaa, 0xf1, 0xa1, 0x26,
        0x1e, 0xb8, 0x68, 0xf1, 0x13, 0x22, 0x7b, 0xee,
        0x58, 0xec, 0x11, 0xdd, 0x2b, 0x9c, 0x76, 0xb7,
        0xf9, 0x22
    };

    struct t_cose_sign1_verify_ctx verify_ctx;
    t_cose_sign1_verify_init(&verify_ctx, 0);

    t_cose_sign1_set_verification_key(&verify_ctx, key);

    return_value = t_cose_sign1_verify_detached(
        &verify_ctx,
        UsefulBuf_FROM_BYTE_ARRAY(cose_signature),
        NULL_Q_USEFUL_BUF_C,
        UsefulBuf_FROM_BYTE_ARRAY(detached_payload),
        NULL);
    EXPECT_EQ(T_COSE_SUCCESS, return_value);
}
