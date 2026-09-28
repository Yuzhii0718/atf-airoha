// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024 AIROHA Inc
 */

/*======================================================================================
 * MODULE NAME: spi
 * FILE NAME: parallel_nand_flash_table.c
 * VERSION: 2.00
 * PURPOSE: Parallel (raw) NAND device table.
 *
 * NOTES:
 *   Ported from the ATF 2.3 driver/mtd/chips/parallel_nand_flash_table.c.  The
 *   obsolete GNU "field:" initializer extension was replaced by C99 designated
 *   initializers, the entries themselves are unchanged.
 *
 *   The extra geometry fields (ext_id, timing_setting, min_ecc_req, addr_cycle)
 *   only exist in struct SPI_NAND_FLASH_INFO_T when TCSUPPORT_PARALLEL_NAND is
 *   defined; everything else keeps the SPI-NAND defaults (zero).
 *
 * Per-entry comment format:
 *   <vendor> | <type> | <chip name> | <ECC requirement>
 *   type is SLC for every part below; ECC is min_ecc_req in bit/512B.
 *
 *   timings:           0x44333   -> 100 MHz APB
 *                      PARALLEL_NAND_FLASH_TIMING -> CPU clock dependent value
 *   addr_cycle:        4 = 1 column + 3 row bytes, 5 = 2 column + 3 row bytes
 *======================================================================================
 */

/* INCLUDE FILE DECLARATIONS --------------------------------------------------------- */
#include <lib/utils_def.h>
#include <string.h>

#include <ecnt_spi_nand_flash.h>

#include "parallel_nand_flash.h"

#if defined(TCSUPPORT_PARALLEL_NAND)

/* STATIC VARIABLE DECLARATIONS ------------------------------------------------------ */
static const struct SPI_NAND_FLASH_INFO_T parallel_nand_flash_tables[] = {
	/* MXIC */
	/* MXIC   | SLC | MX30LF1G18AC     | 4bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_MXIC,
		.dev_id		= 0xF1,
		.ptr_name	= (const u8 *)"MX30LF1G18AC",
		.device_size	= _SPI_NAND_CHIP_SIZE_1GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_64BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x029580,
		.timing_setting	= 0x44333,
		.min_ecc_req	= 4,
		.addr_cycle	= 4,
		.soc_ecc_ability	= 4,
	},
	/* MXIC   | SLC | MX30LF2G18AC     | 4bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_MXIC,
		.dev_id		= 0xDA,
		.ptr_name	= (const u8 *)"MX30LF2G18AC",
		.device_size	= _SPI_NAND_CHIP_SIZE_2GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_64BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x069590,
		.timing_setting	= 0x44333,
		.min_ecc_req	= 4,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 4,
	},
	/* MXIC   | SLC | MX30LF1G08AA     | 1bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_MXIC,
		.dev_id		= 0xF1,
		.ptr_name	= (const u8 *)"MX30LF1G08AA",
		.device_size	= _SPI_NAND_CHIP_SIZE_1GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_64BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0xC21D80,
		.timing_setting	= PARALLEL_NAND_FLASH_TIMING,
		.min_ecc_req	= 1,
		.addr_cycle	= 4,
		.soc_ecc_ability	= 4,
	},
	/* ESMT */
	/* ESMT   | SLC | F59L2G81A        | 4bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_ESMT,
		.dev_id		= 0xDA,
		.ptr_name	= (const u8 *)"F59L2G81A",
		.device_size	= _SPI_NAND_CHIP_SIZE_2GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_64BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x449590,
		.timing_setting	= 0x44333,
		.min_ecc_req	= 4,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 4,
	},
	/* ESMT   | SLC | F59L4G81A        | 4bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_ESMT,
		.dev_id		= 0xDC,
		.ptr_name	= (const u8 *)"F59L4G81A",
		.device_size	= _SPI_NAND_CHIP_SIZE_4GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_64BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x549590,
		.timing_setting	= 0x44333,
		.min_ecc_req	= 4,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 4,
	},
	/* Winbond */
	/* Winbond| SLC | W29N08GVSIAD     | 1bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_WINBOND,
		.dev_id		= 0xDC,
		.ptr_name	= (const u8 *)"W29N08GVSIAD",
		.device_size	= _SPI_NAND_CHIP_SIZE_8GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_64BYTE,
		.feature	= PARALLEL_NAND_FLASH_2CE,
		.ext_id		= 0x549590,
		.timing_setting	= 0x44333,
		.min_ecc_req	= 1,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 4,
	},
	/* Winbond| SLC | W29N08GVSIAA     | 1bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_WINBOND,
		.dev_id		= 0xD3,
		.ptr_name	= (const u8 *)"W29N08GVSIAA",
		.device_size	= _SPI_NAND_CHIP_SIZE_8GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_64BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x589591,
		.timing_setting	= 0x44333,
		.min_ecc_req	= 1,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 4,
	},
	/* Winbond| SLC | W29N02GV         | 1bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_WINBOND,
		.dev_id		= 0xDA,
		.ptr_name	= (const u8 *)"W29N02GV",
		.device_size	= _SPI_NAND_CHIP_SIZE_2GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_64BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x049590,
		.timing_setting	= PARALLEL_NAND_FLASH_TIMING,
		.min_ecc_req	= 1,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 4,
	},
	/* Winbond| SLC | W29N02KVSIAE     | 8bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_WINBOND,
		.dev_id		= 0xDA,
		.ptr_name	= (const u8 *)"W29N02KVSIAE",
		.device_size	= _SPI_NAND_CHIP_SIZE_2GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_128BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x079510,
		.timing_setting	= PARALLEL_NAND_FLASH_TIMING,
		.min_ecc_req	= 8,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 8,
	},
	/* Winbond| SLC | W29N02KVSIAF     | 4bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_WINBOND,
		.dev_id		= 0xDA,
		.ptr_name	= (const u8 *)"W29N02KVSIAF",
		.device_size	= _SPI_NAND_CHIP_SIZE_2GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_128BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x069510,
		.timing_setting	= PARALLEL_NAND_FLASH_TIMING,
		.min_ecc_req	= 4,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 4,
	},
	/* Micron */
	/* Micron | SLC | MT29F01G08ABAEA  | 4bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_MICRON,
		.dev_id		= 0xF1,
		.ptr_name	= (const u8 *)"MT29F01G08ABAEA",
		.device_size	= _SPI_NAND_CHIP_SIZE_1GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_64BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x049580,
		.timing_setting	= 0x44333,
		.min_ecc_req	= 4,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 4,
	},
	/* Micron | SLC | MT29F02G08ABAGA  | 8bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_MICRON,
		.dev_id		= 0xDA,
		.ptr_name	= (const u8 *)"MT29F02G08ABAGA",
		.device_size	= _SPI_NAND_CHIP_SIZE_2GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_128BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x069590,
		.timing_setting	= 0x44333,
		.min_ecc_req	= 8,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 12,
	},
	/* Micron | SLC | MT29F08G08ABACA  | 8bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_MICRON,
		.dev_id		= 0xD3,
		.ptr_name	= (const u8 *)"MT29F08G08ABACA",
		.device_size	= _SPI_NAND_CHIP_SIZE_8GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_4KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_256KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_224BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x64A690,
		.timing_setting	= 0x44333,
		.min_ecc_req	= 8,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 12,
	},
	/* Micron | SLC | MT29F4G08ABAEA   | 8bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_MICRON,
		.dev_id		= 0xDC,
		.ptr_name	= (const u8 *)"MT29F4G08ABAEA",
		.device_size	= _SPI_NAND_CHIP_SIZE_4GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_4KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_256KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_224BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x54A690,
		.timing_setting	= PARALLEL_NAND_FLASH_TIMING,
		.min_ecc_req	= 8,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 12,
	},
	/* Micron | SLC | MT29F2G08ABAFA   | 8bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_MICRON,
		.dev_id		= 0xDA,
		.ptr_name	= (const u8 *)"MT29F2G08ABAFA",
		.device_size	= _SPI_NAND_CHIP_SIZE_2GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_224BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x049590,
		.timing_setting	= PARALLEL_NAND_FLASH_TIMING,
		.min_ecc_req	= 8,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 12,
	},
	/* Micron | SLC | MT29F16G08ABACA  | 8bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_MICRON,
		.dev_id		= 0x48,
		.ptr_name	= (const u8 *)"MT29F16G08ABACA",
		.device_size	= _SPI_NAND_CHIP_SIZE_16GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_4KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_512KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_224BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0xA92600,
		.timing_setting	= PARALLEL_NAND_FLASH_TIMING,
		.min_ecc_req	= 8,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 12,
	},
	/* Toshiba */
	/* Toshiba| SLC | TC58NVG4S0HTA20  | 8bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_TOSHIBA,
		.dev_id		= 0xD3,
		.ptr_name	= (const u8 *)"TC58NVG4S0HTA20",
		.device_size	= _SPI_NAND_CHIP_SIZE_16GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_4KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_256KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_256BYTE,
		.feature	= PARALLEL_NAND_FLASH_2CE,
		.ext_id		= 0x762691,
		.timing_setting	= 0x44333,
		.min_ecc_req	= 8,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 12,
	},
	/* Toshiba| SLC | TC58NVG1S3HTAI0  | 8bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_TOSHIBA,
		.dev_id		= 0xDA,
		.ptr_name	= (const u8 *)"TC58NVG1S3HTAI0",
		.device_size	= _SPI_NAND_CHIP_SIZE_2GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_128BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x761590,
		.timing_setting	= 0x44333,
		.min_ecc_req	= 8,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 8,
	},
	/* Toshiba| SLC | TC58NVG0S3HTA00  | 8bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_TOSHIBA,
		.dev_id		= 0xF1,
		.ptr_name	= (const u8 *)"TC58NVG0S3HTA00",
		.device_size	= _SPI_NAND_CHIP_SIZE_1GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_128BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x721580,
		.timing_setting	= PARALLEL_NAND_FLASH_TIMING,
		.min_ecc_req	= 8,
		.addr_cycle	= 4,
		.soc_ecc_ability	= 8,
	},
	/* Toshiba| SLC | TC58NVG3S0FTA00  | 4bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_TOSHIBA,
		.dev_id		= 0xD3,
		.ptr_name	= (const u8 *)"TC58NVG3S0FTA00",
		.device_size	= _SPI_NAND_CHIP_SIZE_8GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_4KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_256KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_232BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x762690,
		.timing_setting	= PARALLEL_NAND_FLASH_TIMING,
		.min_ecc_req	= 4,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 4,
	},
	/* Toshiba| SLC | TC58NVG2S0HTA00  | 12bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_TOSHIBA,
		.dev_id		= 0xDC,
		.ptr_name	= (const u8 *)"TC58NVG2S0HTA00",
		.device_size	= _SPI_NAND_CHIP_SIZE_4GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_4KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_256KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_256BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x762690,
		.timing_setting	= PARALLEL_NAND_FLASH_TIMING,
		.min_ecc_req	= 12,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 12,
	},
	/* Toshiba| SLC | TH58NVG2S3HTA00  | 4bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_TOSHIBA,
		.dev_id		= 0xDC,
		.ptr_name	= (const u8 *)"TH58NVG2S3HTA00",
		.device_size	= _SPI_NAND_CHIP_SIZE_4GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_128BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x761591,
		.timing_setting	= PARALLEL_NAND_FLASH_TIMING,
		.min_ecc_req	= 4,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 4,
	},
	/* Spansion */
	/* Spansion| SLC | S34ML02G300TFI00| 4bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_SPANSION,
		.dev_id		= 0xDA,
		.ptr_name	= (const u8 *)"S34ML02G300TFI00",
		.device_size	= _SPI_NAND_CHIP_SIZE_2GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_128BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x469500,
		.timing_setting	= PARALLEL_NAND_FLASH_TIMING,
		.min_ecc_req	= 4,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 4,
	},
	/* Spansion| SLC | S34ML04G200TFI00| 4bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_SPANSION,
		.dev_id		= 0xDC,
		.ptr_name	= (const u8 *)"S34ML04G200TFI00",
		.device_size	= _SPI_NAND_CHIP_SIZE_4GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_128BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x569590,
		.timing_setting	= PARALLEL_NAND_FLASH_TIMING,
		.min_ecc_req	= 4,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 4,
	},
	/* Spansion| SLC | S34ML01G1       | 1bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_SPANSION,
		.dev_id		= 0xF1,
		.ptr_name	= (const u8 *)"S34ML01G1",
		.device_size	= _SPI_NAND_CHIP_SIZE_1GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_64BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x1D00,
		.timing_setting	= PARALLEL_NAND_FLASH_TIMING,
		.min_ecc_req	= 1,
		.addr_cycle	= 4,
		.soc_ecc_ability	= 4,
	},
	/* Spansion| SLC | S34ML02G1       | 1bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_SPANSION,
		.dev_id		= 0xDA,
		.ptr_name	= (const u8 *)"S34ML02G1",
		.device_size	= _SPI_NAND_CHIP_SIZE_2GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_64BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x449590,
		.timing_setting	= PARALLEL_NAND_FLASH_TIMING,
		.min_ecc_req	= 1,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 4,
	},
	/* Spansion| SLC | S34ML04G1       | 1bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_SPANSION,
		.dev_id		= 0xDC,
		.ptr_name	= (const u8 *)"S34ML04G1",
		.device_size	= _SPI_NAND_CHIP_SIZE_4GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_64BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x549590,
		.timing_setting	= PARALLEL_NAND_FLASH_TIMING,
		.min_ecc_req	= 1,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 4,
	},
	/* Spansion| SLC | S34ML02G2_1     | 4bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_SPANSION,
		.dev_id		= 0xDA,
		.ptr_name	= (const u8 *)"S34ML02G2_1",
		.device_size	= _SPI_NAND_CHIP_SIZE_2GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_128BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x469590,
		.timing_setting	= PARALLEL_NAND_FLASH_TIMING,
		.min_ecc_req	= 4,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 4,
	},
	/* Spansion| SLC | S34ML02G2_2     | 4bit/512 */
	{
		.mfr_id		= _SPI_NAND_MANUFACTURER_ID_SPANSION,
		.dev_id		= 0xDA,
		.ptr_name	= (const u8 *)"S34ML02G2_2",
		.device_size	= _SPI_NAND_CHIP_SIZE_2GBIT,
		.page_size	= _SPI_NAND_PAGE_SIZE_2KBYTE,
		.erase_size	= _SPI_NAND_BLOCK_SIZE_128KBYTE,
		.oob_size	= _SPI_NAND_OOB_SIZE_128BYTE,
		.feature	= SPI_NAND_FLASH_FEATURE_NONE,
		.ext_id		= 0x909546,
		.timing_setting	= PARALLEL_NAND_FLASH_TIMING,
		.min_ecc_req	= 4,
		.addr_cycle	= 5,
		.soc_ecc_ability	= 4,
	},
};

/* EXPORTED SUBPROGRAM BODIES -------------------------------------------------------- */

/*------------------------------------------------------------------------------------
 * FUNCTION: SPI_NAND_FLASH_RTN_T parallel_nand_scan_flash_table(
 *                                  struct SPI_NAND_FLASH_INFO_T *ptr_rtn_device_t )
 * PURPOSE : Look up a probed device by (mfr_id, dev_id, ext_id) and copy its
 *           geometry over the probed information.
 * RETURNS : SPI_NAND_FLASH_RTN_NO_ERROR on match, SPI_NAND_FLASH_RTN_PROBE_ERROR
 *           otherwise.
 *------------------------------------------------------------------------------------
 */
SPI_NAND_FLASH_RTN_T parallel_nand_scan_flash_table(struct SPI_NAND_FLASH_INFO_T *ptr_rtn_device_t)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(parallel_nand_flash_tables); i++) {
		if ((ptr_rtn_device_t->mfr_id == parallel_nand_flash_tables[i].mfr_id) &&
		    (ptr_rtn_device_t->dev_id == parallel_nand_flash_tables[i].dev_id) &&
		    (ptr_rtn_device_t->ext_id == parallel_nand_flash_tables[i].ext_id)) {
			memcpy(ptr_rtn_device_t, &parallel_nand_flash_tables[i],
			       sizeof(struct SPI_NAND_FLASH_INFO_T));

			return SPI_NAND_FLASH_RTN_NO_ERROR;
		}
	}

	return SPI_NAND_FLASH_RTN_PROBE_ERROR;
}

#endif /* TCSUPPORT_PARALLEL_NAND */

/* End of [parallel_nand_flash_table.c] package */
