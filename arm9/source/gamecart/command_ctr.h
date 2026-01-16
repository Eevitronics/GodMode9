// Copyright 2014 Normmatt
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common.h"

void CTR_CmdReadSectorSD(u8* aBuffer, u32 aSector);
void CTR_CmdReadData(u32 sector, u32 length, u32 blocks, void* buffer);
void CTR_CmdReadHeader(void* buffer);
void CTR_CmdReadUniqueID(void* buffer);
u32 CTR_CmdGetSecureId(u32 rand1, u32 rand2);
void CTR_CmdSeed(u32 rand1, u32 rand2);

void CTR_CmdDF(void *buff); //dRD_PARA
void CTR_CmdCA(void *buff); //dRD_UID
void CTR_CmdC8(void *buff); //dRD_ID1
void CTR_CmdC9(void *buff); //dRD_ID2
void CTR_CmdDE(void *buff); //dWR_PARA
void CTR_CmdCF(); //dRD_ERASE
void CTR_CmdCE(void *buff); //dRD_WREXE
void CTR_CmdCB(u32 PA, void *buff);//dRD_SEQ_PAGE

void CTR_CmdCC(u32 PA, u32 PC, void *buff); //dWR_PAGE_START
void CTR_CmdCD(void *buff); //dWR_PAGE
void CTR_CmdReadDataSecure(u32 sector, u32 length, u32 blocks, void* buffer); //dRD_SEQ_PAGE