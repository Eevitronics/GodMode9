#include "devcart.h"
#include "command_ntr.h"
#include "command_ctr.h"
#include "protocol_ntr.h"
#include "protocol_ctr.h"
#include "ui.h"
#include "fs.h"
#include "crc32.h"
#include "micro_aes.h"
#include "keydb.h"

void get_pagesize(uint8_t num, t_nand *nand)
{   

    Debug("get_pagesize: %02X", num);

    if ((num | PAGESIZE_1K) == PAGESIZE_1K)
    {
        nand->pagesize = PAGESIZE_1K;
        Debug("1kbit page");
    }
    if ((num & PAGESIZE_2K) == PAGESIZE_2K)
    {
        nand->pagesize = PAGESIZE_2K;
        Debug("2kbit page");
    }
    if ((num & PAGESIZE_4K) == PAGESIZE_4K)
    {
        nand->pagesize = PAGESIZE_4K;
        Debug("4kbit page");
    }
    if ((num & PAGESIZE_8K) == PAGESIZE_8K)
    {
        nand->pagesize = PAGESIZE_8K;
        Debug("8kbit page");
    }
}

void get_blocksize(uint8_t num, t_nand *nand)
{
    uint8_t byte = ((num >> 4) << 4);
    Debug("get_blocksize: %02X", byte);

    if ((byte | BLOCKSIZE_64K) == BLOCKSIZE_64K)
    {
        nand->blocksize = BLOCKSIZE_64K;
        Debug("64kbit block");
    }
    if ((byte & BLOCKSIZE_128K) == BLOCKSIZE_128K)
    {
        nand->blocksize = BLOCKSIZE_128K;
        Debug("128kbit block");
    }
    if ((byte & BLOCKSIZE_256K) == BLOCKSIZE_256K)
    {
        nand->blocksize = BLOCKSIZE_256K;
        Debug("256kbit block");
    }
    if ((byte & BLOCKSIZE_512K) == BLOCKSIZE_512K)
    {
        nand->blocksize = BLOCKSIZE_512K;
        Debug("512kbit block");
    }
}

int v1_read_copts(void *COPTS_data){

    char filename[256];
    snprintf(filename, sizeof(filename), "0:/cartridge_copts.bin");
    NTR_Cmd6D(COPTS_data);
    FileSetData(filename, COPTS_data, 0x800, 0, true);

    return 1;
}

int v2_read_copts(void *COPTS_data){

    char filename[256];
    snprintf(filename, sizeof(filename), "0:/cartridge_copts.bin");
    CTR_CmdDF(COPTS_data);
    FileSetData(filename, COPTS_data, 0x200, 0, true);

    return 1;
}


int v1_write_copts(void *COPTS_data)
{
    uint8_t buffer[4];

    NTR_Cmd91(COPTS_data);
    NTR_Cmd6F(buffer);

    while (!(buffer[0] & (1 << 6))) {
        NTR_Cmd6F(buffer);
    }

    if (!(buffer[0] & (1 << 0))) //pass
    {
        return 1;
    }else{
        return 0;
    }

    return 0;
}



int v2_write_copts(void *COPTS_data)
{   
    uint8_t buffer[4];

    CTR_CmdDE(COPTS_data);
    CTR_CmdCE(buffer);
    return 1;
}



// Checks the CRC32 of the file vs the CRC of the Cartirdge. 
int v1_erase(size_t nb_blocks){
    u32 address_multiplier = 0x40;

    u32 blk_num = 0;

    ShowProgress(blk_num, nb_blocks * address_multiplier, "Erasing Cartridge");
    uint8_t buffer[4];

    int send_twice = 0;

    while (blk_num < nb_blocks * address_multiplier)
    {

        NTR_Cmd9D(blk_num);
        NTR_Cmd6F(buffer);

        while (!(buffer[0] & (1 << 6)))//busy
        {
            //poll again, change 6f to take a pointer as parameter or we will run out of mem
            NTR_Cmd6F(buffer);
        }

        // Use the bit rather than the byte
        if((buffer[0] & (1 << 0))) {
            Debug("Bad Block: %d Buffer: %02X%02X%02X%02X",(blk_num / address_multiplier), buffer[0],buffer[1],buffer[2],buffer[3] );
        }

        if(blk_num % 100 == 0){
            if(ShowProgress(blk_num, nb_blocks * address_multiplier, "Erasing Cartridge") == 0){
                return 3;
            }
        }
        send_twice++;
        if (send_twice == 2) {
            blk_num += address_multiplier;
            send_twice = 0;
        }
    }

    return 1;
}


// Checks the CRC32 of the file vs the CRC of the Cartirdge. 
int v2_erase(){

    ShowProgress(0, 1, "Erasing Cartridge");
    CTR_CmdCF();
    ShowProgress(1,1, "Erasing Cartridge");
    return 1;
}



// Checks the CRC32 of the file vs the CRC of the Cartirdge. 
int v1_verify_data(size_t nb_pages, void *filepath, void* dec_title_key){

    u32 blk_num = 0;
    u32 curr_offset = 0;
    uint8_t crc_data_buffer[0x80000];
    u32 crc32 = ~0;
    uint8_t buffer[4];
    u32 address_multiplier = 0x40;
    bool first_block = true;

    while (blk_num < nb_pages)
    {
        crc32 = ~0; 
        memset(crc_data_buffer, 0xFF, sizeof(crc_data_buffer));
        FileGetData(filepath, crc_data_buffer, 0x80000, curr_offset);

        // RE-ENABLE THIS!
        if(first_block == true) memcpy(&crc_data_buffer[0x1400], dec_title_key, 0x10);
        first_block = false;

        NTR_Cmd9B(blk_num);
        //Calculate 3DS Checksum while we wait for cartridge. 
        crc32 = crc32_calculate(crc32, crc_data_buffer, 0x80000);
        crc32 = ~crc32; 
        NTR_Cmd68(buffer);

        while (!(buffer[0] & (1 << 6))) //busy
        {
            //poll again, change 68 to take a pointer as parameter or we will run out of mem
            NTR_Cmd68(buffer);
        }
        NTR_Cmd97(buffer);
        u32 cart_crc = buffer[3] | (buffer[2] << 8) | (buffer[1] << 16) | (buffer[0] << 24);
        if(cart_crc == crc32){
            // Prints Debug if needed. 
        }else{
            Debug("Block %d CRC File Offset: 0x%08X Check *FAIL*",blk_num / address_multiplier, curr_offset);
            Debug("Cart CRC32 %8X - File CRC32 %8X", cart_crc, crc32);
            return 0;
        }

        if(blk_num % 10 == 0){
            if(ShowProgress(blk_num, nb_pages, "Verifying Data") == 0){
                return 3;
            }
        }
        blk_num += address_multiplier;
        curr_offset+=0x80000;
    }

    return 1;

}


// Checks the CRC32 of the file vs the CRC of the Cartirdge. 
int v2_verify_data(size_t nb_pages, void *filepath, void* dec_title_key){

    u32 blk_num = 0;
    u32 curr_offset = 0;
    u32 curr_address= 0;
    uint8_t crc_data_buffer[0x80000];
    uint8_t cart_crc_data_buffer[0x80000];
    u32 crc32 = ~0;
    u32 cart_crc = ~0;
    uint8_t buffer[4];
    u32 address_multiplier = 0x400;
    bool first_block = true;

    ShowProgress(blk_num, nb_pages / 0x400, "Verifying Data");

    while (blk_num < (nb_pages / 0x400))
    {
        crc32 = ~0; 
        memset(crc_data_buffer, 0xFF, sizeof(crc_data_buffer));
        FileGetData(filepath, crc_data_buffer, 0x80000, curr_offset);
        
        if(first_block == true) memcpy(&crc_data_buffer[0x1400], dec_title_key, 0x10);
        first_block = false;

        crc32 = crc32_calculate(crc32, crc_data_buffer, 0x80000);
        crc32 = ~crc32; 

        cart_crc = ~0; 
        memset(cart_crc_data_buffer, 0xFF, sizeof(cart_crc_data_buffer));
        CTR_CmdReadDataSecure(curr_address, 0x200, 0x400, cart_crc_data_buffer);
        cart_crc = crc32_calculate(cart_crc, cart_crc_data_buffer, 0x80000);
        cart_crc = ~cart_crc;

        if(cart_crc == crc32){
            
        }else{
            Debug("Block %d CRC File Offset: 0x%08X Check *FAIL*",blk_num, curr_offset);
            Debug("Cart CRC32 %08X - File CRC32 %08X", cart_crc, crc32);
            return 0;
        }

        if(ShowProgress(blk_num, nb_pages / 0x400, "Verifying Data") == 0){
            return 3;
        }

        blk_num += 1;
        curr_offset+=0x80000;
        curr_address+=0x400;

    }

    return 1;

}



int v1_write_data(size_t nb_pages, void *filepath, void* dec_title_key){
    u32 curr_page = 0;
    u32 curr_address = 0;

    u32 curr_offset = 0;
    u32 cmd_dummy[2] = {0x02000000, 0x00000000};
    uint8_t status_buff[4];

    ShowProgress(curr_page, nb_pages, "Writing Data to Cartridge");

    bool first_block = true;

    int bytes_per_SD_read =  0x80000;

    uint8_t block_buffer[bytes_per_SD_read];  // 512 KiB temp buffer (Minimise reading from SD)
    u32 pages_loaded = 0;

    while(curr_page < nb_pages) {
        if (pages_loaded == 0) {
            u32 cmd[2] = {0x93000000, 0x00000000};
            cmd[0] = cmd[0] + curr_address;
            // Debug("Block: %lu  Command %8X", curr_page / 64, cmd[0]);
            // Debug("          Offset %8X", curr_offset);

            memset(block_buffer, 0xFF, sizeof(block_buffer));
            FileGetData(filepath, block_buffer, bytes_per_SD_read, curr_offset);

            if(first_block == true) memcpy(&block_buffer[0x1400], dec_title_key, 0x10);
            first_block = false;
        }

        for(int i = 0; i < 16; i++) {
            u8* chunk = &block_buffer[(pages_loaded * 0x2000) + (i * 0x200)];

            if (i == 0) {
                NTR_Cmd92(curr_address, chunk);
            }else {
                NTR_SendCommandWrite(cmd_dummy, 0x200, 0x100, chunk);
            }
            curr_offset += 0x200;
        }

        NTR_Cmd6F(status_buff);
        while (!(status_buff[0] & (1 << 6))) //busy
        {
            //poll again, change 6f to take a pointer as parameter or we will run out of mem
            NTR_Cmd6F(status_buff);
        }

        if (!(status_buff[0] & (0 << 0))) //pass
        {   

        }else{
            Debug("Critical Error Writing Data!");
            return 0;
        }

        curr_address += 0x4;
        curr_page += 1;
        pages_loaded++;

        if (pages_loaded >= bytes_per_SD_read / 0x2000)
            pages_loaded = 0;

        if(curr_page % 10 == 0){
            if(ShowProgress(curr_page, nb_pages, "Writing Data to Cartridge") == 0){
                return 3;
            }
        }

    }
    return 1;
}

// Checks the CRC32 of the file vs the CRC of the Cartirdge. 
int twl_erase(size_t nb_blocks){
    u32 address_multiplier = 0x40;

    u32 blk_num = 0;

    ShowProgress(blk_num, nb_blocks * address_multiplier, "Erasing Cartridge");
    uint8_t buffer[4];

    int send_twice = 0;

    while (blk_num < nb_blocks * address_multiplier)
    {

        NTR_Cmd9D(blk_num);
        NTR_Cmd6F(buffer);

        while (!(buffer[0] & (1 << 6)))//busy
        {
            //poll again, change 6f to take a pointer as parameter or we will run out of mem
            NTR_Cmd6F(buffer);
        }

        // Use the bit rather than the byte
        if((buffer[0] & (1 << 0))) {
            Debug("Bad Block: %d Buffer: %02X%02X%02X%02X",(blk_num / address_multiplier), buffer[0],buffer[1],buffer[2],buffer[3] );
        }

        if(blk_num % 100 == 0){
            if(ShowProgress(blk_num, nb_blocks * address_multiplier, "Erasing Cartridge") == 0){
                return 3;
            }
        }
        send_twice++;
        if (send_twice == 2) {
            blk_num += address_multiplier;
            send_twice = 0;
        }
    }

    return 1;
}



int twl_write_data(size_t nb_blocks, size_t page_size, size_t block_size, size_t pages_per_block, void *filepath){
    u32 curr_page = 0;
    u32 curr_address = 0;

    u32 curr_offset = 0;
    u32 cmd_dummy[2] = {0x02000000, 0x00000000};


    uint8_t status_buff[4];

    ShowProgress(curr_page, nb_blocks*pages_per_block, "Writing Data to Cartridge");

    int bytes_per_SD_read =  0x80000;
    int bytes_to_read = bytes_per_SD_read;
    uint8_t block_buffer[bytes_per_SD_read];  // 512 KiB temp buffer (Minimise reading from SD)
    u32 pages_loaded = 0;

    size_t fsize = FileGetSize(filepath);

    while(curr_page < nb_blocks*pages_per_block) {
 
        if (pages_loaded == 0) {
            
            printf("Block: %lu  \n", curr_page);
            // Debug("          Offset %8X", curr_offset);

            memset(block_buffer, 0xFF, sizeof(block_buffer));
            if(curr_offset + bytes_per_SD_read > fsize){
                bytes_to_read = fsize;
            }else{
                bytes_to_read = bytes_per_SD_read;
            }
            FileGetData(filepath, block_buffer, bytes_to_read, curr_offset);
            // FileGetData(filepath, block_buffer, bytes_per_SD_read, curr_offset);

            // if(first_block == true) memcpy(&block_buffer[0x1400], dec_title_key, 0x10);
            // first_block = false;
        }

        for(int i = 0; i < 4; i++) {
            u8* chunk = &block_buffer[(pages_loaded * page_size) + (i * page_size/4)];
            NTR_Cmd92(curr_address, chunk);
            curr_offset += page_size/4;
        }


        NTR_Cmd6F(status_buff);
        while (!(status_buff[0] & (1 << 6))) //busy
        {
            //poll again, change 6f to take a pointer as parameter or we will run out of mem
            NTR_Cmd6F(status_buff);
        }

        if ((status_buff[0] & 0x01) == 0) //pass
        {   

        }else{
            Debug("Critical Error Writing Data!");
            return 0;
        }

        curr_address += 1;
        curr_page += 1;
        pages_loaded++;

        if (pages_loaded >= bytes_per_SD_read / block_size)
            pages_loaded = 0;

        if(curr_page % 10 == 0){
            if(ShowProgress(curr_page, nb_blocks*pages_per_block, "Writing Data to Cartridge") == 0){
                return 3;
            }
        }


    }
    return 1;
}

// Checks the CRC32 of the file vs the CRC of the Cartirdge. 
int twl_verify_data(size_t nb_blocks, void *filepath){

    // TODO, PASS PARAMETER
    u32 block_size = 0x20000;


    u32 blk_num = 0;
    u32 curr_offset = 0;
    uint8_t crc_data_buffer[block_size];
    u32 crc32 = ~0;
    uint8_t buffer[4];
    u32 address_multiplier = 0x40;
    // bool first_block = true;

    
    while (blk_num < nb_blocks)
    {
        crc32 = ~0; 
        memset(crc_data_buffer, 0xFF, sizeof(crc_data_buffer));

        FileGetData(filepath, crc_data_buffer, block_size, curr_offset);

        // RE-ENABLE THIS!
        // if(first_block == true) memcpy(&crc_data_buffer[0x1400], dec_title_key, 0x10);
        // first_block = false;

        NTR_Cmd9B(blk_num * address_multiplier);

        //Calculate 3DS Checksum while we wait for cartridge. 
        crc32 = crc32_calculate(crc32, crc_data_buffer, block_size);
        crc32 = ~crc32; 
        NTR_Cmd6E(buffer);
        // Debug("buffer %2X", buffer[0]);


        while (buffer[0] & (1 << 7)) // while busy
        {
            //poll again, change 68 to take a pointer as parameter or we will run out of mem
            // Debug("-");
            NTR_Cmd6E(buffer); 
            // Debug("buffer %2X", buffer[0]);
        }

        NTR_Cmd97(buffer);
        u32 cart_crc = buffer[3] | (buffer[2] << 8) | (buffer[1] << 16) | (buffer[0] << 24);

        // Debug("Block %d CRC File Offset: 0x%08X Check *TEST*",blk_num / address_multiplier, curr_offset);
        // Debug("Cart CRC32 %8X - File CRC32 %8X", cart_crc, crc32);

        if(cart_crc == crc32){
            // Prints Debug if needed. 
        }else{
            Debug("Block %d CRC File Offset: 0x%08X Check *FAIL*",blk_num / address_multiplier, curr_offset);
            Debug("Cart CRC32 %8X - File CRC32 %8X", cart_crc, crc32);
            return 0;
        }

        if(blk_num % 10 == 0){
            if(ShowProgress(blk_num, nb_blocks, "Verifying Data") == 0){
                return 3;
            }
        }
        blk_num += 1;
        curr_offset+=block_size;
    }

    return 1;

}


int v2_write_data(u32 start_page, u32 num_pages, void *filepath, void* dec_title_key){
    uint8_t data_buffer [0x200];
    u32 curr_page = 0;

    u32 curr_offset = 0;
    uint8_t status_buff[4];

    ShowProgress(curr_page, num_pages, "Writing Data to Cartridge");

    bool first_block = true;
    int bytes_per_SD_read =  0x400000;

    uint8_t block_buffer[bytes_per_SD_read];  // 512 KiB temp buffer (Minimise reading from SD)
    u32 pages_loaded = 0;

    while(curr_page < num_pages) {
        if (pages_loaded == 0) {
            memset(block_buffer, 0xFF, sizeof(block_buffer));
            FileGetData(filepath, block_buffer, bytes_per_SD_read, curr_offset);

            // if(first_block == true){
            //     memcpy(&block_buffer[0x1400], dec_title_key, 0x10);
            // }
        }

        u8* chunk = &block_buffer[(pages_loaded * 0x200)];
        if(first_block == true){
            CTR_CmdCC(start_page, num_pages, chunk); 
            CTR_CmdCE(status_buff);
            first_block = false;
        }else{
            CTR_CmdCD(chunk); 
            CTR_CmdCE(status_buff);
        }

        curr_offset += 0x200;
        curr_page += 1;
        pages_loaded++;

        if (status_buff[0] == 0x1) //pass
        {

            // Data is still waiting to be written. 
        }else{
            if(curr_page == num_pages){
                ShowProgress(curr_page, num_pages, "Writing Data!");
                return 1;
            }else{
                Debug("Critical Error Writing Data!");
                return 0;
            }
            break;
        }

        if (pages_loaded >= bytes_per_SD_read / 0x200)
            pages_loaded = 0;

        if(curr_page % 2000 == 0){
            if(ShowProgress(curr_page, num_pages, "Writing Data to Cartridge") == 0){
                return 3;
            }
        }


    }
    return 1;
}


void rol128(uint8_t* dst, const uint8_t* src, int rbits) {
    uint64_t hi = 0, lo = 0;

    // Load big-endian 128-bit number into two 64-bit halves
    for (int i = 0; i < 8; i++) {
        hi = (hi << 8) | src[i];
        lo = (lo << 8) | src[i + 8];
    }

    rbits %= 128;
    if (rbits == 0) {
        // No rotation needed
    } else if (rbits < 64) {
        uint64_t new_hi = (hi << rbits) | (lo >> (64 - rbits));
        uint64_t new_lo = (lo << rbits) | (hi >> (64 - rbits));
        hi = new_hi;
        lo = new_lo;
    } else {
        int r = rbits - 64;
        uint64_t new_hi = (lo << r) | (hi >> (64 - r));
        uint64_t new_lo = (hi << r) | (lo >> (64 - r));
        hi = new_hi;
        lo = new_lo;
    }

    // Store result back to dst as big-endian
    for (int i = 7; i >= 0; i--) {
        dst[i]      = hi & 0xFF;
        dst[i + 8]  = lo & 0xFF;
        hi >>= 8;
        lo >>= 8;
    }
}


void derive_title_key(uint8_t* out_key, const uint8_t* keyX, const uint8_t* keyY) {
    // Constants
    uint8_t const_val[16] = {
        0x1F, 0xF9, 0xE9, 0xAA, 0xC5, 0xFE, 0x04, 0x08,
        0x02, 0x45, 0x91, 0xDC, 0x5D, 0x52, 0x76, 0x8A
    };

    uint8_t tmp1[16], tmp2[16], tmp3[16];

    rol128(tmp1, keyX, 2);  // tmp1 = ROL(KeyX, 2)
    
    for (int i = 0; i < 16; i++) {
        printf("%02X", tmp1[i]);
    }
    printf("\n");


    for (int i = 0; i < 16; i++)
        tmp2[i] = tmp1[i] ^ keyY[i];  // tmp2 = tmp1 XOR KeyY

    uint16_t carry = 0;
    for (int i = 15; i >= 0; i--) {
        uint16_t sum = tmp2[i] + const_val[i] + carry;
        tmp3[i] = sum & 0xFF;
        carry = sum >> 8;
    }

    rol128(out_key, tmp3, 87);  // out_key = ROL(tmp3, 87)
}


int decrypt_card_title_Key(uint8_t* seed, uint8_t* mac, uint8_t* nonce, bool is_debug_signed, uint8_t* encrypted_key, uint8_t* decrypted_key){

    uint8_t zerokey [0x10] = {0};
    uint8_t derived_key[16];

    u32 keys_offset = 0;

    keys_offset += 0x8000;

    uint8_t slot0x2Ckey[0x10];
    uint8_t slot0x3Bkey[0x10];

    char *boot9path = "0:/gm9/support/boot9.bin";
    if(FileGetData(boot9path, slot0x2Ckey, sizeof(slot0x2Ckey), 0x59D0 + keys_offset)){
        FileGetData(boot9path, slot0x3Bkey, sizeof(slot0x3Bkey), 0x5A00 + keys_offset);

        u32 slot0x2Ckey_crc32 = ~0; 
        slot0x2Ckey_crc32 = crc32_calculate(slot0x2Ckey_crc32, slot0x2Ckey, sizeof(slot0x2Ckey));
        slot0x2Ckey_crc32 = ~slot0x2Ckey_crc32; 

        u32 slot0x3Bkey_crc32 = ~0; 
        slot0x3Bkey_crc32 = crc32_calculate(slot0x3Bkey_crc32, slot0x3Bkey, sizeof(slot0x3Bkey));
        slot0x3Bkey_crc32 = ~slot0x3Bkey_crc32; 

        if((slot0x2Ckey_crc32 != 0xDF067DC8) && (slot0x3Bkey_crc32 != 0x264DF6D1)){
            ShowPrompt(false, "Error: 'boot9.bin' is invalid.\nPlease place 'boot9.bin' in sd:/gm9/support/boot9.bin\n");
            return 0;
        }
    }else{
        ShowPrompt(false, "Error: 'boot9.bin' not found.\nPlease place 'boot9.bin' in sd:/gm9/support/boot9.bin\n");
        return 0;
    }

    uint8_t ciphertext_with_tag[32];
    memcpy(ciphertext_with_tag, encrypted_key, 16);
    memcpy(ciphertext_with_tag + 16, mac, 16);


    if(is_debug_signed){
        memcpy(derived_key, zerokey, 16);
    } else{
        derive_title_key(derived_key, slot0x3Bkey, seed);
    }

    int result = AES_CCM_decrypt(
        derived_key,
        nonce,
        ciphertext_with_tag,
        16,      // ciphertext + tag
        NULL, 0, // no associated data
        16,      // tag length
        decrypted_key
    );

    if (result == 0) {
        return 1;
    } else {
        return 0;
    }

    return 0;
}

uint16_t crc16_modbus(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }

    return crc;
}

