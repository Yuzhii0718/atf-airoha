/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef ARM_GPT_PARTITION_GUID_H
#define ARM_GPT_PARTITION_GUID_H

#include <drivers/partition/efi.h>

/*
 * Platforms can override these GUIDs (for example in platform_def.h) to use
 * GPT partition GUIDs for lookup. The defaults are NULL_GUID so that the
 * common code can fall back to partition names when GUIDs are not provided.
 */
#ifndef ARM_GPT_FWU_METADATA_TYPE_GUID
#define ARM_GPT_FWU_METADATA_TYPE_GUID		\
	EFI_GUID(0x8a7a84a0U, 0x8387U, 0x40f6U, 0xabU, 0x41U, \
		 0xa8U, 0xb9U, 0xa5U, 0xa6U, 0x0dU, 0x23U)
#endif

#ifndef ARM_FIP_IMG_TYPE_GUID
#define ARM_FIP_IMG_TYPE_GUID			\
	EFI_GUID(0x78c312e5U, 0x9fccU, 0x491aU, 0xa7U, 0x8fU, \
		 0xd2U, 0x68U, 0x9cU, 0x35U, 0x9eU, 0x22U)
#endif

#endif /* ARM_GPT_PARTITION_GUID_H */
