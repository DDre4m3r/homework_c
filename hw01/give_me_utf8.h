//
// Created by DDre4m3r on 10.02.2025.
//

#include <stdio.h>
#include <stdlib.h>

#ifndef GIVE_ME_UTF8_H
#define GIVE_ME_UTF8_H
#include <sys/_types/_size_t.h>

unsigned char* cp1251_to_utf8(unsigned char *buffer, size_t buffer_size);

unsigned char* koi8r_to_utf8(unsigned char *buffer, size_t buffer_size);

unsigned char* iso8859_5_to_utf8(unsigned char *buffer, size_t buffer_size);

#endif
