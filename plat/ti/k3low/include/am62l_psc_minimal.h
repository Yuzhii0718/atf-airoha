/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Minimal PSC driver for AM62L BL1.
 *
 * Provides a lightweight set_main_psc_state() for use during BL1 before the
 * full ti-am62l-clk driver is available.  Once that driver is integrated this
 * header (and its companion .c) should be retired in favour of the shared PSC
 * API.
 *
 * Copyright (C) 2025-2026 Texas Instruments Incorporated - https://www.ti.com/
 */

#ifndef AM62L_PSC_MINIMAL_H
#define AM62L_PSC_MINIMAL_H

#include <stdint.h>

#include <lib/utils_def.h>

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

/* PSC module / power-domain next-state values */
#define PSC_MD_SWRESETDISABLE	(0x0U)
#define PSC_MD_ENABLE		(0x3U)
#define PSC_PD_OFF		(0x0U)
#define PSC_PD_ON		(0x1U)

/* Main PSC register base addresses */
#define MAIN_PSC_BASE		UL(0x00400000)
#define MAIN_PSC_MDCTL_BASE	UL(0x00400A00)
#define MAIN_PSC_MDSTAT_BASE	UL(0x00400800)
#define MAIN_PSC_PDCTL_BASE	UL(0x00400300)
#define MAIN_PSC_PDSTAT_BASE	UL(0x00400200)

#define PSC_PTCMD		0x120
#define PSC_PTSTAT		0x128

#define MAIN_PSC_PTSTAT		(MAIN_PSC_BASE + PSC_PTSTAT)
#define MAIN_PSC_PTCMD		(MAIN_PSC_BASE + PSC_PTCMD)

/* PSC register field masks */
#define MDSTAT_STATE_MASK	GENMASK(5, 0)
#define PDSTAT_STATE_MASK	GENMASK(5, 0)
#define PTSTAT_DOMAIN_GOSTAT	0x1U
#define PDCTL_NEXT_STATE_MASK	GENMASK(0, 0)
#define MDCTL_NEXT_STATE_MASK	GENMASK(5, 0)
#define PTCMD_DOMAIN_GO		0x1U
#define PSC_GOSTAT_RETRY_COUNT	100000U

int set_main_psc_state(uint32_t pd_id, uint32_t md_id,
		       uint32_t pd_state, uint32_t md_state);

#endif /* AM62L_PSC_MINIMAL_H */
