/*
 * Copyright (C) 2026 Texas Instruments Incorporated - https://www.ti.com/
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <common/runtime_svc.h>
#include <drivers/scmi-msg.h>
#include <tools_share/uuid.h>

#include <ti_sip_svc.h>

/* K3 SiP Service UUID */
DEFINE_SVC_UUID2(ti_sip_svc_uid,
		0x4fed2dcd, 0x1da1, 0x4b4a, 0xbd, 0x31,
		0x69, 0xc9, 0xa4, 0xec, 0xea, 0xfa);

/*
 * This function is responsible for handling all SiP calls from the NS world
 */
uintptr_t ti_sip_smc_handler(uint32_t smc_fid,
			    u_register_t x1,
			    u_register_t x2,
			    u_register_t x3,
			    u_register_t x4,
			    void *cookie,
			    void *handle,
			    u_register_t flags)
{
	switch (smc_fid) {
	case SIP_SVC_CALL_COUNT:
		/* Return the number of TI K3 SiP Service Calls. */
		SMC_RET1(handle,
			K3_COMMON_SIP_NUM_CALLS);

	case SIP_SVC_UID:
		/* Return UID to the caller */
		SMC_UUID_RET(handle, ti_sip_svc_uid);

	case SIP_SVC_VERSION:
		/* Return the version of current implementation */
		SMC_RET2(handle, K3_SIP_SVC_VERSION_MAJOR,
			K3_SIP_SVC_VERSION_MINOR);

	case K3_SIP_SCMI_AGENT0:
		/*
		 * The SMC is a doorbell to trigger SCMI message processing.
		 * Per the SCMI/SMT protocol (ARM DEN0056), the SCMI response
		 * status is written into the shared memory buffer, not returned
		 * via the SMC. The NS caller reads the SCMI status from
		 * smt_header::status and the protocol-level result from
		 * payload[0] in shared memory. SMC return value 0 signals
		 * only that the doorbell was delivered successfully.
		 */
		scmi_smt_fastcall_smc_entry(0);
		SMC_RET1(handle, 0);

	default:
		ERROR("%s: unhandled SMC (0x%x)\n", __func__, smc_fid);
		SMC_RET1(handle, SMC_UNK);
	}
}

/* Define a runtime service descriptor for fast SMC calls */
DECLARE_RT_SVC(
	k3_svc,
	OEN_SIP_START,
	OEN_SIP_END,
	SMC_TYPE_FAST,
	NULL,
	ti_sip_smc_handler
);
