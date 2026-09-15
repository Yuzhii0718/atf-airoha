// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024 AIROHA Inc
 */

/*======================================================================================
 * MODULE NAME: spi
 * FILE NAME: parallel_nand_flash.c
 * VERSION: 2.00
 * PURPOSE: Parallel (raw) NAND flash backend for the EcoNet NFI controller.
 *
 * NOTES:
 *   Ported from the ATF 2.3 driver/mtd/chips/parallel_nand_flash.c and adapted
 *   to the ATF 2.10 flash HAL:
 *     - TF-A logging instead of the kernel/bootloader print shims, so the
 *       LINUX_VERSION_CODE / SPRAM_IMG / BOOTROM_EXT print variants and the
 *       two "#if 0" leftovers are gone,
 *     - the LZMA_IMG / TCSUPPORT_BB_256KB guards around the program and erase
 *       paths are KEPT: they have to stay in sync with the very same guards
 *       around the call sites in spi_nand_flash.c,
 *     - every helper except the entry points consumed by spi_nand_flash.c is
 *       static, the prototypes live in parallel_nand_flash.h,
 *     - a probe failure reports an error instead of spinning forever.
 *
 *   The register flows (command / address / data phase, chip select, timing and
 *   the ECC block configuration of the write path) are unchanged.
 *======================================================================================
 */

/* INCLUDE FILE DECLARATIONS --------------------------------------------------------- */
#include <common/debug.h>
#include <string.h>

#include <asm/tc3162.h>			/* isFPGA() */

#include <ecnt_spi_nand_flash.h>
#include <ecnt_spi_nfi.h>
#include <ecnt_spi_ecc.h>

#include "parallel_nand_flash.h"

#if defined(TCSUPPORT_PARALLEL_NAND)

/* NAMING CONSTANT DECLARATIONS ------------------------------------------------------ */
#define PARALLEL_NAND_ID_LEN			(5)
#define PARALLEL_NAND_STATUS_LEN		(1)
#define PARALLEL_NAND_COL_NOB			(2)
#define PARALLEL_NAND_ROW_NOB			(ptr_dev_info_t->addr_cycle - 2)
#define PARALLEL_NAND_STATUS_FAIL_BIT	(0x01)

/* STATIC VARIABLE DECLARATIONS ------------------------------------------------------ */
extern struct SPI_NAND_FLASH_INFO_T _current_flash_info_t;

static struct SPI_NAND_FLASH_INFO_T *ptr_dev_info_t = &_current_flash_info_t;

/* LOCAL SUBPROGRAM BODIES------------------------------------------------------------ */

/*------------------------------------------------------------------------------------
 * FUNCTION: static void parallel_nand_chip_select( u8 chip )
 * PURPOSE : Drive the chip select line of the parallel NAND interface.
 *------------------------------------------------------------------------------------
 */
static void parallel_nand_chip_select(u8 chip)
{
	PARALLEL_NFI_SET_CHIP_SELECT(chip);

	VERBOSE("parallel_nand: chip_select=0x%x\n", chip);
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static void parallel_nand_chip_select_by_page( u32 *p_page_number )
 * PURPOSE : Select the chip owning the page and rebase the page number on it.
 *           Only needed for devices stacking two dies behind one interface.
 *------------------------------------------------------------------------------------
 */
static void parallel_nand_chip_select_by_page(u32 *p_page_number)
{
	u8 chip = PARALLEL_NAND_CHIP0;

	if ((ptr_dev_info_t->feature & PARALLEL_NAND_FLASH_2CE) != 0U) {
		u32 pages_per_chip = (ptr_dev_info_t->device_size / ptr_dev_info_t->page_size) / 2;

		if (*p_page_number >= pages_per_chip) {
			*p_page_number -= pages_per_chip;
			chip = PARALLEL_NAND_CHIP1;
		}
	}

	parallel_nand_chip_select(chip);
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static void parallel_nand_setting_timing( void )
 * PURPOSE : Apply the AC timing of the detected device.
 *------------------------------------------------------------------------------------
 */
static void parallel_nand_setting_timing(void)
{
	PARALLEL_NFI_SET_TIMING(ptr_dev_info_t->timing_setting);
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static void parallel_nand_initial_hw( void )
 * PURPOSE : Reset the NFI block and bring up its parallel NAND mode.
 * NOTES   : The pin mux of the parallel NAND interface is expected to be set up
 *           by the stage that handed over to BL2 (hw trap / pin mux tables).
 *------------------------------------------------------------------------------------
 */
static void parallel_nand_initial_hw(void)
{
	SPI_NFI_Reset();

	/* Enable NFI All Interrupt */
	PARALLEL_NFI_INIT();
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T parallel_nand_transfer_data(
 *                                  unsigned long *p_data, u8 dirt )
 * PURPOSE : Move one page between the NAND data register and the DMA buffer.
 *------------------------------------------------------------------------------------
 */
static SPI_NAND_FLASH_RTN_T parallel_nand_transfer_data(unsigned long *p_data, u8 dirt)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	SPI_NFI_RTN_T			status = SPI_NFI_RTN_NO_ERROR;

	status = PARALLEL_NFI_START_DMA(p_data, dirt);
	if (status != SPI_NFI_RTN_NO_ERROR) {
		rtn_status = SPI_NAND_FLASH_RTN_NFI_FAIL;
		ERROR("parallel_nand: %s data failed\n",
		      ((dirt == SPI_NFI_WRITE_DATA) ? "write" : "read"));
	}

	return rtn_status;
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T parallel_nand_sigle_read( u8 len, u8 *p_data )
 * PURPOSE : PIO read of a few bytes (status / device id).
 *------------------------------------------------------------------------------------
 */
static SPI_NAND_FLASH_RTN_T parallel_nand_sigle_read(u8 len, u8 *p_data)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	SPI_NFI_RTN_T			status = SPI_NFI_RTN_NO_ERROR;

	status = PARALLEL_NFI_START_BYTE_READ(len, p_data);
	if (status != SPI_NFI_RTN_NO_ERROR) {
		rtn_status = SPI_NAND_FLASH_RTN_NFI_FAIL;
		ERROR("parallel_nand: single read failed, len=0x%x\n", len);
	}

	return rtn_status;
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T parallel_nand_send_address(
 *                                  u32 col_addr, u32 row_addr, u8 col_nob, u8 row_nob )
 * PURPOSE : Issue the column / row address cycles.
 *------------------------------------------------------------------------------------
 */
static SPI_NAND_FLASH_RTN_T parallel_nand_send_address(u32 col_addr, u32 row_addr, u8 col_nob, u8 row_nob)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	SPI_NFI_RTN_T			status = SPI_NFI_RTN_NO_ERROR;

	status = PARALLEL_NFI_ISSUE_ADDR(col_addr, row_addr, col_nob, row_nob);
	if (status != SPI_NFI_RTN_NO_ERROR) {
		if (status == SPI_NFI_RTN_ACCESS_LOCK) {
			rtn_status = SPI_NAND_FLASH_RTN_CMD_ABORT;
		} else {
			rtn_status = SPI_NAND_FLASH_RTN_NFI_FAIL;
			ERROR("parallel_nand: send address failed, col_addr=0x%x row_addr=0x%x col_nob=0x%x row_nob=0x%x\n",
			      col_addr, row_addr, col_nob, row_nob);
		}
	}

	VERBOSE("parallel_nand: send address, col_addr=0x%x row_addr=0x%x col_nob=0x%x row_nob=0x%x\n",
		col_addr, row_addr, col_nob, row_nob);

	return rtn_status;
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T parallel_nand_send_cmd( u8 cmd )
 * PURPOSE : Issue a NAND command, either as a standalone first cycle (command 1)
 *           or as the second cycle of a two-command sequence (command 2).
 *------------------------------------------------------------------------------------
 */
static SPI_NAND_FLASH_RTN_T parallel_nand_send_cmd(u8 cmd)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	SPI_NFI_RTN_T			status = SPI_NFI_RTN_NO_ERROR;

	switch (cmd) {
	case NAND_CMD_READID:
	case NAND_CMD_STATUS:
	case NAND_CMD_ERASE1:
		PARALLEL_NFI_RESET();
		/* fallthrough */
	case NAND_CMD_READ:
	case NAND_CMD_SEQIN:
		PARALLEL_NFI_ISSUE_CMD_1(cmd);
		break;
	case NAND_CMD_RESET:
	case NAND_CMD_ERASE2:
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_READSTART:
		status = PARALLEL_NFI_ISSUE_CMD_2(cmd);
		break;
	default:
		ERROR("parallel_nand: unknown command 0x%x\n", cmd);
		break;
	}

	if (status != SPI_NFI_RTN_NO_ERROR) {
		rtn_status = SPI_NAND_FLASH_RTN_NFI_FAIL;
		ERROR("parallel_nand: send command failed, cmd=0x%x\n", cmd);
	}

	VERBOSE("parallel_nand: send command, cmd=0x%x\n", cmd);

	return rtn_status;
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T parallel_nand_protocol_read(
 *                                  u32 col_addr, u32 row_addr, unsigned long *p_data )
 * PURPOSE : READ (0x00) -> address -> READ START (0x30) -> data phase.
 *------------------------------------------------------------------------------------
 */
static SPI_NAND_FLASH_RTN_T parallel_nand_protocol_read(u32 col_addr, u32 row_addr, unsigned long *p_data)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	rtn_status = parallel_nand_send_cmd(NAND_CMD_READ);
	if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
		rtn_status = parallel_nand_send_address(col_addr, row_addr, PARALLEL_NAND_COL_NOB, PARALLEL_NAND_ROW_NOB);
		if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
			rtn_status = parallel_nand_send_cmd(NAND_CMD_READSTART);
			if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
				rtn_status = parallel_nand_transfer_data(p_data, SPI_NFI_READ_DATA);
			}
		}
	}

	VERBOSE("parallel_nand: protocol_read, col_addr=0x%x row_addr=0x%x\n", col_addr, row_addr);

	return rtn_status;
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T parallel_nand_protocol_program(
 *                                  u32 col_addr, u32 row_addr, unsigned long *p_data )
 * PURPOSE : SEQIN (0x80) -> address -> data phase -> PAGE PROGRAM (0x10).
 *------------------------------------------------------------------------------------
 */
#if !defined(LZMA_IMG) || defined(TCSUPPORT_BB_256KB)
static SPI_NAND_FLASH_RTN_T parallel_nand_protocol_program(u32 col_addr, u32 row_addr, unsigned long *p_data)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	rtn_status = parallel_nand_send_cmd(NAND_CMD_SEQIN);
	if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
		rtn_status = parallel_nand_send_address(col_addr, row_addr, PARALLEL_NAND_COL_NOB, PARALLEL_NAND_ROW_NOB);
		if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
			rtn_status = parallel_nand_transfer_data(p_data, SPI_NFI_WRITE_DATA);
			if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
				rtn_status = parallel_nand_send_cmd(NAND_CMD_PAGEPROG);
			}
		}
	}

	VERBOSE("parallel_nand: protocol_program, col_addr=0x%x row_addr=0x%x\n", col_addr, row_addr);

	return rtn_status;
}

/*------------------------------------------------------------------------------------
 * FUNCTION: SPI_NAND_FLASH_RTN_T parallel_nand_protocol_get_status( u8 *p_status )
 * PURPOSE : STATUS (0x70) followed by a one byte PIO read.
 *------------------------------------------------------------------------------------
 */
SPI_NAND_FLASH_RTN_T parallel_nand_protocol_get_status(u8 *p_status)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	u8						buf[8] = { 0 };

	rtn_status = parallel_nand_send_cmd(NAND_CMD_STATUS);
	if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
		rtn_status = parallel_nand_sigle_read(PARALLEL_NAND_STATUS_LEN, buf);
		if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
			memcpy(p_status, &(buf[0]), sizeof(u8));
		}
	}

	VERBOSE("parallel_nand: protocol_get_status, status=0x%x\n", *p_status);

	return rtn_status;
}

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T parallel_nand_protocol_erase( u32 row_addr )
 * PURPOSE : ERASE1 (0x60) -> row address -> ERASE2 (0xD0).
 *------------------------------------------------------------------------------------
 */
static SPI_NAND_FLASH_RTN_T parallel_nand_protocol_erase(u32 row_addr)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	rtn_status = parallel_nand_send_cmd(NAND_CMD_ERASE1);
	if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
		rtn_status = parallel_nand_send_address(0, row_addr, 0, PARALLEL_NAND_ROW_NOB);
		if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
			rtn_status = parallel_nand_send_cmd(NAND_CMD_ERASE2);
		}
	}

	VERBOSE("parallel_nand: protocol_erase, row_addr=0x%x\n", row_addr);

	return rtn_status;
}
#endif /* !defined(LZMA_IMG) || defined(TCSUPPORT_BB_256KB) */

/*------------------------------------------------------------------------------------
 * FUNCTION: static SPI_NAND_FLASH_RTN_T parallel_nand_protocol_read_id(
 *                                  struct SPI_NAND_FLASH_INFO_T *ptr_rtn_flash_id )
 * PURPOSE : READ ID (0x90) -> 1 address cycle -> 5 id bytes.
 *------------------------------------------------------------------------------------
 */
static SPI_NAND_FLASH_RTN_T parallel_nand_protocol_read_id(struct SPI_NAND_FLASH_INFO_T *ptr_rtn_flash_id)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	u8						buf[8] = { 0 };
	u32						ext_id = 0;

	rtn_status = parallel_nand_send_cmd(NAND_CMD_READID);
	if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
		rtn_status = parallel_nand_send_address(0, 0, 1, 0);
		if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
			rtn_status = parallel_nand_sigle_read(PARALLEL_NAND_ID_LEN, buf);
			if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
				memcpy((void *)&(ptr_rtn_flash_id->mfr_id), (void *)&(buf[0]), sizeof(u8));
				memcpy((void *)&(ptr_rtn_flash_id->dev_id), (void *)&(buf[1]), sizeof(u8));

				ext_id = buf[2] | (buf[3] << 8) | (buf[4] << 16);
				memcpy((void *)&(ptr_rtn_flash_id->ext_id), (void *)&ext_id, sizeof(u32));
			}
		}
	}

	VERBOSE("parallel_nand: protocol_read_id, mfr_id=0x%x dev_id=0x%x ext_id=0x%x\n",
		ptr_rtn_flash_id->mfr_id, ptr_rtn_flash_id->dev_id, ptr_rtn_flash_id->ext_id);

	return rtn_status;
}

/* EXPORTED SUBPROGRAM BODIES -------------------------------------------------------- */

/*------------------------------------------------------------------------------------
 * FUNCTION: SPI_NAND_FLASH_RTN_T parallel_nand_protocol_reset( void )
 * PURPOSE : RESET (0xFF) the currently selected device.
 *------------------------------------------------------------------------------------
 */
SPI_NAND_FLASH_RTN_T parallel_nand_protocol_reset(void)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;

	rtn_status = parallel_nand_send_cmd(NAND_CMD_RESET);

	VERBOSE("parallel_nand: protocol_reset\n");

	return rtn_status;
}

/*------------------------------------------------------------------------------------
 * FUNCTION: SPI_NAND_FLASH_RTN_T parallel_nand_probe(
 *                                  struct SPI_NAND_FLASH_INFO_T *ptr_rtn_device_t )
 * PURPOSE : Probe both chip selects, identify the device and fill in its
 *           geometry from the parallel NAND device table.
 *------------------------------------------------------------------------------------
 */
SPI_NAND_FLASH_RTN_T parallel_nand_probe(struct SPI_NAND_FLASH_INFO_T *ptr_rtn_device_t)
{
	SPI_NAND_FLASH_RTN_T			rtn_status = SPI_NAND_FLASH_RTN_PROBE_ERROR;
	struct SPI_NAND_FLASH_INFO_T	parallel_nand_dev_info[PARALLEL_NAND_MAX_CHIPS];
	u8								chip = 0;

	VERBOSE("parallel_nand: probe start\n");

	memset(parallel_nand_dev_info, 0, sizeof(parallel_nand_dev_info));

	parallel_nand_initial_hw();

	for (chip = 0; chip < PARALLEL_NAND_MAX_CHIPS; chip++) {
		parallel_nand_chip_select(chip);
		parallel_nand_protocol_reset();
		(void)parallel_nand_protocol_read_id(&parallel_nand_dev_info[chip]);

		if ((parallel_nand_dev_info[chip].mfr_id != 0x0) &&
		    (parallel_nand_dev_info[chip].dev_id != 0x0) &&
		    (parallel_nand_dev_info[chip].ext_id != 0x0)) {
			VERBOSE("parallel_nand: probe, mfr_id=0x%x dev_id=0x%x ext_id=0x%x\n",
				parallel_nand_dev_info[chip].mfr_id,
				parallel_nand_dev_info[chip].dev_id,
				parallel_nand_dev_info[chip].ext_id);
			break;
		}
	}

	if (chip >= PARALLEL_NAND_MAX_CHIPS) {
		ERROR("parallel_nand: no device on chip select 0x0~0x%x\n", PARALLEL_NAND_MAX_CHIPS - 1);
		return SPI_NAND_FLASH_RTN_PROBE_ERROR;
	}

	memcpy(ptr_rtn_device_t, &parallel_nand_dev_info[chip], sizeof(struct SPI_NAND_FLASH_INFO_T));

	rtn_status = parallel_nand_scan_flash_table(ptr_rtn_device_t);
	if (rtn_status != SPI_NAND_FLASH_RTN_NO_ERROR) {
		ERROR("parallel_nand: unknown device, mfr_id=0x%x dev_id=0x%x ext_id=0x%x\n",
		      ptr_rtn_device_t->mfr_id, ptr_rtn_device_t->dev_id, ptr_rtn_device_t->ext_id);
		return rtn_status;
	}

	INFO("parallel_nand: %s size=0x%x page=0x%x erase=0x%x oob=0x%x feature=0x%x\n",
	     (const char *)ptr_rtn_device_t->ptr_name, ptr_rtn_device_t->device_size,
	     ptr_rtn_device_t->page_size, ptr_rtn_device_t->erase_size,
	     ptr_rtn_device_t->oob_size, ptr_rtn_device_t->feature);

	/* The device must not require more correction bits per sector than the
	 * ECC block is able to provide. */
	if (ptr_rtn_device_t->soc_ecc_ability < ptr_rtn_device_t->min_ecc_req) {
		ERROR("parallel_nand: ECC ability too low, required=0x%x available=0x%x\n",
		      ptr_rtn_device_t->min_ecc_req, ptr_rtn_device_t->soc_ecc_ability);
		return SPI_NAND_FLASH_RTN_PROBE_ERROR;
	}

	if (!isFPGA) {
		/* EN7581 has to keep the _PARALLEL_NFI_REGS_ACCCON hardware default,
		 * and an FPGA has no calibrated timing to program. */
#if !defined(TCSUPPORT_CPU_EN7581)
		parallel_nand_setting_timing();
#endif
	}

	VERBOSE("parallel_nand: probe end\n");

	return rtn_status;
}

/*------------------------------------------------------------------------------------
 * FUNCTION: SPI_NAND_FLASH_RTN_T parallel_nand_dma_read(
 *                                  u32 read_addr, u32 page_number, unsigned long *p_data )
 * PURPOSE : Read one page of the addressed device.
 *------------------------------------------------------------------------------------
 */
SPI_NAND_FLASH_RTN_T parallel_nand_dma_read(u32 read_addr, u32 page_number, unsigned long *p_data)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_PROBE_ERROR;

	parallel_nand_chip_select_by_page(&page_number);

	rtn_status = parallel_nand_protocol_read(read_addr, page_number, p_data);

	return rtn_status;
}

#if !defined(LZMA_IMG) || defined(TCSUPPORT_BB_256KB)
/*------------------------------------------------------------------------------------
 * FUNCTION: SPI_NAND_FLASH_RTN_T parallel_nand_dma_write(
 *                                  u32 write_addr, u32 page_number, unsigned long *p_data,
 *                                  u32 oob_len, u8 *ptr_oob )
 * PURPOSE : Program one page of the addressed device.
 * NOTES   : The raw NAND device carries no on-die ECC, so the controller ECC
 *           block has to encode the spare area as well: the encode block size
 *           becomes (fdm_ecc_num + 512) and the auto FDM path feeds the caller
 *           provided OOB bytes into the FDM registers.
 *------------------------------------------------------------------------------------
 */
SPI_NAND_FLASH_RTN_T parallel_nand_dma_write(u32 write_addr, u32 page_number, unsigned long *p_data, u32 oob_len, u8 *ptr_oob)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	u8						status = 0;
	SPI_NFI_CONF_T			spi_nfi_conf_t;
	SPI_ECC_ENCODE_CONF_T	spi_ecc_encode_conf_t;

	SPI_NFI_Reset();

	SPI_NFI_Get_Configure(&spi_nfi_conf_t);
	SPI_NFI_Set_Configure(&spi_nfi_conf_t);

	SPI_ECC_Encode_Get_Configure(&spi_ecc_encode_conf_t);
	SPI_ECC_Encode_Set_Configure(&spi_ecc_encode_conf_t);

	if (spi_nfi_conf_t.hw_ecc_t == SPI_NFI_CON_HW_ECC_Enable) {
		SPI_ECC_Encode_Get_Configure(&spi_ecc_encode_conf_t);

		if (spi_ecc_encode_conf_t.encode_en == SPI_ECC_ENCODE_ENABLE) {
			spi_ecc_encode_conf_t.encode_block_size = (spi_nfi_conf_t.fdm_ecc_num + 512);
			SPI_ECC_Encode_Set_Configure(&spi_ecc_encode_conf_t);

			SPI_ECC_Encode_Disable();
			SPI_ECC_Decode_Disable();
			SPI_ECC_Encode_Enable();
		}
	}

	if (spi_nfi_conf_t.auto_fdm_t == SPI_NFI_CON_AUTO_FDM_Enable) {
		SPI_NFI_Write_SPI_NAND_FDM(ptr_oob, oob_len);
	}

	parallel_nand_chip_select_by_page(&page_number);

	rtn_status = parallel_nand_protocol_program(write_addr, page_number, p_data);

	if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
		rtn_status = parallel_nand_protocol_get_status(&status);

		if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
			if ((status & PARALLEL_NAND_STATUS_FAIL_BIT) != 0U) {
				rtn_status = SPI_NAND_FLASH_RTN_PROGRAM_FAIL;
			}
		}
	}

	if (rtn_status == SPI_NAND_FLASH_RTN_CMD_ABORT) {
		rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	}

	return rtn_status;
}

/*------------------------------------------------------------------------------------
 * FUNCTION: SPI_NAND_FLASH_RTN_T parallel_nand_erase( u32 page_number )
 * PURPOSE : Erase the block containing page_number.
 *------------------------------------------------------------------------------------
 */
SPI_NAND_FLASH_RTN_T parallel_nand_erase(u32 page_number)
{
	SPI_NAND_FLASH_RTN_T	rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	u8						status = 0;

	parallel_nand_chip_select_by_page(&page_number);

	rtn_status = parallel_nand_protocol_erase(page_number);

	if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
		rtn_status = parallel_nand_protocol_get_status(&status);

		if (rtn_status == SPI_NAND_FLASH_RTN_NO_ERROR) {
			if ((status & PARALLEL_NAND_STATUS_FAIL_BIT) != 0U) {
				rtn_status = SPI_NAND_FLASH_RTN_ERASE_FAIL;
			}
		}
	}

	if (rtn_status == SPI_NAND_FLASH_RTN_CMD_ABORT) {
		rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
	}

	return rtn_status;
}
#endif /* !defined(LZMA_IMG) || defined(TCSUPPORT_BB_256KB) */

#endif /* TCSUPPORT_PARALLEL_NAND */

/* End of [parallel_nand_flash.c] package */
