#ifndef __ETHEREUM_CONTRACT_H__
#define __ETHEREUM_CONTRACT_H__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define ARGUMENT_MAX_SIZE 32
#define FUNCTION_SIGNATURE_SIZE 4

bool build_raw_data_input(uint8_t function_signature[FUNCTION_SIGNATURE_SIZE], uint8_t parameters[][32], uint32_t *parameter_sizes, uint32_t num_parameters, uint8_t *data,  uint32_t data_max_size);

#endif // __ETHEREUM_CONTRACT_H__