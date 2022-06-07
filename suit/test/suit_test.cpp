#include "gtest/gtest.h"
#include "mbedtls/pk.h"
#include "mbedtls/error.h"
#include "teerng.h"

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
