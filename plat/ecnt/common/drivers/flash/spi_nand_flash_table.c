/***************************************************************************************
 *      Copyright(c) 2014 ECONET Incorporation All rights reserved.
 *
 *      This is unpublished proprietary source code of ECONET Networks Incorporation
 *
 *      The copyright notice above does not evidence any actual or intended
 *      publication of such source code.
 ***************************************************************************************
 */

/*======================================================================================
 * MODULE NAME: spi
 * FILE NAME: spi_nand_flash_table.c
 * DATE: 2017/08/06
 * VERSION: 2.00
 * PURPOSE: To Provide SPI NAND Access interface.
 * NOTES:
 *
 *   The chip ID list is no longer maintained here.  It lives in a single
 *   file, mtk-snand-ids.c: the upstream MediaTek table plus an appended
 *   "ecnt extension" section for chips that U-Boot supports but the upstream
 *   table does not.  Every entry is converted into the ecnt runtime
 *   descriptor (struct SPI_NAND_FLASH_INFO_T) by ecnt_info_from_mtk().
 *
 *   Fields that the upstream table does not carry (OOB free layout, ECC
 *   enable, write-enable ordering, ...) are derived from the chip geometry
 *   and vendor; see ecnt_info_from_mtk().
 *
 *======================================================================================
 */

#include <stdint.h>
#include <ecnt_spi_nand_flash.h>

extern struct spi_nand_flash_ooblayout *snand_oob_layout_by_id(u8 id);
extern const struct snand_quirks *snand_quirk_get(u8 id);
#include "mtk-snand-def.h"

#if defined(TCSUPPORT_BL2_OPTIMIZATION) && defined(IMAGE_BL2) && !defined(FLASH_TABLE_OPEN)

/*======================================================================================
 * BL2 optimized runtime
 *
 * flash.mk links mtk-snand-ids.c unconditionally, but this translation unit
 * never references the table in the BL2 branch; --gc-sections drops the
 * unused table/layout symbols.  The runtime descriptor is deserialized from
 * flash_table.bin, which the host generator below produces from that table.
 *======================================================================================*/

#include <platform_def.h>

struct SPI_NAND_FLASH_INFO_T spi_nand_flash_tables[1];
/* ===== SPI NAND Controller ECC ooblayout (defined in mtk-snand-ids.c) ===== */

static SPI_NAND_EXTEND_DEVICE_ID_STATUS_T check_extend_dev_id (struct SPI_NAND_FLASH_INFO_T *ptr_rtn_device_t, u32 index)
{
	u8 i, result = SPI_NAND_EXTEND_DEVICE_ID_MATCH;

	for (i=0 ; i<spi_nand_flash_tables[index].extend_dev_id.extend_len ; i++)
	{
		if (ptr_rtn_device_t->extend_dev_id.extend_id[i] != spi_nand_flash_tables[index].extend_dev_id.extend_id[i])
		{
			result = SPI_NAND_EXTEND_DEVICE_ID_UNMATCH;

			break;
		}
	}

	return result;
}

SPI_NAND_FLASH_RTN_T get_spi_nand_from_mem(struct SPI_NAND_FLASH_INFO_T *ptr_rtn_device_t, u8 read_method)
{

	int i = 0;
	int len = 0;
	int index = 0;
	int have_match = 0;
	int best_len = -1;
	struct SPI_NAND_FLASH_INFO_T best;
	unsigned char * currentP = (unsigned char *)ECNT_FLASH_TABLE_BASE;

	struct bl2_flash_H flash_h;

	memcpy(&flash_h, currentP, sizeof(struct bl2_flash_H));

	currentP += sizeof(struct bl2_flash_H);

	for (i = 0; i < flash_h.flash_entry; i++)
	{
		memcpy(&spi_nand_flash_tables[0].mfr_id, currentP, sizeof(spi_nand_flash_tables[0].mfr_id));
		currentP+=sizeof(spi_nand_flash_tables[0].mfr_id);
		memcpy(&spi_nand_flash_tables[0].dev_id, currentP, sizeof(spi_nand_flash_tables[0].dev_id));
		currentP+=sizeof(spi_nand_flash_tables[0].dev_id);
		memcpy(&spi_nand_flash_tables[0].device_size, currentP, sizeof(spi_nand_flash_tables[0].device_size));
		currentP+=sizeof(spi_nand_flash_tables[0].device_size);
		memcpy(&spi_nand_flash_tables[0].page_size, currentP, sizeof(spi_nand_flash_tables[0].page_size));
		currentP+=sizeof(spi_nand_flash_tables[0].page_size);
		memcpy(&spi_nand_flash_tables[0].erase_size, currentP, sizeof(spi_nand_flash_tables[0].erase_size));
		currentP+=sizeof(spi_nand_flash_tables[0].erase_size);
		memcpy(&spi_nand_flash_tables[0].oob_size, currentP, sizeof(spi_nand_flash_tables[0].oob_size));
		currentP+=sizeof(spi_nand_flash_tables[0].oob_size);
		memcpy(&spi_nand_flash_tables[0].dummy_mode, currentP, sizeof(spi_nand_flash_tables[0].dummy_mode));
		currentP+=sizeof(spi_nand_flash_tables[0].dummy_mode);
		memcpy(&spi_nand_flash_tables[0].read_mode, currentP, sizeof(spi_nand_flash_tables[0].read_mode));
		currentP+=sizeof(spi_nand_flash_tables[0].read_mode);
		memcpy(&spi_nand_flash_tables[0].die_num, currentP, sizeof(spi_nand_flash_tables[0].die_num));
		currentP+=sizeof(spi_nand_flash_tables[0].die_num);
		memcpy(&spi_nand_flash_tables[0].write_mode, currentP, sizeof(spi_nand_flash_tables[0].write_mode));
		currentP+=sizeof(spi_nand_flash_tables[0].write_mode);
		memcpy(&spi_nand_flash_tables[0].feature, currentP, sizeof(spi_nand_flash_tables[0].feature));
		currentP+=sizeof(spi_nand_flash_tables[0].feature);
		memcpy(&spi_nand_flash_tables[0].ecc_fail_check_info, currentP, sizeof(spi_nand_flash_tables[0].ecc_fail_check_info));
		currentP+=sizeof(spi_nand_flash_tables[0].ecc_fail_check_info);
		memcpy(&spi_nand_flash_tables[0].write_en_type, currentP, sizeof(spi_nand_flash_tables[0].write_en_type));
		currentP+=sizeof(spi_nand_flash_tables[0].write_en_type);
		memcpy(&spi_nand_flash_tables[0].unlock_block_info, currentP, sizeof(spi_nand_flash_tables[0].unlock_block_info));
		currentP+=sizeof(spi_nand_flash_tables[0].unlock_block_info);
		memcpy(&spi_nand_flash_tables[0].quad_en, currentP, sizeof(spi_nand_flash_tables[0].quad_en));
		currentP+=sizeof(spi_nand_flash_tables[0].quad_en);
		memcpy(&spi_nand_flash_tables[0].ecc_en, currentP, sizeof(spi_nand_flash_tables[0].ecc_en));
		currentP+=sizeof(spi_nand_flash_tables[0].ecc_en);
#ifdef TCSUPPORT_NAND_FLASH_OTP
		memcpy(&spi_nand_flash_tables[0].otp_page_num, currentP, sizeof(spi_nand_flash_tables[0].otp_page_num));
		currentP+=sizeof(spi_nand_flash_tables[0].otp_page_num);
#endif
		memcpy(&spi_nand_flash_tables[0].soc_ecc_ability, currentP, sizeof(spi_nand_flash_tables[0].soc_ecc_ability));
		currentP+=sizeof(spi_nand_flash_tables[0].soc_ecc_ability);

		memcpy(&spi_nand_flash_tables[0].extend_dev_id, currentP, sizeof(spi_nand_flash_tables[0].extend_dev_id));
		currentP+=sizeof(spi_nand_flash_tables[0].extend_dev_id);
		memcpy(&spi_nand_flash_tables[0].read_method, currentP, sizeof(spi_nand_flash_tables[0].read_method));
		currentP+=sizeof(spi_nand_flash_tables[0].read_method);
		memcpy(&spi_nand_flash_tables[0].oob_layout, currentP, sizeof(spi_nand_flash_tables[0].oob_layout));
		currentP+=sizeof(spi_nand_flash_tables[0].oob_layout);

		if (((ptr_rtn_device_t->mfr_id == spi_nand_flash_tables[0].mfr_id) &&
		     (ptr_rtn_device_t->dev_id == spi_nand_flash_tables[0].dev_id) &&
		     (check_extend_dev_id(ptr_rtn_device_t, 0) == SPI_NAND_EXTEND_DEVICE_ID_MATCH) &&
		     ((spi_nand_flash_tables[0].read_method == SPI_NAND_READ_METHOD_ANY) ||
		      (spi_nand_flash_tables[0].read_method == read_method))))
		{
			int elen = spi_nand_flash_tables[0].extend_dev_id.extend_len;

			/*
			 * Prefer the most specific entry: a longer ID wins over
			 * a shorter prefix, and a method specific entry wins
			 * over a method agnostic one at the same length.
			 */
			if (!have_match || elen > best_len ||
			    (elen == best_len &&
			     best.read_method == SPI_NAND_READ_METHOD_ANY)) {
				best = spi_nand_flash_tables[0];
				best_len = elen;
				index = i;
				have_match = 1;
			}
		}

	}

	if (have_match)
		spi_nand_flash_tables[0] = best;

	currentP = (unsigned char *)(ECNT_FLASH_TABLE_BASE + flash_h.flash_name_off);

	for (i = 0; i < flash_h.flash_entry; i++)
	{
		memcpy(&len, currentP, sizeof(int));
		currentP+=sizeof(int);

		if(i == index)
		{
			/* assign pointer to ptr_name.*/
			spi_nand_flash_tables[0].ptr_name = currentP;
			currentP = (unsigned char *)(ECNT_FLASH_TABLE_BASE + flash_h.flash_oob_off);
			break;

		}
		currentP+=len;

	}

	{
		u8 id = spi_nand_flash_tables[0].oob_layout;
		u8 idx = *((u8 *)(ECNT_FLASH_TABLE_BASE +
				  flash_h.flash_oob_map_off + id));

		if (idx >= flash_h.flash_oob_count)
			idx = 0;
		spi_nand_flash_tables[0].oob_free_layout =
			(struct spi_nand_flash_ooblayout *)(ECNT_FLASH_TABLE_BASE +
				flash_h.flash_oob_off +
				idx * sizeof(struct spi_nand_flash_ooblayout));
	}

	return SPI_NAND_FLASH_RTN_NO_ERROR;
}

SPI_NAND_FLASH_RTN_T scan_spi_nand_table(struct SPI_NAND_FLASH_INFO_T *ptr_rtn_device_t,
					 u8 read_method)
{
	u32 i = 0;
	SPI_NAND_FLASH_RTN_T rtn_status = SPI_NAND_FLASH_RTN_PROBE_ERROR;

	get_spi_nand_from_mem(ptr_rtn_device_t, read_method);

	for (i = 0; i < 1; i++)
	{
		if ( ( (ptr_rtn_device_t->mfr_id) == spi_nand_flash_tables[i].mfr_id) &&
		     ( (ptr_rtn_device_t->dev_id) == spi_nand_flash_tables[i].dev_id) &&
		     (check_extend_dev_id(ptr_rtn_device_t, i) == SPI_NAND_EXTEND_DEVICE_ID_MATCH) &&
		     ((spi_nand_flash_tables[i].read_method == SPI_NAND_READ_METHOD_ANY) ||
		      (spi_nand_flash_tables[i].read_method == read_method)) )
		{
			ptr_rtn_device_t->device_size 			= spi_nand_flash_tables[i].device_size;
			ptr_rtn_device_t->erase_size  			= spi_nand_flash_tables[i].erase_size;
			ptr_rtn_device_t->page_size	  			= spi_nand_flash_tables[i].page_size;
			ptr_rtn_device_t->oob_size	  			= spi_nand_flash_tables[i].oob_size;
			ptr_rtn_device_t->dummy_mode  			= spi_nand_flash_tables[i].dummy_mode;
			ptr_rtn_device_t->read_mode	  			= spi_nand_flash_tables[i].read_mode;
			ptr_rtn_device_t->write_mode			= spi_nand_flash_tables[i].write_mode;
#ifdef TCSUPPORT_NAND_FLASH_OTP
			ptr_rtn_device_t->otp_page_num			= spi_nand_flash_tables[i].otp_page_num;
#endif
			ptr_rtn_device_t->soc_ecc_ability			= spi_nand_flash_tables[i].soc_ecc_ability;
			memcpy( &(ptr_rtn_device_t->ptr_name) , &(spi_nand_flash_tables[i].ptr_name), sizeof(ptr_rtn_device_t->ptr_name));
			memcpy( &(ptr_rtn_device_t->oob_free_layout) , &(spi_nand_flash_tables[i].oob_free_layout), sizeof(ptr_rtn_device_t->oob_free_layout));
			ptr_rtn_device_t->feature = spi_nand_flash_tables[i].feature;
			ptr_rtn_device_t->die_num = spi_nand_flash_tables[i].die_num;
			memcpy( &(ptr_rtn_device_t->ecc_fail_check_info) , &(spi_nand_flash_tables[i].ecc_fail_check_info), sizeof(struct SPI_NAND_ECC_FAIL_CHECK_INFO_T));
			ptr_rtn_device_t->write_en_type = spi_nand_flash_tables[i].write_en_type;
			memcpy( &(ptr_rtn_device_t->unlock_block_info) , &(spi_nand_flash_tables[i].unlock_block_info), sizeof(struct SPI_NAND_UNLOCK_BLOCK_INFO_T));
			memcpy( &(ptr_rtn_device_t->quad_en) , &(spi_nand_flash_tables[i].quad_en), sizeof(struct SPI_NAND_QUAD_EN_INFO_T));
			memcpy( &(ptr_rtn_device_t->ecc_en) , &(spi_nand_flash_tables[i].ecc_en), sizeof(struct SPI_NAND_ECC_EN_INFO_T));
			memcpy( &(ptr_rtn_device_t->extend_dev_id) , &(spi_nand_flash_tables[i].extend_dev_id), sizeof(struct SPI_NAND_EXTEND_ID_T));
			memcpy( &(ptr_rtn_device_t->read_ecc_ceck) , &(spi_nand_flash_tables[i].read_ecc_ceck), sizeof(struct SPI_NAND_READ_ECC_CHECK_T));
			ptr_rtn_device_t->read_method = spi_nand_flash_tables[i].read_method;
			ptr_rtn_device_t->oob_layout = spi_nand_flash_tables[i].oob_layout;

			rtn_status = SPI_NAND_FLASH_RTN_NO_ERROR;
			break;
		}
	}

	return rtn_status;
}

#else /* !(TCSUPPORT_BL2_OPTIMIZATION && IMAGE_BL2 && !FLASH_TABLE_OPEN) */

/*======================================================================================
 * OOB free layouts
 *
 * These describe the user-visible free area of the spare region.  Each chip
 * names one of them explicitly in mtk-snand-ids.c (SNAND_INFO's sixth
 * argument); snand_oob_layout_by_id() maps that index to a definition here.
 * There is no vendor/geometry guess at runtime any more.
 *======================================================================================*/


/*======================================================================================
 * Conversion from the upstream descriptor to the ecnt runtime descriptor
 *======================================================================================*/


/*
 * ECC status layout carried by the chip table (enum snand_ecc_status).
 *
 * The value is derived from U-Boot's per-family *_ecc_get_status(); U-Boot
 * enables on-die ECC for every chip through CFG_ECC_ENABLE (0xB0 bit 4), so
 * only the status decode differs between families.
 */
static void ecnt_ecc_status_info(u8 status,
				 struct SPI_NAND_ECC_FAIL_CHECK_INFO_T *e)
{
	switch (status) {
	case SNAND_ECC_70_70:
		e->ecc_check_mask = 0x70;
		e->ecc_uncorrected_value = 0x70;
		break;
	case SNAND_ECC_70_20:
		e->ecc_check_mask = 0x70;
		e->ecc_uncorrected_value = 0x20;
		break;
	case SNAND_ECC_3C_20:
		e->ecc_check_mask = 0x3c;
		e->ecc_uncorrected_value = 0x20;
		break;
	case SNAND_ECC_30_30:
		e->ecc_check_mask = 0x30;
		e->ecc_uncorrected_value = 0x30;
		break;
	case SNAND_ECC_30_20:
	default:
		e->ecc_check_mask = 0x30;
		e->ecc_uncorrected_value = 0x20;
		break;
	}
}

static void ecnt_info_from_mtk(const struct snand_flash_info *f,
			       struct SPI_NAND_FLASH_INFO_T *d)
{
	const struct snand_mem_org *m = &f->memorg;
	u32 caps_rd = f->cap_rd ? f->cap_rd->caps : 0;

	/*
	 * Only clear the fields that may be partially filled.  Clearing the
	 * whole descriptor would also drop fields managed by the probe or by
	 * SPI_NAND_Flash_Set_Flash_Info().
	 */
	memset(&d->extend_dev_id, 0, sizeof(d->extend_dev_id));
	memset(&d->read_ecc_ceck, 0, sizeof(d->read_ecc_ceck));

	d->mfr_id = f->id.id[0];
	d->dev_id = f->id.id[1];
	d->read_method = f->id.type;
	d->ptr_name = (const u8 *)f->model;

	d->page_size = m->pagesize;
	d->oob_size = m->sparesize;
	d->erase_size = (u32)m->pagesize * m->pages_per_block;
	d->die_num = m->ndies ? m->ndies : 1;
	/*
	 * blocks_per_die already covers every plane of a die; planes_per_die
	 * is only a subdivision (selected through the column address) and must
	 * not multiply the total capacity.
	 */
	d->device_size = d->erase_size * m->blocks_per_die * d->die_num;

	/* Extended device ID bytes after the 2-byte vendor/device ID. */
	if (f->id.len > 2) {
		u8 n = f->id.len - 2;
		u8 i;

		if (n > MAX_SPI_NAND_FLASH_EXTEND_DEV_ID_LEN)
			n = MAX_SPI_NAND_FLASH_EXTEND_DEV_ID_LEN;

		d->extend_dev_id.extend_len = n;
		for (i = 0; i < n; i++)
			d->extend_dev_id.extend_id[i] = f->id.id[2 + i];
	}

	/*
	 * Bus modes.  The ecnt controller/board is only validated for dual
	 * read and single program: the previous hand-written table used
	 * DUAL/SINGLE for every one of its chips and never enabled quad.
	 * Sending quad read (0x6b) here makes the whole page read garbage on
	 * boards that do not wire IO2/IO3, which is what broke UBI
	 * ("Bad EC magic in block ...").  Keep this rule global; do not add
	 * per-chip overrides.
	 */
	if ((f->flags & SNAND_FLAG_QUAD_ENABLE) &&
	    (caps_rd & (SPI_IO_1_1_4 | SPI_IO_1_4_4)))
		d->read_mode = SPI_NAND_FLASH_READ_SPEED_MODE_QUAD;
	else if (caps_rd & (SPI_IO_1_1_2 | SPI_IO_1_2_2))
		d->read_mode = SPI_NAND_FLASH_READ_SPEED_MODE_DUAL;
	else
		d->read_mode = SPI_NAND_FLASH_READ_SPEED_MODE_SINGLE;

	d->write_mode = SPI_NAND_FLASH_WRITE_SPEED_MODE_SINGLE;

	/* Upstream read commands put the dummy byte after the address. */
	d->dummy_mode = SPI_NAND_FLASH_READ_DUMMY_BYTE_APPEND;

	/*
	 * Plane/die selection is a table property: the table carries
	 * SNAND_FLAG_PLANE_SELECT and enum snand_die_select, so the driver no
	 * longer guesses from the vendor ID.
	 */
	d->feature = SPI_NAND_FLASH_FEATURE_NONE;
	if (f->flags & SNAND_FLAG_PLANE_SELECT)
		d->feature |= SPI_NAND_FLASH_PLANE_SELECT_HAVE;
	if (f->die_select == SNAND_DIE_SELECT_2)
		d->feature |= SPI_NAND_FLASH_DIE_SELECT_2_HAVE;
	else if (f->die_select == SNAND_DIE_SELECT_1)
		d->feature |= SPI_NAND_FLASH_DIE_SELECT_1_HAVE;

	/* Functional feature bits carried one-to-one by the chip table. */
	if (f->flags & SNAND_FLAG_NO_ECC_STATUS)
		d->feature |= SPI_NAND_FLASH_NO_ECC_STATUS_HAVE;
	if (f->flags & SNAND_FLAG_DISABLE_CONT_RD)
		d->feature |= SPI_NAND_FLASH_DISABLE_CONTINUOUS_RD;
	if (f->flags & SNAND_FLAG_ERASE_STATS)
		d->feature |= SPI_NAND_FLASH_ERASE_STATISTICS;
	if (f->flags & SNAND_FLAG_BLANK_PAGE_FIXUP)
		d->feature |= SPI_NAND_FLASH_BLANK_PAGE_FIXUP;
	if (f->flags & SNAND_FLAG_OOB_RESERVE_BMT)
		d->feature |= SPI_NAND_FLASH_OOB_RESERVE_FOR_BMT;
	if (f->flags & SNAND_FLAG_READ_ECC_ERR_CHECK)
		d->feature |= SPI_NAND_FLASH_READ_ECC_ERROR_BIT_CHECK;
	if (f->flags & SNAND_FLAG_WINBOND_BLANK_PARITY)
		d->feature |= SPI_NAND_FLASH_WINBOND_BLANK_PARITY;

	/*
	 * Prefer the layout carried by the chip table; only fall back to the
	 * vendor/geometry heuristic when the table entry has no layout.
	 */
	d->oob_layout = f->oob_layout;
	d->oob_free_layout = NULL;
	if (f->oob_layout)
		d->oob_free_layout = snand_oob_layout_by_id(f->oob_layout);
	if (!d->oob_free_layout)
		d->oob_free_layout = &ooblayout_spi_controller_ecc;

	/*
	 * Defaults for the fields the upstream table does not carry.
	 *
	 * Do not write registers with guessed values during boot:
	 *  - unlock_block_info is applied by spi_nand_manufacute_init() to the
	 *    protection register (0xA0) on every probe.  Reads do not need it
	 *    and a wrong mask can touch a reserved/other-function bit, so leave
	 *    it disabled unless a table entry provides it.
	 *  - quad_en is only consumed by enable_quad(), which runs only for a
	 *    QUAD read/write mode; this port is fixed to DUAL/SINGLE, so the QE
	 *    write is never reached.  Keep the mask at zero instead of a guess.
	 */
	const struct snand_quirks *q = snand_quirk_get(f->quirks);

	ecnt_ecc_status_info(f->ecc_status, &d->ecc_fail_check_info);

	/* ECC enable register: default 0xB0/0x10/0x10. */
	d->ecc_en.ecc_en_addr = _SPI_NAND_ADDR_FEATURE;
	d->ecc_en.ecc_en_mask = 0x10;
	d->ecc_en.ecc_en_value = 0x10;
	if (q->ecc_en == 1) {
		d->ecc_en.ecc_en_addr = 0x90;
	} else if (q->ecc_en == 2) {
		d->ecc_en.ecc_en_mask = 0x00;
		d->ecc_en.ecc_en_value = 0x00;
	}

	/* Block protection unlock values from the pre-migration table. */
	d->unlock_block_info.unlock_block_mask = 0x38;
	d->unlock_block_info.unlock_block_value = 0x00;
	switch (q->unlock) {
	case 1:
		d->unlock_block_info.unlock_block_mask = 0x7C;
		break;
	case 2:
		d->unlock_block_info.unlock_block_mask = 0xFB;
		break;
	case 3:
		d->unlock_block_info.unlock_block_mask = 0x78;
		break;
	case 4:
		d->unlock_block_info.unlock_block_mask = 0x7A;
		d->unlock_block_info.unlock_block_value = 0x02;
		break;
	}

	d->quad_en.quad_en_mask = 0x00;
	d->quad_en.quad_en_value = 0x00;
	/* Table hook for a board that actually routes IO2/IO3. */
	if (f->flags & SNAND_FLAG_QUAD_ENABLE) {
		d->quad_en.quad_en_mask = 0x01;
		d->quad_en.quad_en_value = 0x01;
	}

	d->otp_page_num = -1;
	d->soc_ecc_ability = q->soc_ecc ? q->soc_ecc : 12;

	/* Read ECC threshold check (runtime only; 0 = disabled). */
	{
		static const struct SPI_NAND_READ_ECC_CHECK_T ecnt_read_ecc[] = {
			[0] = { 0, { 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x00 } },
			[1] = { 1, { 0x30, 0x00, 0x00 }, { 0x10, 0x60, 0x00 } },
			[2] = { 2, { 0x50, 0x00, 0x00 }, { 0x00, 0x00, 0x00 } },
			[3] = { 3, { 0x30, 0x10, 0x00 }, { 0xF0, 0x20, 0x30 } },
			[4] = { 1, { 0x30, 0x00, 0x00 }, { 0x10, 0x30, 0x00 } },
		};
		u8 re = q->read_ecc <= 4 ? q->read_ecc : 0;

		d->read_ecc_ceck = ecnt_read_ecc[re];
	}

	/*
	 * Write-enable ordering is a per-chip property carried by the table.
	 */
	if (f->flags & SNAND_FLAG_LOAD_FIRST)
		d->write_en_type = SPI_NAND_FLASH_WRITE_LOAD_FIRST;
	else
		d->write_en_type = SPI_NAND_FLASH_WRITE_EN_FIRST;
}

#ifndef FLASH_TABLE_OPEN
static void ecnt_consider(const struct snand_flash_info *f, const u8 *buf,
			  u8 method, const struct snand_flash_info **best)
{
	if (!f)
		return;
	if (f->id.id[0] != buf[0] || f->id.id[1] != buf[1])
		return;
	if (f->id.len > 2 && f->id.id[2] != buf[2])
		return;
	if (f->id.len > 3 && f->id.id[3] != buf[3])
		return;
	/* A method-specific entry only matches its own read sequence. */
	if (f->id.type != SNAND_ID_ANY && f->id.type != method)
		return;

	if (!*best ||
	    f->id.len > (*best)->id.len ||
	    (f->id.len == (*best)->id.len && (*best)->id.type == SNAND_ID_ANY))
		*best = f;
}

/*
 * Find the chip descriptor for the ID reported by the chip.  The upstream
 * MediaTek table is searched first-class together with the U-Boot supplement;
 * a longer (more specific) ID wins over a shorter prefix, and a method
 * specific entry wins over a method agnostic one.
 */
static const struct snand_flash_info *ecnt_snand_lookup(
				const struct SPI_NAND_FLASH_INFO_T *id,
				u8 method)
{
	const struct snand_flash_info *best = NULL;
	u8 buf[SNAND_MAX_ID_LEN] = { 0, 0, 0, 0 };
	u32 i;

	buf[0] = id->mfr_id;
	buf[1] = id->dev_id;
	/*
	 * The probe always reads MAX_SPI_NAND_FLASH_EXTEND_DEV_ID_LEN bytes
	 * into extend_id[].  extend_len is a table-side field and is never
	 * filled by the probe, so it must not gate the copy (otherwise every
	 * 3-byte upstream ID would be skipped here).
	 */
	for (i = 0; i < MAX_SPI_NAND_FLASH_EXTEND_DEV_ID_LEN && i < 2; i++)
		buf[2 + i] = id->extend_dev_id.extend_id[i];

	for (i = 0; i < snand_flash_id_count(); i++)
		ecnt_consider(snand_flash_id_index(i), buf, method, &best);

	return best;
}
#endif /* !FLASH_TABLE_OPEN */

#ifdef FLASH_TABLE_OPEN

/*======================================================================================
 * Host-side flash_table.bin generator
 *======================================================================================*/

#include <stdio.h>
#include <stdlib.h>

#define SPI_NAND_TABLE_MAX	512

static struct SPI_NAND_FLASH_INFO_T gen_flash_tables[SPI_NAND_TABLE_MAX];
/*
 * The header also declares "extern const ... spi_nand_flash_tables[]" for the
 * non-BL2 build, so keep the serialization code below unchanged by mapping the
 * name onto the generator-local array.
 */
#define spi_nand_flash_tables gen_flash_tables

int main(void){
	int num;
	int i;
	FILE *fp;
	int name_len = 0;
	unsigned int total_size = 0;
	int buf_size = 1000000; //16M
	struct bl2_flash_H flash_h;
	char *buf = NULL;
	unsigned char oob_map[256] = { 0 };
	unsigned int oob_count = 1;
	struct spi_nand_flash_ooblayout *oob_pool = NULL;

	num = (int)snand_flash_id_count();
	if (num > SPI_NAND_TABLE_MAX) {
		printf("too many spi nand entries: %d\n", num);
		return -1;
	}

	for (i = 0; i < num; i++)
		ecnt_info_from_mtk(snand_flash_id_index(i), &spi_nand_flash_tables[i]);

	/*
	 * Build the deduplicated OOB layout pool.  Pool index 0 is the
	 * controller-ECC layout, the fallback for entries with oob_layout 0.
	 */
	oob_pool = malloc(256 * sizeof(*oob_pool));
	if (!oob_pool) {
		printf("oob pool malloc failed\n");
		return -1;
	}
	{
		struct spi_nand_flash_ooblayout *ctl = snand_oob_layout_by_id(255);

		if (ctl)
			memcpy(&oob_pool[0], ctl, sizeof(*oob_pool));
		else
			memset(&oob_pool[0], 0, sizeof(*oob_pool));
	}
	for (i = 0; i < num; i++) {
		u8 id = spi_nand_flash_tables[i].oob_layout;

		if (id == 0 || oob_map[id])
			continue;
		if (spi_nand_flash_tables[i].oob_free_layout && oob_count < 255) {
			oob_map[id] = (unsigned char)oob_count;
			memcpy(&oob_pool[oob_count],
			       spi_nand_flash_tables[i].oob_free_layout,
			       sizeof(*oob_pool));
			oob_count++;
		}
	}

	fp = fopen("flash_table.bin","w+");
	if(fp == NULL){
		printf("open flash_table.bin fail!!\r\n");
		return 0;
	}

	if (!(buf = malloc(buf_size))) {
		printf("malloc failed\n");
		fclose(fp);
		return -1;
	}

	memset(&flash_h, 0, sizeof(flash_h));
	flash_h.flash_entry = num;

	total_size += sizeof(struct bl2_flash_H);

	for (i = 0 ; i < num ;i++)
	{
		memcpy((buf+total_size), &spi_nand_flash_tables[i].mfr_id, sizeof(spi_nand_flash_tables[i].mfr_id));
		total_size +=sizeof(spi_nand_flash_tables[i].mfr_id);

		memcpy((buf+total_size), &spi_nand_flash_tables[i].dev_id, sizeof(spi_nand_flash_tables[i].dev_id));
		total_size +=sizeof(spi_nand_flash_tables[i].dev_id);

		memcpy((buf+total_size), &spi_nand_flash_tables[i].device_size, sizeof(spi_nand_flash_tables[i].device_size));
		total_size +=sizeof(spi_nand_flash_tables[i].device_size);

		memcpy((buf+total_size), &spi_nand_flash_tables[i].page_size, sizeof(spi_nand_flash_tables[i].page_size));
		total_size +=sizeof(spi_nand_flash_tables[i].page_size);

		memcpy((buf+total_size), &spi_nand_flash_tables[i].erase_size, sizeof(spi_nand_flash_tables[i].erase_size));
		total_size +=sizeof(spi_nand_flash_tables[i].erase_size);

		memcpy((buf+total_size), &spi_nand_flash_tables[i].oob_size, sizeof(spi_nand_flash_tables[i].oob_size));
		total_size +=sizeof(spi_nand_flash_tables[i].oob_size);

		memcpy((buf+total_size), &spi_nand_flash_tables[i].dummy_mode, sizeof(spi_nand_flash_tables[i].dummy_mode));
		total_size +=sizeof(spi_nand_flash_tables[i].dummy_mode);

		memcpy((buf+total_size), &spi_nand_flash_tables[i].read_mode, sizeof(spi_nand_flash_tables[i].read_mode));
		total_size +=sizeof(spi_nand_flash_tables[i].read_mode);
		memcpy((buf+total_size), &spi_nand_flash_tables[i].die_num, sizeof(spi_nand_flash_tables[i].die_num));
		total_size +=sizeof(spi_nand_flash_tables[i].die_num);

		memcpy((buf+total_size), &spi_nand_flash_tables[i].write_mode, sizeof(spi_nand_flash_tables[i].write_mode));
		total_size +=sizeof(spi_nand_flash_tables[i].write_mode);

		memcpy((buf+total_size), &spi_nand_flash_tables[i].feature, sizeof(spi_nand_flash_tables[i].feature));
		total_size +=sizeof(spi_nand_flash_tables[i].feature);

		memcpy((buf+total_size), &spi_nand_flash_tables[i].ecc_fail_check_info, sizeof(spi_nand_flash_tables[i].ecc_fail_check_info));
		total_size +=sizeof(spi_nand_flash_tables[i].ecc_fail_check_info);

		memcpy((buf+total_size), &spi_nand_flash_tables[i].write_en_type, sizeof(spi_nand_flash_tables[i].write_en_type));
		total_size +=sizeof(spi_nand_flash_tables[i].write_en_type);

		memcpy((buf+total_size), &spi_nand_flash_tables[i].unlock_block_info, sizeof(spi_nand_flash_tables[i].unlock_block_info));
		total_size +=sizeof(spi_nand_flash_tables[i].unlock_block_info);

		memcpy((buf+total_size), &spi_nand_flash_tables[i].quad_en, sizeof(spi_nand_flash_tables[i].quad_en));
		total_size +=sizeof(spi_nand_flash_tables[i].quad_en);

		memcpy((buf+total_size), &spi_nand_flash_tables[i].ecc_en, sizeof(spi_nand_flash_tables[i].ecc_en));
		total_size +=sizeof(spi_nand_flash_tables[i].ecc_en);

#ifdef TCSUPPORT_NAND_FLASH_OTP

		memcpy((buf+total_size), &spi_nand_flash_tables[i].otp_page_num, sizeof(spi_nand_flash_tables[i].otp_page_num));
		total_size +=sizeof(spi_nand_flash_tables[i].otp_page_num);

#endif
		memcpy((buf+total_size), &spi_nand_flash_tables[i].soc_ecc_ability, sizeof(spi_nand_flash_tables[i].soc_ecc_ability));
		total_size +=sizeof(spi_nand_flash_tables[i].soc_ecc_ability);

		memcpy((buf+total_size), &spi_nand_flash_tables[i].extend_dev_id, sizeof(spi_nand_flash_tables[i].extend_dev_id));
		total_size +=sizeof(spi_nand_flash_tables[i].extend_dev_id);

		memcpy((buf+total_size), &spi_nand_flash_tables[i].read_method, sizeof(spi_nand_flash_tables[i].read_method));
		total_size +=sizeof(spi_nand_flash_tables[i].read_method);

		memcpy((buf+total_size), &spi_nand_flash_tables[i].oob_layout, sizeof(spi_nand_flash_tables[i].oob_layout));
		total_size +=sizeof(spi_nand_flash_tables[i].oob_layout);
	}

	flash_h.flash_name_off = total_size;

	for(i=0; i<num; i++){
		name_len = strlen((const char *)spi_nand_flash_tables[i].ptr_name);
		name_len++;
		memcpy((buf+total_size), &name_len, 4);

		memcpy((buf+total_size+4), spi_nand_flash_tables[i].ptr_name, name_len);
		total_size += (name_len+4);
	}
	/* for 4 byte-alignment */
	if ((total_size % 4)!= 0 )
	{
		memcpy((buf+total_size), &total_size, 4-(total_size %4));
		total_size += (4-(total_size %4));
	}
	flash_h.flash_oob_map_off = total_size;
	memcpy((buf+total_size), oob_map, sizeof(oob_map));
	total_size += sizeof(oob_map);

	flash_h.flash_oob_off = total_size;
	flash_h.flash_oob_count = oob_count;
	memcpy((buf+total_size), oob_pool, oob_count * sizeof(*oob_pool));
	total_size += oob_count * sizeof(*oob_pool);

	memcpy(buf, &flash_h, sizeof(struct bl2_flash_H));

	fwrite(buf, total_size, 1 ,fp);
	fclose(fp);

	free(buf);

	return 0;
}

#else /* !FLASH_TABLE_OPEN */

/*======================================================================================
 * Normal runtime lookup
 *======================================================================================*/

SPI_NAND_FLASH_RTN_T scan_spi_nand_table(struct SPI_NAND_FLASH_INFO_T *ptr_rtn_device_t,
					 u8 read_method)
{
	const struct snand_flash_info *f;

	f = ecnt_snand_lookup(ptr_rtn_device_t, read_method);
	if (!f)
		return SPI_NAND_FLASH_RTN_PROBE_ERROR;

	ecnt_info_from_mtk(f, ptr_rtn_device_t);

	return SPI_NAND_FLASH_RTN_NO_ERROR;
}

#endif /* FLASH_TABLE_OPEN */

#endif /* BL2 optimized runtime */
/* End of [spi_nand_flash_table.c] package */