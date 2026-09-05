#
# Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

LZMA_PATH	:=	lib/lzma

# Enable verbose LZMA debug logs (default: off).
# Build with LZMA_DBG=1, e.g.: LZMA_DBG=1 SOC=an7583 ./build.sh bl2
ifeq ($(LZMA_DBG),1)
$(eval $(call add_define,LZMA_DBG))
endif

# Imported from zlib 1.2.11 (do not modify them)
LZMA_SOURCES	:=	$(addprefix $(LZMA_PATH)/,	\
					LzmaDec.c)

# Implemented for TF

INCLUDES	+=	-Iinclude/lib/lzma

# REVISIT: the following flags need not be given globally
LZMA_SOURCES	+=	$(addprefix $(LZMA_PATH)/,	\
					LzmaTools.c)
