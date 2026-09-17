/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Cadence DDR Driver
 *
 * Copyright (C) 2012-2026 Cadence Design Systems, Inc.
 * Copyright (C) 2018-2026 Texas Instruments Incorporated - https://www.ti.com/
 */

#ifndef LPDDR4_STRUCTS_IF_H
#define LPDDR4_STRUCTS_IF_H

#include "inttypes.h"

#include "lpddr4_if.h"

/*
 * Both ti_lpddr4_config_s and ti_lpddr4_privatedata_s intentionally share the
 * same four members. This follows the upstream Cadence LPDDR4 driver API
 * convention: config holds caller-supplied settings passed to init(), while
 * privatedata is the driver's internal state populated during init(). Keeping
 * them as distinct types enforces the const/non-const ownership boundary in
 * the API and prevents callers from accidentally aliasing the two roles.
 */

/* Input configuration passed to ti_lpddr4_init(). */
struct ti_lpddr4_config_s {
	struct ti_lpddr4_ctlregs_s *ctlbase;
	lpddr4_infocallback infohandler;
	lpddr4_ctlcallback ctlinterrupthandler;
	lpddr4_phyindepcallback phyindepinterrupthandler;
};

/* Driver internal state populated by ti_lpddr4_init(). */
struct ti_lpddr4_privatedata_s {
	struct ti_lpddr4_ctlregs_s *ctlbase;
	lpddr4_infocallback infohandler;
	lpddr4_ctlcallback ctlinterrupthandler;
	lpddr4_phyindepcallback phyindepinterrupthandler;
};

#endif  /* LPDDR4_STRUCTS_IF_H */
