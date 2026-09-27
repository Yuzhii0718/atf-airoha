// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024 AIROHA Inc
 */

/*======================================================================================
 * MODULE NAME: spi
 * FILE NAME: parallel_nand_flash.h
 * VERSION: 2.00
 * PURPOSE: Public interface of the parallel (raw) NAND flash backend.
 *
 * The parallel NAND interface is driven through the same NFI block that serves
 * SPI-NAND; only the command/address/data register protocol differs (see the
 * PARALLEL_NFI_* helpers of ecnt_spi_nfi.h).  spi_nand_flash.c picks this
 * backend at probe time through the nand_probe / arht_nand_reset function
 * pointers whenever nfi_type() reports SPI_NFI_PARALLEL.
 *
 * The device table (parallel_nand_flash_table.c) is a plain read-only array:
 * unlike the SPI-NAND table it is never relocated into the BL2 optimization
 * blob, so parallel_nand_scan_flash_table() always scans the array in place.
 *======================================================================================
 */

#ifndef __PARALLEL_NAND_FLASH_H__
#define __PARALLEL_NAND_FLASH_H__

#if defined(TCSUPPORT_PARALLEL_NAND)

#include <ecnt_spi_nand_flash.h>

/* Chip selects wired on the parallel NAND interface. */
#define PARALLEL_NAND_CHIP0			(0)
#define PARALLEL_NAND_CHIP1			(1)
#define PARALLEL_NAND_MAX_CHIPS		(2)

/* Device table, implemented in parallel_nand_flash_table.c. */
SPI_NAND_FLASH_RTN_T parallel_nand_scan_flash_table(struct SPI_NAND_FLASH_INFO_T *ptr_rtn_device_t);

/* Backend entry points consumed by spi_nand_flash.c. */
SPI_NAND_FLASH_RTN_T parallel_nand_probe(struct SPI_NAND_FLASH_INFO_T *ptr_rtn_device_t);
SPI_NAND_FLASH_RTN_T parallel_nand_dma_read(u32 read_addr, u32 page_number, unsigned long *p_data);
SPI_NAND_FLASH_RTN_T parallel_nand_dma_write(u32 write_addr, u32 page_number, unsigned long *p_data, u32 oob_len, u8 *ptr_oob);
SPI_NAND_FLASH_RTN_T parallel_nand_erase(u32 page_number);
SPI_NAND_FLASH_RTN_T parallel_nand_protocol_get_status(u8 *p_status);
SPI_NAND_FLASH_RTN_T parallel_nand_protocol_reset(void);

/*------------------------------------------------------------------------------------
 * Read path latency instrumentation, off unless the build asks for it with
 *
 *   PNAND_LAT_DBG=1 SOC=an7581 ./build.sh bl2
 *
 * (same idea as LZMA_DBG).  When enabled, parallel_nand_flash.c accumulates the
 * time spent in each phase of a read and spi_nand_flash.c does the same for the
 * work it wraps around the read; the totals are dumped by bl2_boot_nand_ubi.c at
 * the end of the UBI scan.  It is pure measurement: no register value or timing
 * depends on it.  When disabled none of it is compiled in, including the
 * timestamp reads.
 *------------------------------------------------------------------------------------
 */
#if defined(PNAND_LAT_DBG)
/* Slots, in the order the dumper prints them. */
#define PNAND_LAT_SLOT_PROTO_WAIT		(0)	/* READSTART -> chip ready    */
#define PNAND_LAT_SLOT_PROTO_XFER		(1)	/* NFI DMA data phase         */
#define PNAND_LAT_SLOT_PAGE_PRE			(2)	/* spi_nand_read_page() setup */
#define PNAND_LAT_SLOT_PAGE_POST		(3)	/* ECC poll + deinterleave    */
#define PNAND_LAT_SLOT_SECT_CFG			(4)	/* first sector read setup    */
#define PNAND_LAT_SLOT_SECT_ECC			(5)	/* first sector ECC poll      */
#define PNAND_LAT_SLOT_SECT_RESTORE		(6)	/* first sector restore       */
#define PNAND_LAT_SLOT_PAGE_ECC			(7)	/* ECC poll only              */
#define PNAND_LAT_SLOT_PAGE_COPY		(8)	/* deinterleave only          */
#define PNAND_LAT_SLOT_SECT_BADPG		(9)	/* bad page hit in the sector */
#define PNAND_LAT_SLOT_SECT_OFF			(10)	/* sector path given up      */
#define PNAND_LAT_SLOTS					(11)

u32 pnand_lat_ticks_to_us(u64 ticks);
void pnand_lat_add(u32 slot, u32 us);
void pnand_lat_reset(void);
void pnand_lat_dump(const char *tag);
#endif /* PNAND_LAT_DBG */

#endif /* TCSUPPORT_PARALLEL_NAND */

#endif /* __PARALLEL_NAND_FLASH_H__ */
