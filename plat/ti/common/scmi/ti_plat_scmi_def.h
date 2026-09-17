/*
 * Copyright (C) 2026 Texas Instruments Incorporated - https://www.ti.com/
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_SCMI_DEF_H
#define PLAT_SCMI_DEF_H

/*
 * Initializes the SCMI server for TI SoCs. Sets up the necessary
 * infrastructure to handle requests; called during system initialization.
 */
void ti_init_scmi_server(void);

#endif /* PLAT_SCMI_DEF_H */
