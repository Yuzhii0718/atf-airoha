/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Cadence DDR Driver
 *
 * Copyright (C) 2012-2026 Cadence Design Systems, Inc.
 * Copyright (C) 2018-2026 Texas Instruments Incorporated - https://www.ti.com/
 */

#ifndef CPS_DRV_LPDDR4_H
#define CPS_DRV_LPDDR4_H

/*
 * FIELD_GET / FIELD_PREP: identical definitions exist in
 * include/drivers/cadence/cdns_nand.h (marked "TBD: Move to common place").
 * They are inlined here rather than included from that header because
 * cdns_nand.h transitively pulls in cdns_combo_phy.h, which requires
 * <stdint.h> types not available at this point in the DDR driver include
 * chain. Once a suitable common TF-A header is agreed on, both copies
 * should be consolidated there.
 */
#ifndef bf_shf
#define bf_shf(x)		(__builtin_ffsll(x) - 1U)
#endif

#ifndef FIELD_GET
#define FIELD_GET(_mask, _reg)	\
	({ \
		(typeof(_mask))(((_reg) & (_mask)) >> bf_shf(_mask)); \
	})
#endif

#ifndef FIELD_PREP
#define FIELD_PREP(_mask, _val) \
	({ \
		((typeof(_mask))(_val) << bf_shf(_mask)) & (_mask); \
	})
#endif

#define CPS_FLD_MASK(fld)  (fld ## _MASK)

#define CPS_FLD_READ(fld, reg_value) (FIELD_GET((CPS_FLD_MASK(fld)), (uint32_t)(reg_value)))

#define CPS_FLD_WRITE(fld, reg_value, value) \
	({ \
		(reg_value & ~(CPS_FLD_MASK(fld))) | FIELD_PREP(CPS_FLD_MASK(fld), value); \
	})

#define CPS_FLD_SET(fld, reg_value) \
	({ \
		(reg_value | CPS_FLD_MASK(fld)); \
	})

#endif /* CPS_DRV_LPDDR4_H */
