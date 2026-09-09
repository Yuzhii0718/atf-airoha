
#include <LzmaTools.h>
#include <LzmaDec.h>

#include <string.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <lib/utils.h>

#define LZMA_PROPERTIES_OFFSET		0
#define LZMA_SIZE_OFFSET		LZMA_PROPS_SIZE
#define LZMA_DATA_OFFSET		(LZMA_SIZE_OFFSET + sizeof(uint64_t))

/* Verbose LZMA debug prints, controlled by the LZMA_DBG build flag
 * (e.g. LZMA_DBG=1 make / LZMA_DBG=1 ./build.sh). Disabled by default. */
#ifdef LZMA_DBG
#define LZMA_DBG_NOTICE(...) NOTICE(__VA_ARGS__)
#else
#define LZMA_DBG_NOTICE(...) ((void)0)
#endif

#define MAX_SIZE			0x580000
#define ZALLOC_ALIGNMENT		sizeof(void *)

static uintptr_t zalloc_start;
static uintptr_t zalloc_end;
static uintptr_t zalloc_current;
static void * MyAlloc(size_t size)
{
	uintptr_t p, p_end;

	p = round_up(zalloc_current, ZALLOC_ALIGNMENT);
	p_end = p + size;

	if (p_end > zalloc_end)
		return NULL;

	memset((void *)p, 0, size);

	zalloc_current = p_end;

	return (void *)p;
}

static void MyFree(void *address)
{

}

static void *SzAlloc(void *p, size_t size) { p = p; return MyAlloc(size); }
static void SzFree(void *p, void *address) { p = p; MyFree(address); }

int lzmaBuffToBuffDecompress(uintptr_t *inStream, size_t length, uintptr_t *outStream,
	   size_t uncompressedSize, uintptr_t work_buf, size_t work_len)
{
	unsigned int* p = (unsigned int*)(*outStream);
	int res = SZ_ERROR_DATA;
	int i = 0;

	ISzAlloc g_Alloc;
	SizeT outSize = 0;
	SizeT outSizeHigh = 0;
	SizeT outProcessed = MAX_SIZE;
	ELzmaStatus state;
	SizeT compressedSize = (SizeT)(length - LZMA_PROPS_SIZE);

	zalloc_start = work_buf;
	zalloc_end = work_buf + work_len;
	zalloc_current = zalloc_start;
	LZMA_DBG_NOTICE("LZMA DBG: in=0x%lx len=0x%zx out=0x%lx maxsz=0x%zx work=0x%lx worklen=0x%zx\n", *inStream, length, *outStream, uncompressedSize, work_buf, work_len);
	LZMA_DBG_NOTICE("LZMA DBG: src head=%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		((unsigned char *)*inStream)[0], ((unsigned char *)*inStream)[1], ((unsigned char *)*inStream)[2], ((unsigned char *)*inStream)[3],
		((unsigned char *)*inStream)[4], ((unsigned char *)*inStream)[5], ((unsigned char *)*inStream)[6], ((unsigned char *)*inStream)[7],
		((unsigned char *)*inStream)[8], ((unsigned char *)*inStream)[9], ((unsigned char *)*inStream)[10], ((unsigned char *)*inStream)[11],
		((unsigned char *)*inStream)[12]);
#ifdef LZMA_DBG
	{
		const unsigned char *s = (const unsigned char *)*inStream;
		unsigned int sum = 0;
		size_t k;

		/* Checksum of the whole compressed buffer: compare it against the
		 * same sum computed on the host to tell whether the data in the
		 * source memory is intact. */
		for (k = 0; k < length; k++)
			sum += s[k];

		NOTICE("LZMA DBG: src sum=0x%08x len=0x%zx\n", sum, length);
		NOTICE("LZMA DBG: src[13..28]=%02x %02x %02x %02x %02x %02x %02x %02x "
		       "%02x %02x %02x %02x %02x %02x %02x %02x\n",
			s[13], s[14], s[15], s[16], s[17], s[18], s[19], s[20],
			s[21], s[22], s[23], s[24], s[25], s[26], s[27], s[28]);

		/* Verify the destination buffer is really writable and that writes
		 * can be read back (the decoder reads dic[] while decoding).
		 * Byte and word accesses are checked separately: with the MMU off
		 * this SoC's DRAM port only corrupts the narrow ones. */
		{
			volatile unsigned char *o = (volatile unsigned char *)*outStream;
			volatile unsigned int *ow = (volatile unsigned int *)*outStream;
			unsigned int j;

			for (j = 0; j < 8; j++)
				o[j] = (unsigned char)(0x5a + j);
			NOTICE("LZMA DBG: dst byte test= %02x %02x %02x %02x %02x %02x %02x %02x (expect 5a 5b 5c 5d 5e 5f 60 61)\n",
				o[0], o[1], o[2], o[3], o[4], o[5], o[6], o[7]);

			ow[0] = 0x11223344u;
			ow[1] = 0x55667788u;
			NOTICE("LZMA DBG: dst word test= 0x%08x 0x%08x (expect 11223344 55667788)\n",
				ow[0], ow[1]);
		}

		/* The probability table is an array of UInt16 allocated from the
		 * work buffer, so make sure 16 bit accesses stick there. */
		{
			volatile unsigned short *w = (volatile unsigned short *)work_buf;

			w[0] = 0x55aa;
			w[1] = 0x1234;
			NOTICE("LZMA DBG: work rw test=0x%04x 0x%04x (expect 55aa 1234)\n",
				w[0], w[1]);
		}
	}
#endif
	INFO("LZMA: Image address............... 0x%lx\n", *inStream);
	INFO("LZMA: Properties address.......... 0x%lx\n", *inStream + LZMA_PROPERTIES_OFFSET);
	INFO("LZMA: Uncompressed size address... 0x%lx\n", *inStream + LZMA_SIZE_OFFSET);
	INFO("LZMA: Compressed data address..... 0x%lx\n", *inStream + LZMA_DATA_OFFSET);
	INFO("LZMA: Destination address......... 0x%lx\n", *outStream);
	memset(&state, 0, sizeof(state));

	INFO("LZMA: Uncompresed size............ 0x%zx\n", outProcessed);
	INFO("LZMA: Compresed size.............. 0x%zx\n", compressedSize);

	for (i = 0; i < 8; i++) {
		unsigned char b = ((unsigned char *) *inStream)[LZMA_SIZE_OFFSET + i];

		if (i < 4) {
			outSize     += (SizeT)(b) << (i * 8);
		} else {
			outSizeHigh += (SizeT)(b) << ((i - 4) * 8);
		}
	}

	if ((outSizeHigh != 0) || (outSize > outProcessed)) {
		LZMA_DBG_NOTICE("LZMA DBG: CHECK FAIL outSize=0x%zx outSizeHigh=0x%zx outProcessed=0x%zx\n", outSize, outSizeHigh, outProcessed);
		return SZ_ERROR_DATA;
	}

	outProcessed = outSize;

	g_Alloc.Alloc = SzAlloc;
	g_Alloc.Free = SzFree;

	res = LzmaDecode((Byte *) *outStream, &outProcessed,
			 ((Byte *) *inStream) + LZMA_DATA_OFFSET, &compressedSize,
			 ((Byte *) *inStream) , LZMA_PROPS_SIZE, LZMA_FINISH_END, &state, &g_Alloc);
	INFO("LZMA: Uncompresed ................ 0x%zx\n", outProcessed);

	VERBOSE("\033[33;1m 	decompressed data=> 0x%x-0x%x-0x%x-0x%x    \n\033[0m",*p,*(p+1), *(p+2), *(p+3));
	if (res != SZ_OK) {
		ERROR("LZMA: res %d state %d\n", res, state);
	}
	LZMA_DBG_NOTICE("LZMA DBG: res=%d state=%d outProcessed=0x%zx compressedUsed=0x%zx\n", res, state, outProcessed, compressedSize);

	*outStream = round_up(*outStream + outProcessed, sizeof(uintptr_t ));

	return res;
}
