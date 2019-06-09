#ifndef __ETHEREUM_TX_H__
#define __ETHEREUM_TX_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>


#include "rlp.h"
#include "sha3.h"
#include "secp256k1.h"
#include "ecdsa.h"
#undef __BIGNUM_H__
#include "bn.h"

#if DEBUG 
#define CHAIN_ID 508674158
#else
#define CHAIN_ID 49262
#endif


typedef struct ETH_FIELD
{
    uint8_t bytes[64];
    uint32_t size;
} ETH_FIELD;

typedef struct ETH_DATA
{
    uint8_t bytes[256];
    uint32_t size;
} ETH_DATA;

/**
 * Ethereum Transaction Structure.
 */
typedef struct ETH_TX
{
    // Provided Fields
    ETH_FIELD nonce;
    ETH_FIELD gas_price;
    ETH_FIELD gas_limit;
    ETH_FIELD to; // contract address
    ETH_FIELD value;
    ETH_DATA data;
    uint32_t chain_id; // Should be always the same. (EVAN NETWORK 49262)

    // To be calculated
    ETH_FIELD sig;
    ETH_FIELD sig_v;
} ETH_TX;

void init_tx(ETH_TX *tx);
void build(ETH_TX *tx, uint8_t tx_hash[32]);
bool sign(uint8_t tx_hash[32], uint32_t chain_id, uint8_t sig[64], uint8_t sig_v[32], uint32_t *sig_v_size);
bool get_ethereum_tx(ETH_TX *tx, uint8_t *rlp_tx, uint32_t *tx_size);

// To be defined
void get_private_key(uint8_t priv_key[32]);

#endif //__ETHEREUM_TX_H__