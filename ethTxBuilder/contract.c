#include "contract.h"

/*
 * Adds parameter value to the contract method. DO NOT USE DIRECTLY.
*/
static uint8_t *add_parameter(uint8_t *data, uint8_t *arg, uint32_t arg_size)
{
    // Fill with function signature
    memcpy(data + ARGUMENT_MAX_SIZE - arg_size, arg, arg_size);
    return data + ARGUMENT_MAX_SIZE;
}

/*
 * Builds data field when calling a contract method with one or multiple arguments.
*/
bool build_raw_data_input(uint8_t function_signature[FUNCTION_SIGNATURE_SIZE], uint8_t parameters[][32], uint32_t *parameter_sizes, uint32_t num_parameters, uint8_t *data,  uint32_t data_max_size)
{

    if(data_max_size < ARGUMENT_MAX_SIZE * num_parameters + FUNCTION_SIGNATURE_SIZE)
        return false;

    // Clears buffer
    memset(data, 0, ARGUMENT_MAX_SIZE + FUNCTION_SIGNATURE_SIZE);
    memcpy(data, function_signature, FUNCTION_SIGNATURE_SIZE);
    data += FUNCTION_SIGNATURE_SIZE;

    for(uint32_t i = 0; i < num_parameters; i++)
    {
        if(parameter_sizes[i] > ARGUMENT_MAX_SIZE)
            return false;

        data = add_parameter(data, parameters[i], parameter_sizes[i]);
    }

    return true;
}