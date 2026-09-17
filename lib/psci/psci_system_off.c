/*
 * Copyright (c) 2014-2026, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stddef.h>

#include <arch_helpers.h>
#include <common/debug.h>
#include <drivers/arm/gic.h>
#include <drivers/console.h>
#include <plat/common/platform.h>

#include "psci_private.h"

static void __dead2 system_power_down(void)
{
	/*
	 * Turn the GIC off after the platform has had powered other cores off
	 * but before caching has been disabled as part of powerdown prep.
	 */
#if USE_GIC_DRIVER
	unsigned int core_pos = plat_my_core_pos();
	gic_cpuif_disable(core_pos);
	gic_pcpu_off(core_pos);
#endif /* USE_GIC_DRIVER */

	psci_pwrdown_cpu_start((unsigned int)PLAT_MAX_PWR_LVL);
	psci_pwrdown_cpu_end_terminal();
}

void __dead2 psci_system_off(void)
{
	psci_print_power_domain_map();

	assert(psci_plat_pm_ops->system_off != NULL);

	/* Notify the Secure Payload Dispatcher */
	if ((psci_spd_pm != NULL) && (psci_spd_pm->svc_system_off != NULL)) {
		psci_spd_pm->svc_system_off();
	}

	console_flush();

	/* Call the platform specific hook */
	psci_plat_pm_ops->system_off();

	system_power_down();
}

void __dead2 psci_system_reset(void)
{
	psci_print_power_domain_map();

	assert(psci_plat_pm_ops->system_reset != NULL);

	/* Notify the Secure Payload Dispatcher */
	if ((psci_spd_pm != NULL) && (psci_spd_pm->svc_system_reset != NULL)) {
		psci_spd_pm->svc_system_reset();
	}

	console_flush();

	/* Call the platform specific hook */
	psci_plat_pm_ops->system_reset();

	system_power_down();
}

u_register_t psci_system_reset2(uint32_t reset_type, u_register_t cookie)
{
	unsigned int is_vendor;
	int ret;

	psci_print_power_domain_map();

	assert(psci_plat_pm_ops->system_reset2 != NULL);

	is_vendor = (reset_type >> PSCI_RESET2_TYPE_VENDOR_SHIFT) & 1U;
	if (is_vendor == 0U) {
		/*
		 * Only WARM_RESET is allowed for architectural type resets.
		 */
		if (reset_type != PSCI_RESET2_SYSTEM_WARM_RESET) {
			return (u_register_t) PSCI_E_INVALID_PARAMS;
		}
		if ((psci_plat_pm_ops->write_mem_protect != NULL) &&
		    (psci_plat_pm_ops->write_mem_protect(0) < 0)) {
			return (u_register_t) PSCI_E_NOT_SUPPORTED;
		}
	}

	/* Notify the Secure Payload Dispatcher */
	if ((psci_spd_pm != NULL) && (psci_spd_pm->svc_system_reset != NULL)) {
		psci_spd_pm->svc_system_reset();
	}
	console_flush();

	ret = psci_plat_pm_ops->system_reset2((int) is_vendor, reset_type, cookie);
	if (ret != PSCI_E_SUCCESS) {
		return (u_register_t) ret;
	}

	system_power_down();
}
