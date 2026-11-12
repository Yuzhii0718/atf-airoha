/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Minimal, self-contained definition set for the upstream MediaTek SPI-NAND
 * chip ID table (mtk-snand-ids.c).
 *
 * The original file (atf-mtksoc/plat/mediatek/apsoc_common/drivers/snfi/
 * mtk-snand-def.h) carries the whole MTK snand driver.  The ecnt port only
 * needs the descriptor types used by the ID table, so they are kept here
 * without pulling in the MTK driver, OS glue or platform headers.
 *
 * Keep this file in sync with the upstream ID table: if upstream extends
 * struct snand_mem_org or the capability macros, mirror it here.
 */

#ifndef ECNT_MTK_SNAND_DEF_H
#define ECNT_MTK_SNAND_DEF_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)			(sizeof(a) / sizeof((a)[0]))
#endif

/* SPI I/O modes ------------------------------------------------------------ */
enum snand_flash_io {
	SNAND_IO_1_1_1,
	SNAND_IO_1_1_2,
	SNAND_IO_1_2_2,
	SNAND_IO_1_1_4,
	SNAND_IO_1_4_4,

	__SNAND_IO_MAX
};

/*
 * Do not use BIT() here: ATF's utils_def.h also defines BIT, and this header
 * is included alongside platform_def.h, which produced a redefinition warning.
 */
#define SPI_IO_1_1_1			(1U << SNAND_IO_1_1_1)
#define SPI_IO_1_1_2			(1U << SNAND_IO_1_1_2)
#define SPI_IO_1_2_2			(1U << SNAND_IO_1_2_2)
#define SPI_IO_1_1_4			(1U << SNAND_IO_1_1_4)
#define SPI_IO_1_4_4			(1U << SNAND_IO_1_4_4)

struct snand_opcode {
	uint8_t opcode;
	uint8_t dummy;
};

struct snand_io_cap {
	uint8_t caps;
	struct snand_opcode opcodes[__SNAND_IO_MAX];
};

#define SNAND_OP(_io, _opcode, _dummy) [_io] = { .opcode = (_opcode), \
						 .dummy = (_dummy) }

#define SNAND_IO_CAP(_name, _caps, ...) \
	struct snand_io_cap _name = { .caps = (_caps), \
				      .opcodes = { __VA_ARGS__ } }

/* Chip ID ----------------------------------------------------------------- */
#define SNAND_MAX_ID_LEN		4

/*
 * Read-ID method, used as part of the lookup key.  Chips can share the same
 * vendor/device ID but respond to a different READ ID sequence The first three
 * values mirror U-Boot's SPINAND_READID_METHOD_OPCODE / _ADDR / _DUMMY; SNAND_ID_ANY
 * is the method-agnostic default used by the ecnt extension.
 */
enum snand_id_type {
	SNAND_ID_OPCODE = 0,	/* 0x9F only (U-Boot SPINAND_READID_METHOD_OPCODE) */
	SNAND_ID_ADDR,		/* 0x9F + 1 address byte */
	SNAND_ID_DUMMY,		/* 0x9F + 1 dummy byte */
	SNAND_ID_ANY,		/* method agnostic (ecnt extension) */

	__SNAND_ID_TYPE_MAX
};

struct snand_id {
	uint8_t type;	/* enum snand_id_type */
	uint8_t len;
	uint8_t id[SNAND_MAX_ID_LEN];
};

#define SNAND_ID(_type, ...) \
	{ .type = (_type), .id = { __VA_ARGS__ }, \
	  .len = sizeof((uint8_t[]) { __VA_ARGS__ }) }

/* Memory organization ----------------------------------------------------- */
struct snand_mem_org {
	uint16_t pagesize;
	uint16_t sparesize;
	uint16_t pages_per_block;
	uint16_t blocks_per_die;
	uint16_t planes_per_die;
	uint16_t ndies;
};

#define SNAND_MEMORG(_ps, _ss, _ppb, _bpd, _ppd, _nd) \
	{ .pagesize = (_ps), .sparesize = (_ss), .pages_per_block = (_ppb), \
	  .blocks_per_die = (_bpd), .planes_per_die = (_ppd), .ndies = (_nd) }

/*
 * OOB free-layout index.  The actual layouts are defined in
 * spi_nand_flash_table.c as ooblayout_typeN; the chip table only stores
 * the index so it stays independent of the driver translation unit.
 *
 * Types 1..22 come from the pre-migration ecnt table and are kept for
 * boards that already have BMT/UBI metadata written with those layouts;
 * changing them would move the metadata.  Types 23..50 are derived from
 * U-Boot's per-vendor ooblayout ops and are used by chips that only exist
 * in the U-Boot table.  Do not renumber or merge the two sets.
 */
enum snand_oob_layout {
	SNAND_OOB_NONE = 0,
	SNAND_OOB_TYPE1 = 1,
	SNAND_OOB_TYPE2 = 2,
	SNAND_OOB_TYPE3 = 3,
	SNAND_OOB_TYPE4 = 4,
	SNAND_OOB_TYPE5 = 5,
	SNAND_OOB_TYPE6 = 6,
	SNAND_OOB_TYPE7 = 7,
	SNAND_OOB_TYPE8 = 8,
	SNAND_OOB_TYPE9 = 9,
	SNAND_OOB_TYPE10 = 10,
	SNAND_OOB_TYPE11 = 11,
	SNAND_OOB_TYPE12 = 12,
	SNAND_OOB_TYPE13 = 13,
	SNAND_OOB_TYPE14 = 14,
	SNAND_OOB_TYPE15 = 15,
	SNAND_OOB_TYPE16 = 16,
	SNAND_OOB_TYPE17 = 17,
	SNAND_OOB_TYPE18 = 18,
	SNAND_OOB_TYPE19 = 19,
	SNAND_OOB_TYPE20 = 20,
	SNAND_OOB_TYPE21 = 21,
	SNAND_OOB_TYPE22 = 22,
	SNAND_OOB_TYPE23 = 23,
	SNAND_OOB_TYPE24 = 24,
	SNAND_OOB_TYPE25 = 25,
	SNAND_OOB_TYPE26 = 26,
	SNAND_OOB_TYPE27 = 27,
	SNAND_OOB_TYPE28 = 28,
	SNAND_OOB_TYPE29 = 29,
	SNAND_OOB_TYPE30 = 30,
	SNAND_OOB_TYPE31 = 31,
	SNAND_OOB_TYPE32 = 32,
	SNAND_OOB_TYPE33 = 33,
	SNAND_OOB_TYPE34 = 34,
	SNAND_OOB_TYPE35 = 35,
	SNAND_OOB_TYPE36 = 36,
	SNAND_OOB_TYPE37 = 37,
	SNAND_OOB_TYPE38 = 38,
	SNAND_OOB_TYPE39 = 39,
	SNAND_OOB_TYPE40 = 40,
	SNAND_OOB_TYPE41 = 41,
	SNAND_OOB_TYPE42 = 42,
	SNAND_OOB_TYPE43 = 43,
	SNAND_OOB_TYPE44 = 44,
	SNAND_OOB_TYPE45 = 45,
	SNAND_OOB_TYPE46 = 46,
	SNAND_OOB_TYPE47 = 47,
	SNAND_OOB_TYPE48 = 48,
	SNAND_OOB_TYPE49 = 49,
	SNAND_OOB_TYPE50 = 50,
	SNAND_OOB_CONTROLLER = 255,
};

#define SNAND_FLAG_NONE		0

/* Chip descriptor --------------------------------------------------------- */

/*
 * On-die ECC status layout, mirroring U-Boot's per-family get_status().
 * Values are (status mask, uncorrectable value).
 */
enum snand_ecc_status {
	SNAND_ECC_30_20 = 0,	/* U-Boot generic STATUS_ECC_MASK/UNCOR */
	SNAND_ECC_70_70,	/* GigaDevice Q4UF */
	SNAND_ECC_70_20,	/* Micron / DOSICON / UNILC */
	SNAND_ECC_3C_20,	/* XTX XT26G0xA */
	SNAND_ECC_30_30,	/* Skyhigh */

	__SNAND_ECC_STATUS_MAX
};

/* How the driver switches between dies (maps to the ecnt feature bits). */
enum snand_die_select {
	SNAND_DIE_SELECT_NONE = 0,
	SNAND_DIE_SELECT_1,	/* 0xC2 + die index */
	SNAND_DIE_SELECT_2,	/* 0xD0 bit6 */
};

/* Per-chip flags; new board/feature requirements grow here. */
#define SNAND_FLAG_QUAD_CAPABLE	(1U << 0)	/* cap_rd has 4-bit I/O */
#define SNAND_FLAG_QUAD_ENABLE	(1U << 1)	/* write QE at boot */
#define SNAND_FLAG_PLANE_SELECT	(1U << 2)	/* use plane-select bit */
#define SNAND_FLAG_LOAD_FIRST	(1U << 3)	/* program-load before write-enable */
#define SNAND_FLAG_NO_ECC_STATUS	(1U << 4)
#define SNAND_FLAG_DISABLE_CONT_RD	(1U << 5)
#define SNAND_FLAG_ERASE_STATS		(1U << 6)
#define SNAND_FLAG_BLANK_PAGE_FIXUP	(1U << 7)
#define SNAND_FLAG_OOB_RESERVE_BMT	(1U << 8)
#define SNAND_FLAG_READ_ECC_ERR_CHECK	(1U << 9)
#define SNAND_FLAG_WINBOND_BLANK_PARITY	(1U << 10)

/*
 * Per-chip quirks that only a few chips need.  The table stores an index into
 * snand_quirks[]; 0 means "all defaults".
 */
struct snand_quirks {
	uint8_t ecc_en;		/* 0=0xB0/0x10, 1=0x90/0x10, 2=0xB0/0x00 */
	uint8_t unlock;		/* 0=0x38/0x00, 1=0x7C, 2=0xFB, 3=0x78, 4=0x7A/0x02 */
	uint8_t read_ecc;	/* 0=none, 1..5 (see ecnt_read_ecc_quirk()) */
	uint8_t soc_ecc;	/* 0=default (12), else the strength value */
};

/* Quirk indices carried by the table; actual values live in mtk-snand-ids.c. */
enum snand_quirk_id {
	SNAND_QUIRK_DEFAULT = 0,
	SNAND_QUIRK_ECC_90,		/* ecc enable register 0x90 */
	SNAND_QUIRK_ECC_OFF,		/* ecc enable register 0xB0, value 0 */
	SNAND_QUIRK_ECC_OFF_SOC4,	/* + SoC ECC strength 4 */
	SNAND_QUIRK_UNLOCK_7C,
	SNAND_QUIRK_UNLOCK_FB,
	SNAND_QUIRK_UNLOCK_78,
	SNAND_QUIRK_UNLOCK_7A2,
	SNAND_QUIRK_UNLOCK_7C_READ1,
	SNAND_QUIRK_UNLOCK_7C_READ2,
	SNAND_QUIRK_UNLOCK_78_READ4,
	SNAND_QUIRK_READ1,
	SNAND_QUIRK_READ3,
};

struct snand_flash_info {
	const char *model;
	struct snand_id id;
	const struct snand_mem_org memorg;
	const struct snand_io_cap *cap_rd;
	const struct snand_io_cap *cap_pl;
	uint8_t oob_layout;	/* user OOB free-layout index */
	uint8_t ecc_status;	/* enum snand_ecc_status */
	uint8_t die_select;	/* enum snand_die_select */
	uint8_t quirks;		/* index into snand_quirks[] */
	uint16_t flags;		/* SNAND_FLAG_* */
};

#define SNAND_INFO(_model, _id, _memorg, _cap_rd, _cap_pl, _oob, _ecc, _die, _quirk, _flags) \
	{ .model = (_model), .id = _id, .memorg = _memorg, \
	  .cap_rd = (_cap_rd), .cap_pl = (_cap_pl), \
	  .oob_layout = (_oob), .ecc_status = (_ecc), \
	  .die_select = (_die), .quirks = (_quirk), .flags = (_flags) }

/* Opcodes referenced by the capability tables ----------------------------- */
#define SNAND_CMD_RESET			0xff
#define SNAND_CMD_BLOCK_ERASE		0xd8
#define SNAND_CMD_READ_FROM_CACHE	0x0b
#define SNAND_CMD_READ_FROM_CACHE_SINGLE	0x03
#define SNAND_CMD_READ_FROM_CACHE_X2	0x3b
#define SNAND_CMD_READ_FROM_CACHE_DUAL	0xbb
#define SNAND_CMD_READ_FROM_CACHE_X4	0x6b
#define SNAND_CMD_READ_FROM_CACHE_QUAD	0xeb
#define SNAND_CMD_READID		0x9f
#define SNAND_CMD_PROGRAM_LOAD		0x02
#define SNAND_CMD_PROGRAM_LOAD_X4	0x32
#define SNAND_CMD_PROGRAM_LOAD_RANDOM	0x84
#define SNAND_CMD_PROGRAM_LOAD_RANDOM_X4	0x34
#define SNAND_CMD_SET_FEATURE		0x1f
#define SNAND_CMD_GET_FEATURE		0x0f
#define SNAND_CMD_WRITE_ENABLE		0x06
#define SNAND_CMD_WRITE_DISABLE		0x04
#define SNAND_CMD_PROGRAM_EXECUTE	0x10
#define SNAND_CMD_READ_TO_CACHE		0x13
#define SNAND_CMD_WINBOND_SELECT_DIE	0xc2

/* Exported by mtk-snand-ids.c -------------------------------------------- */
const struct snand_flash_info *snand_flash_id_lookup(enum snand_id_type type,
						     const uint8_t *id);
uint32_t snand_flash_id_count(void);
const struct snand_flash_info *snand_flash_id_index(uint32_t index);

#endif /* ECNT_MTK_SNAND_DEF_H */
