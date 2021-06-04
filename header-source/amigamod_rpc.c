// amigamod_rpc.cpp

#include <tamtypes.h>
#include <kernel.h>
#include <sifcmd.h>
#include <sifrpc.h>
#include <string.h>
#include "amigamod.h"
#include <iopheap.h>

static SifRpcClientData_t amodCd __attribute__((aligned(64)));
static unsigned sbuff[64] __attribute__((aligned (64)));

static int ammodi = 0;
static void *iopmodimg = 0;

int amigaModInit(int nosdinit)
{
	if (ammodi)
		return 0;

	if (SifBindRpc( &amodCd, VZMOD, 0) < 0)
		return -1;

	if (!nosdinit)
	{
		char hi[16] = "amigaModInit !";
		memcpy((char *)sbuff, hi, 16);
		SifCallRpc( &amodCd, MOD_INIT, 0, (void *)(&sbuff[0]), 16, (void *)(&sbuff[0]), 64, 0, 0);
	}

	ammodi = 1;
	return 0;
}

// Only call when playback is paused
int amigaModLoad( void *moddata, int size )
{
	int i;

	SifDmaTransfer_t sdt;

	if (SifInitIopHeap() != 0)
		return -1;

	if (iopmodimg)
		SifFreeIopHeap(iopmodimg);
	iopmodimg = SifAllocIopHeap(size);

	// transfer the moddata via dma into spu RAM
	sdt.src = (void*)moddata;
	sdt.dest = iopmodimg;
	sdt.size = size;
	sdt.attr = 0;
	i = SifSetDma(&sdt, 1);
	while (SifDmaStat(i) >= 0); // potential for infinite loop ...

	sbuff[0] = (int)iopmodimg;
	SifCallRpc( &amodCd, MOD_LOAD, 0, (void *)(&sbuff[0]), 4, (void *)(&sbuff[0]), 64, 0, 0);

	return sbuff[0];
}

int amigaModPlay(unsigned linear)
{
	sbuff[0] = linear;
	SifCallRpc( &amodCd, MOD_PLAY, 0, (void *)(&sbuff[0]), 4, (void *)(&sbuff[0]), 16, 0, 0);
	return sbuff[0];
	//return 0;
}

// Doesn't pause right away; It is advised to wait until 1 vsync before calling another amigamod function
int amigaModPause()
{
	SifCallRpc( &amodCd, MOD_PAUSE, 1, (void *)(&sbuff[0]), 4, (void *)(&sbuff[0]), 0, 0, 0);
	return 0;
}

// Volume between 0 and 0x3fff
int amigaModSetVolume( unsigned short volume)
{
	sbuff[0] = volume & 0x3fff;
	SifCallRpc( &amodCd, MOD_SETVOL, 0, (void *)(&sbuff[0]), 4, (void *)(&sbuff[0]), 0, 0, 0);
	return 0;
}

int amigaModGetInfo( ModInfoStruct *info)
{
	SifCallRpc( &amodCd, MOD_GETINFO, 0, (void *)(&sbuff[0]), 4, (void *)(&sbuff[0]), 64, 0, 0);
	info->curorder = sbuff[1];
	info->currow = sbuff[2];
	info->numchannels = sbuff[3];
	info->bpm = sbuff[4];
	return 0;
}

int amigaModQuit()
{
	SifCallRpc( &amodCd, MOD_QUIT, 1, (void *)(&sbuff[0]), 4, (void *)(&sbuff[0]), 0, 0, 0);
	return 0;
}

void ModPuts(char *s)
{
	memcpy((char *)sbuff, s, 252);
	SifCallRpc( &amodCd, MOD_PUTS, 1, (void *)(&sbuff[0]), 252, (void *)(&sbuff[0]), 0, 0, 0);
}