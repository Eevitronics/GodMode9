// Copyright 2014 Normmatt
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "command_ctr.h"

#include "protocol_ctr.h"

static int read_count = 0;

static void CTR_CmdC5()
{
    static const u32 c5_cmd[4] = { 0xC5000000, 0x00000000, 0x00000000, 0x00000000 };
    CTR_SendCommand(c5_cmd, 0, 1, 0x100002C, NULL);
}

void CTR_CmdReadData(u32 sector, u32 length, u32 blocks, void* buffer)
{
    if(read_count++ > 10000)
    {
        CTR_CmdC5();
        read_count = 0;
    }

    const u32 read_cmd[4] = {
        (0xBF000000 | (u32)(sector >> 23)),
        (u32)((sector << 9) & 0xFFFFFFFF),
        0x00000000, 0x00000000
    };
    CTR_SendCommand(read_cmd, length, blocks, 0x104822C, buffer); // Clock divider 5 (13.4 MHz). Same as Process9.
}

void CTR_CmdReadHeader(void* buffer)
{
    static const u32 readheader_cmd[4] = { 0x82000000, 0x00000000, 0x00000000, 0x00000000 };
    CTR_SendCommand(readheader_cmd, 0x200, 1, 0x704802C, buffer);
}

void CTR_CmdReadUniqueID(void* buffer)
{
    static const u32 readheader_cmd[4] = { 0xC6000000, 0x00000000, 0x00000000, 0x00000000 };
    CTR_SendCommand(readheader_cmd, 0x40, 1, 0x903002C, buffer);
}

u32 CTR_CmdGetSecureId(u32 rand1, u32 rand2)
{
    u32 id = 0;
    const u32 getid_cmd[4] = { 0xA2000000, 0x00000000, rand1, rand2 };
    CTR_SendCommand(getid_cmd, 0x4, 1, 0x701002C, &id);
    return id;
}

void CTR_CmdSeed(u32 rand1, u32 rand2)
{
    const u32 seed_cmd[4] = { 0x83000000, 0x00000000, rand1, rand2 };
    CTR_SendCommand(seed_cmd, 0, 1, 0x700822C, NULL);
}

void CTR_CmdDF(void *buff) { //dRD_PARA
    u32 cmd16[4] = {0xDF000000, 0x00000000, 0x00000000, 0x00000000};
    CTR_SendCommand(cmd16, 0x200, 1, 0x700822C, buff);
}

void CTR_CmdCA(void *buff) { //dRD_UID
    u32 cmd16[4] = {0xCA000000, 0x00000000, 0x00000000, 0x00000000};
    CTR_SendCommand(cmd16, 0x4, 1, 0x700822C, buff);
}

void CTR_CmdC8(void *buff) //dRD_ID1
{ 
    u32 cmd16[4] = {0xC8000000, 0x00000000, 0x00000000, 0x00000000};
    CTR_SendCommand(cmd16, 0x4, 1, 0x700822C, buff);
}

void CTR_CmdC9(void *buff) //dRD_ID2
{ 
    u32 cmd16[4] = {0xC9000000, 0x00000000, 0x00000000, 0x00000000};
    CTR_SendCommand(cmd16, 0x4, 1, 0x700822C, buff);
}

void CTR_CmdDE(void *buff) //dWR_PARA
{ 
    u32 cmd16[4] = {0xDE000000, 0x00000000, 0x00000000, 0x00000000};
    CTR_SendCommandWrite_CmdCE(cmd16, 512, 1, 200, 0x700822C, buff);
}

void CTR_CmdCF() //dRD_ERASE
{ 
    u32 cmd16[4] = {0xCF000000, 0x00000000, 0x00000000, 0x00000000};
    CTR_SendCommand(cmd16, 0x4, 1, 0x700822C, NULL);
}

void CTR_CmdCE(void *buff) //dRD_WREXE
{ 
    u32 cmd16[4] = {0xCE000000, 0x00000000, 0x00000000, 0x00000000};
    CTR_SendCommand(cmd16, 0x4, 1, 0x700822C, buff);
}

void CTR_CmdCC(uint32_t PA, uint32_t PC, void *buff) //dWR_PAGE_START
{   

    const u32 cmd16[4] = {
        (0xCC000000 | (u32)(PA >> 23)),
        (u32)((PA << 9) & 0xFFFFFFFF),
        0x00000000, 
        (u32)((PC) & 0xFFFFFFFF)
    };

    CTR_SendCommandWrite_CmdCE(cmd16, 512, 1, 200, 0x700822C, buff);
}

void CTR_CmdCD(void *buff) //dWR_PAGE
{ 
    u32 cmd16[4] = {0xCD000000, 0x00000000, 0x00000000, 0x00000000};
    CTR_SendCommandWrite_CmdCE(cmd16, 512, 1, 20, 0x700822C, buff);
}


void CTR_CmdReadDataSecure(u32 sector, u32 length, u32 blocks, void* buffer)
{
    const u32 read_cmd[4] = {
        (0xCB000000 | (u32)(sector >> 23)),
        (u32)((sector << 9) & 0xFFFFFFFF),
        0x00000000, 0x00000000
    };
    CTR_SendCommand(read_cmd, length, blocks, 0x104822C, buffer); // Clock divider 5 (13.4 MHz). Same as Process9.
}