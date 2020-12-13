/*
 * cebsocket is a lightweight WebSocket library for C
 *
 * https://github.com/rohanrhu/cebsocket
 *
 * Licensed under MIT
 * Copyright (C) 2020, Oğuzhan Eroğlu (https://oguzhaneroglu.com/) <rohanrhu2@gmail.com>
 *
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

#include "../include/util.h"

static int is_verbose = 1;

extern void cebsocket_util_verbose(const char* format, ...) {
    if (!is_verbose) {
        return;
    }

    printf("[cebsocket] ");

    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
}

extern void cebsocket_util_set_verbose(int _is_verbose) {
    is_verbose = _is_verbose;
}

extern char* cebsocket_util_base64_encode(char* str) {
    unsigned int str_len = strlen(str);
    
    unsigned int encoded_size = 4 * ceil(((double)str_len / 3));
    encoded_size = encoded_size + (encoded_size % 4);
    
    char* buffer = malloc(encoded_size+1);
    buffer[encoded_size] = '\0';

    FILE* buff_stream = fmemopen(buffer, encoded_size+1, "w");

    BIO *bio;
    BIO *b64;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_fp(buff_stream, BIO_NOCLOSE);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, bio);
    BIO_write(b64, str, str_len);
    BIO_flush(b64);

    fclose(buff_stream);

    return buffer;
}

extern char* cebsocket_util_base64_decode(char* str) {
    unsigned int str_len = strlen(str);
    
    unsigned int decoded_size = 3 * ceil(str_len / 4);
    decoded_size = decoded_size - ((str[str_len-1] == '=') + (str[str_len-2] == '='));
    
    char* buffer = malloc(decoded_size+1);
    buffer[decoded_size] = '\0';

    FILE* str_stream = fmemopen(str, str_len, "r");

    BIO *b64;
    BIO *bio;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_fp(str_stream, BIO_NOCLOSE);
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    buffer[BIO_read(bio, buffer, str_len)] = '\0';
    BIO_free_all(b64);

    fclose(str_stream);

    return buffer;
}