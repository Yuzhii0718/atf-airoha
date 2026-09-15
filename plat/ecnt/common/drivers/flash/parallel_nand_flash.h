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

#endif /* TCSUPPORT_PARALLEL_NAND */

#endif /* __PARALLEL_NAND_FLASH_H__ */
