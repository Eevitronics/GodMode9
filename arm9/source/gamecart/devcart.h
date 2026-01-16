#ifndef DEVCART_H
#define DEVCART_H
#include "common.h"


//page size mask       
#define PAGESIZE_1K 0b00000011 //NOT CORRECT
#define PAGESIZE_2K 0b00000000 
#define PAGESIZE_4K 0b00000001
#define PAGESIZE_8K 0b00000010

//block size mask
#define BLOCKSIZE_64K 0b00110000 //NOT CORRECT
#define BLOCKSIZE_128K 0b00000000
#define BLOCKSIZE_256K 0b00010000
#define BLOCKSIZE_512K 0b00100000



//manufacturer codes
#define TOSHIBA 0x98
#define MACRONIX 0xEC

typedef struct s_nand
{
    uint8_t pagesize;
    uint8_t blocksize;
    uint8_t chipid;
    uint8_t maker_code;
} t_nand;

void get_pagesize(uint8_t num, t_nand *nand);
void get_blocksize(uint8_t num, t_nand *nand);
int v1_read_copts(void *COPTS_data);
int v1_write_copts(void  *COPTS_data);
int v1_erase(size_t nb_blocks);
int v1_write_data(size_t nb_pages, void *filepath, void* dec_title_key);
int v1_verify_data(size_t nb_pages, void *loadpath, void* dec_title_key);

int v2_read_copts(void *COPTS_data);
int v2_write_copts(void *COPTS_data);
int v2_erase();
int v2_write_data(u32 start_page, u32 num_pages, void *filepath, void* dec_title_key);
int v2_verify_data(size_t nb_pages, void *filepath, void* dec_title_key);

int twl_erase(size_t nb_blocks);
int twl_write_data(size_t nb_blocks, size_t page_size, size_t block_size, size_t pages_per_block, void *filepath);
int twl_verify_data(size_t nb_blocks, void *loadpath);


// Some stuff to help with Card Crypto
void rol128(uint8_t* dst, const uint8_t* src, int rbits);
void derive_title_key(uint8_t* out_key, const uint8_t* keyX, const uint8_t* keyY);
int decrypt_card_title_Key(uint8_t* seed, uint8_t* mac, uint8_t* nonce, bool is_debug_signed, uint8_t* encrypted_key, uint8_t* decrypted_key);
uint16_t crc16_modbus(const uint8_t *data, size_t length);

#endif