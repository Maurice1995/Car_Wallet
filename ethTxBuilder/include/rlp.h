#ifndef __ETHEREUM_RLP_H__
#define __ETHEREUM_RLP_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "sha3.h"

int rlp_calculate_length(int length, uint8_t firstbyte);
void hash_rlp_field(const uint8_t *buf, size_t size);
void hash_data(const uint8_t *buf, size_t size);
void hash_rlp_length(uint32_t length, uint8_t firstbyte);
void hash_rlp_list_length(uint32_t length);
void hash_rlp_number(uint32_t number);

#endif //__ETHEREUM_RLP_H__