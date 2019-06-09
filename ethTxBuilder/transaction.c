#include "transaction.h"

SHA3_CTX keccak_ctx;

static int ethereum_is_canonic(uint8_t v, uint8_t signature[64]) {
  (void) signature;
  return (v & 2) == 0;
}

uint8_t test_private_key[32] = {0x3f,0xc8,0x65,0x69,0x19,0x50,0x86,0x14,0xda,0x65,0x6b,0x85,0x1b,0x49,0x71,0x79,0xdc,0x39,0x15,0xfc,0xfb,0x27,0x74,0x21,0x7e,0x56,0x9a,0x48,0xe7,0xf0,0x53,0x89};

void get_private_key(uint8_t priv_key[32])
{
    memcpy(priv_key, test_private_key, 32);
}

/*
 * Converts bignum to array.
*/
void bn_to_array(struct bn *bnum, uint8_t arr[32], uint32_t *size)
{
    *size = 0;
    uint8_t *num = (uint8_t*) bnum->array;
    for(int i = 31; i >= 0; i--)
    {
        // leading 0
        if(num[i] == 0 && *size == 0)
            continue;

        arr[*size] = num[i];
        *size += 1;
    }
}

/*
    Initializes all fields of transaction to 0. Should always be done.
*/
void init_tx(ETH_TX *tx)
{
    memset(tx, 0, sizeof(ETH_TX));
    extern uint8_t encoded_tx[2048];
    extern uint32_t offset_rlp;
    memset(encoded_tx,0, sizeof(encoded_tx));
    offset_rlp = 0;
    tx->chain_id = CHAIN_ID;
}

/*
 * Calculates length of the all elements of the ransaction object.
 * If signature is not present doesn't exist, it includes the chain_id, otherwise, includes the signature and signature_v;
*/
static uint32_t calculate_tx_rlp_length(ETH_TX *tx, uint8_t tx_type)
{
    uint32_t rlp_length = 0;

    rlp_length += rlp_calculate_length(tx->nonce.size, tx->nonce.bytes[0]);
    rlp_length += rlp_calculate_length(tx->gas_price.size, tx->gas_price.bytes[0]);
    rlp_length += rlp_calculate_length(tx->gas_limit.size, tx->gas_limit.bytes[0]);
    rlp_length += rlp_calculate_length(tx->to.size, tx->to.bytes[0]);
    rlp_length += rlp_calculate_length(tx->value.size, tx->value.bytes[0]);
    rlp_length += rlp_calculate_length(tx->data.size, tx->data.bytes[0]);
    if (tx_type) {
        rlp_length += rlp_calculate_length(1, tx_type);
    }


    if(tx->sig.size > 0)
    {
        rlp_length += rlp_calculate_length(32, tx->sig.bytes[0]);
        rlp_length += rlp_calculate_length(32, tx->sig.bytes[32]);

        rlp_length += rlp_calculate_length(tx->sig_v.size, tx->sig_v.bytes[0]);

    }
    else
    {
        int length = tx->chain_id < 0x100 ? 1: tx->chain_id < 0x10000 ? 2: tx->chain_id < 0x1000000 ? 3 : 4;
        rlp_length += rlp_calculate_length(length, tx->chain_id);
        rlp_length += rlp_calculate_length(0, 0);
        rlp_length += rlp_calculate_length(0, 0);
    }
    
    
    return rlp_length;
}

/*
 * Builds the transaction hash to be signed.
*/
void build(ETH_TX *tx, uint8_t tx_hash[32])
{
    sha3_256_Init(&keccak_ctx);

    // TODO: Validate fields bounds

    uint8_t tx_type = 0; // TODO what is this

    /* Stage 2: Store header fields */
    hash_rlp_list_length(calculate_tx_rlp_length(tx, tx_type));

    if (tx_type) {
        hash_rlp_number(tx_type);
    }
    hash_rlp_field(tx->nonce.bytes, tx->nonce.size);
    hash_rlp_field(tx->gas_price.bytes, tx->gas_price.size);
    hash_rlp_field(tx->gas_limit.bytes, tx->gas_limit.size);
    hash_rlp_field(tx->to.bytes, tx->to.size);
    hash_rlp_field(tx->value.bytes, tx->value.size);
    hash_rlp_length(tx->data.size, tx->data.bytes[0]);
    hash_data(tx->data.bytes, tx->data.size);

    if(tx->sig.size == 0)
    {
        hash_rlp_number(tx->chain_id);
        hash_rlp_length(0, 0);
        hash_rlp_length(0, 0);
    }
    else
    {
        hash_rlp_field(tx->sig_v.bytes, tx->sig_v.size);
        hash_rlp_field(&tx->sig.bytes[0], 32);
        hash_rlp_field(&tx->sig.bytes[32], 32);
    }
    
    keccak_Final(&keccak_ctx, tx_hash);
}

/*
 * Calculates the signature_v from the chain_id and a value v coming from the ethereum signature process.
 */
void calculate_sigv(uint32_t chain_id, uint8_t sig_v[32], uint8_t v, uint32_t *sig_v_size)
{
    struct bn bchain_id, mult2, add35, add27, bigv, result;

    bignum_init(&result);
    bignum_from_int(&bigv, v);

   if (chain_id) {
        // *sig_v = v + 2 * chain_id + 35;
        
        bignum_from_int(&bchain_id, chain_id);
        bignum_from_int(&mult2, 2);
        bignum_from_int(&add35, 35);

        bignum_mul(&bchain_id, &mult2, &result);
        bignum_add(&result, &add35, &result);
        bignum_add(&result, &bigv, &result);
        bn_to_array(&result, sig_v, sig_v_size);
    } else {
        // *sig_v = v + 27;
        bignum_from_int(&add27, 27);
        bignum_add(&add27, &bigv, &result);
        bn_to_array(&result, sig_v, sig_v_size);
    }
}

/*
   Signs the given transaction. Gives out the signature, and the signature_v.
*/
bool sign(uint8_t tx_hash[32], uint32_t chain_id, uint8_t sig[64], uint8_t sig_v[32], uint32_t *sig_v_size)
{
    uint8_t v;
    uint8_t priv[32] = {0};

    get_private_key(priv);
    
    if(ecdsa_sign_digest(&secp256k1, priv, tx_hash, sig, &v, ethereum_is_canonic) != 0)
    {
        return false;
    }
    calculate_sigv(chain_id, sig_v, v, sig_v_size);

    return true;
}

/*
 * Given a filled ETH_TX, it builds the RLP structure, hashes it, and signs it. 
 * It rebuilds the the RLP structure, now with the signature details. This RLP structure is the raw transaction to be published.
*/
bool get_ethereum_tx(ETH_TX *tx, uint8_t *rlp_tx, uint32_t *tx_size)
{
    // TODO: validate fields/sizes
    uint8_t tx_hash[32] = {0};
    uint8_t sig[64] = {0};

    build(tx, tx_hash);
    
    if(!sign(tx_hash, tx->chain_id, sig, tx->sig_v.bytes, &(tx->sig_v.size)))
    {
        return false;
    }

    memcpy(tx->sig.bytes, sig, 64);
    tx->sig.size = 64;

    extern uint8_t encoded_tx[2048];
    extern uint32_t offset_rlp;

    uint8_t tmp[32] = {0};
    offset_rlp = 0;

    // TODO: We use this, because the hashing also builds the RLP structure. It's a hack, should be changed because it's hashing for no reason at this point.
    build(tx, tmp);
    memcpy(rlp_tx, (uint8_t*)encoded_tx, offset_rlp);
    *tx_size = offset_rlp;
    return true;
}