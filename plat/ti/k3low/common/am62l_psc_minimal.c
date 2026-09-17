// SPDX-License-Identifier: BSD-3-Clause
/*
 * Minimal PSC driver for AM62L BL1.
 *
 * NOTE: This driver exists because the ti-am62l-clk driver (topic:
 * ti-am62l-clk) is not yet available during BL1 execution.  Once that driver
 * is integrated, this file should be removed and its callers updated to use
 * the shared PSC API to avoid duplicate hardware state management.
 *
 * Copyright (C) 2025-2026 Texas Instruments Incorporated - https://www.ti.com/
 */

#include <errno.h>

#include <common/debug.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#include <am62l_psc_minimal.h>

/*
 * Transition a single power domain / module-state pair on the Main PSC.
 *
 * Returns 0 on success, -ETIMEDOUT if the hardware fails to complete the
 * transition within PSC_GOSTAT_RETRY_COUNT polls.
 */
int set_main_psc_state(uint32_t pd_id, uint32_t md_id,
		       uint32_t pd_state, uint32_t md_state)
{
	uintptr_t mdctrl_ptr;
	volatile uint32_t mdctrl;
	uintptr_t mdstat_ptr;
	volatile uint32_t mdstat;
	uintptr_t pdctrl_ptr;
	volatile uint32_t pdctrl;
	uintptr_t pdstat_ptr;
	volatile uint32_t pdstat;
	volatile uint32_t psc_ptstat;
	volatile uint32_t psc_ptcmd;
	uint32_t retries;

	mdctrl_ptr = (uintptr_t)(MAIN_PSC_MDCTL_BASE + (4U * md_id));
	mdctrl = mmio_read_32(mdctrl_ptr);
	mdstat_ptr = (uintptr_t)(MAIN_PSC_MDSTAT_BASE + (4U * md_id));
	mdstat = mmio_read_32(mdstat_ptr);
	pdctrl_ptr = (uintptr_t)(MAIN_PSC_PDCTL_BASE + (4U * pd_id));
	pdctrl = mmio_read_32(pdctrl_ptr);
	pdstat_ptr = (uintptr_t)(MAIN_PSC_PDSTAT_BASE + (4U * pd_id));
	pdstat = mmio_read_32(pdstat_ptr);

	if (((FIELD_GET(PDSTAT_STATE_MASK, pdstat)) == pd_state) &&
			((FIELD_GET(MDSTAT_STATE_MASK, mdstat)) == md_state))
		return 0;

	psc_ptstat = mmio_read_32(MAIN_PSC_PTSTAT);

	/* wait for GOSTAT to get cleared */
	retries = PSC_GOSTAT_RETRY_COUNT;
	while ((psc_ptstat & (PTSTAT_DOMAIN_GOSTAT << pd_id)) != 0U) {
		if (retries == 0U) {
			ERROR("PSC: pd %u GOSTAT clear timed out\n", pd_id);
			return -ETIMEDOUT;
		}
		retries--;
		psc_ptstat = mmio_read_32(MAIN_PSC_PTSTAT);
	}

	/* Set PDCTL NEXT to new state */
	mmio_write_32(pdctrl_ptr,
		      ((pdctrl & ~(PDCTL_NEXT_STATE_MASK)) |
		      FIELD_PREP(PDCTL_NEXT_STATE_MASK, pd_state)));

	/* Set MDCTL NEXT to new state */
	mmio_write_32(mdctrl_ptr,
		      ((mdctrl & ~(MDCTL_NEXT_STATE_MASK)) |
		      FIELD_PREP(MDCTL_NEXT_STATE_MASK, md_state)));

	/* start power transition by setting PTCMD Go to 1 */
	psc_ptcmd = mmio_read_32((uintptr_t)MAIN_PSC_PTCMD);
	psc_ptcmd |= (PTCMD_DOMAIN_GO << pd_id);
	mmio_write_32((uintptr_t)MAIN_PSC_PTCMD, psc_ptcmd);

	/* wait for GOSTAT to get cleared */
	psc_ptstat = mmio_read_32((uintptr_t)MAIN_PSC_PTSTAT);
	retries = PSC_GOSTAT_RETRY_COUNT;
	while ((psc_ptstat & (PTSTAT_DOMAIN_GOSTAT << pd_id)) != 0U) {
		if (retries == 0U) {
			ERROR("PSC: pd %u GOSTAT clear timed out after PTCMD go\n", pd_id);
			return -ETIMEDOUT;
		}
		retries--;
		psc_ptstat = mmio_read_32((uintptr_t)MAIN_PSC_PTSTAT);
	}

	return 0;
}
