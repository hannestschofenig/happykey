/*
 * Copyright 2021 Stephen Farrell. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/**
 * @file
 *
 * A round-trip test using NSS to encrypt and my code to decrypt.
 *
 */

/*
 * Openssl includes
 */
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>

/*
 * Happykey include
 */
#include "hpke.h"

#define MEMCHAR 0xfa

/*
 * Our Happykey wrapper for NSS stuff
 */
extern int nss_enc(
        char *pskid, size_t psklen, unsigned char *psk,
        size_t publen, unsigned char *pub,
        size_t privlen, unsigned char *priv,
        size_t clearlen, unsigned char *clear,
        size_t aadlen, unsigned char *aad,
        size_t infolen, unsigned char *info,
        size_t *senderpublen, unsigned char *senderpub,
        size_t *cipherlen, unsigned char *cipher
        );

/*!
 * @brief for odd/occasional debugging
 *
 * @param fout is a FILE * to use
 * @param msg is prepended to print
 * @param buf is the buffer to print
 * @param blen is the length of the buffer
 * @return 1 for success 
 */
static int neod_pbuf(char *msg,unsigned char *buf,size_t blen) 
{
    if (!msg) {
        printf("NULL msg:");
    } else {
        printf("%s (%lu): ",msg,blen);
    }
    if (!buf) {
        printf("buf is NULL, so probably something wrong\n");
        return 1;
    }
    if (blen==HPKE_MAXSIZE) {
        printf("length is HPKE_MAXSIZE, so probably unused\n");
        return 1;
    } 
    if (blen==0) {
        printf("length is 0, so probably something wrong\n");
        return 1;
    }
    size_t i=0;
    for (i=0;i<blen;i++) {
        printf("%02x",buf[i]);
    }
    printf("\n");
    return 1;
}

int main(int argc, char **argv) 
{

    /*
     * Generate a key pair
     */
    int hpke_mode=HPKE_MODE_BASE;
    hpke_suite_t hpke_suite = HPKE_SUITE_DEFAULT;
    size_t publen=HPKE_MAXSIZE; unsigned char pub[HPKE_MAXSIZE];
    memset(pub,MEMCHAR,publen);
    size_t privlen=HPKE_MAXSIZE; unsigned char priv[HPKE_MAXSIZE];
    memset(priv,MEMCHAR,privlen);
#define EVP
#ifdef EVP
    EVP_PKEY *privevp=NULL;
    int rv=hpke_kg_evp(
        hpke_mode, hpke_suite,
        &publen, pub,
        &privevp);
    if (rv!=1) {
        fprintf(stderr,"Error (%d) from hpke_kg\n",rv);
        exit(1);
    } 
#else
    int rv=hpke_kg(
        hpke_mode, hpke_suite,
        &publen, pub,
        &privlen, priv);
    if (rv!=1) {
        fprintf(stderr,"Error (%d) from hpke_kg\n",rv);
        exit(1);
    } 
#endif

#ifndef EVP
    printf("receiver priv:\n%s",priv);
#endif
    neod_pbuf("receiver pub",pub,publen);

    /*
     * Setup AAD/Info buffers etc.
     */
    size_t aadlen=HPKE_MAXSIZE; unsigned char aad[HPKE_MAXSIZE];
    size_t infolen=HPKE_MAXSIZE; unsigned char info[HPKE_MAXSIZE];
    size_t cipherlen=HPKE_MAXSIZE; unsigned char cipher[HPKE_MAXSIZE];
    size_t senderpublen=HPKE_MAXSIZE; unsigned char senderpub[HPKE_MAXSIZE];
    size_t psklen=0; unsigned char *psk=NULL; char *pskid=NULL;
    size_t clearlen=HPKE_MAXSIZE; unsigned char clear[HPKE_MAXSIZE];

    /*
     * Initial values
     */
#define INFO (char*) "The Info"
    memset(info,MEMCHAR,infolen);
    infolen=strlen(INFO); memcpy(info,INFO,strlen(INFO)); neod_pbuf("info",info,infolen);

#define AAD (char*) "aad aad aad lots and lots of aad"
    memset(aad,MEMCHAR,aadlen);
    aadlen=strlen(AAD); memcpy(aad,AAD,strlen(AAD)); neod_pbuf("aad",aad,aadlen);

#define MESSAGE (char*)"we need another trip to the bottle bank"
    memset(clear,MEMCHAR,clearlen);
    clearlen=strlen(MESSAGE); memcpy(clear,MESSAGE,strlen(MESSAGE)); neod_pbuf("clear",clear,clearlen);
    memset(cipher,MEMCHAR,cipherlen);

    /*
     * Call NSS encrypt
     */
    rv=nss_enc(
        pskid, psklen, psk,
        publen, pub,
        0, NULL,
        clearlen, clear,
        aadlen, aad,
        // 0, NULL, // infolen, info,
        infolen, info,
        &senderpublen, senderpub,
        &cipherlen, cipher
        );
    if (rv!=1) {
        printf("Error Encrypting (%d) - exiting\n",rv);
        exit(rv);
    }
    neod_pbuf("sender pub",senderpub,senderpublen);
    neod_pbuf("ciphertext",cipher,cipherlen);
    neod_pbuf("psk",psk,psklen);
    printf("pskid: %s\n",(pskid==NULL?"NULL":pskid));

    /*
     * Call happykey decrypt
     */
    rv=hpke_dec( hpke_mode, hpke_suite,
            pskid, psklen, psk,
            0, NULL, // publen, pub,
#ifdef EVP
            0, NULL, privevp,
#else
            privlen, priv, NULL,
#endif
            senderpublen, senderpub,
            cipherlen, cipher,
            aadlen,aad,
            // 0, NULL, // infolen, info,
            infolen, info,
            &clearlen, clear); 
    if (rv!=1) {
        printf("Error decrypting (%d) - exiting\n",rv);
        exit(rv);
    }
    neod_pbuf("recovered clear",clear,clearlen);

    return 1;
}
