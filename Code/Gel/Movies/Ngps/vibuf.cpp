#include <stdio.h>
#include <sifdev.h>
#include "gel/movies/ngps/defs.h"
#include "gel/movies/ngps/vibuf.h"
#include "gel/movies/ngps/p_movies.h"

// ////////////////////////////////////////////////////////////////
//
// Definitions 
//
#define DMA_ID_REFE	0
#define DMA_ID_NEXT	2
#define DMA_ID_REF	3

#define WAITSEMA(v) WaitSema(v)
#define SIGNALSEMA(v) SignalSema(v)

#define REST	2

#define TAG_ADDR(i)	((u_int)DmaAddr(f->tag + i))
#define DATA_ADDR(i)	((u_int)f->data + VIBUF_ELM_SIZE * (i))
#define WRAP_ADDR(addr) ((u_int)(f->data)     + (((u_int)(addr) - (u_int)(f->data)) % (VIBUF_ELM_SIZE * f->n)))

#define IsInRegion(i,start,len,n)  (     (0 <= (((i) + (n) - (start)) % (n))) &&     ((((i) + (n) - (start)) % (n)) < (len)))

#define FS(f)	((f->dmaStart + f->dmaN) * VIBUF_ELM_SIZE)
#define FN(f)	((f->n - REST -  f->dmaN) * VIBUF_ELM_SIZE)

namespace Flx
{




// ////////////////////////////////////////////////////////////////
//
// definition of QWORD
//
typedef union {
    u_long128	q;
    u_long 	l[2];
    u_int  	i[4];
    u_short	s[8];
    u_char	c[16];
} QWORD;

extern inline int IsPtsInRegion(int tgt, int pos, int len, int size)
{
    int tgt1 = (tgt + size - pos) % size;
    return tgt1 < len;
}

int getFIFOindex(ViBuf *f, void *addr)
{
    if (addr == DmaAddr(f->tag + (f->n + 1))) {
	return 0;
    } else {
	return ((u_int)addr - (u_int)f->data) / VIBUF_ELM_SIZE;
    }
}

void setD3_CHCR(u_int val)
{
    DI();
    *D_ENABLEW = ((*D_ENABLER)|0x00010000);	// pause all channels
    *D3_CHCR = val;
    *D_ENABLEW = ((*D_ENABLER)&~0x00010000);	// restart all channels
    EI();
}

void setD4_CHCR(u_int val)
{
    DI();
    *D_ENABLEW = ((*D_ENABLER)|0x00010000);	// pause all channels
    *D4_CHCR = val;
    *D_ENABLEW = ((*D_ENABLER)&~0x00010000);	// restart all channels
    EI();
}

void scTag2(
    QWORD *q,
    void *addr,
    u_int id,
    u_int qwc
)
{
    q->l[0] =
    	  (u_long)(u_int)addr << 32
    	| (u_long)id << 28
    	| (u_long)qwc << 0;
}

// ////////////////////////////////////////////////////////////////
//
// Create video input buffer
//
int viBufCreate(ViBuf *f,
    u_long128 *data, u_long128 *tag, int size,
    TimeStamp *ts, int n_ts)
{
    struct SemaParam param;

    f->data = data;
    f->tag = ( u_long128 * )UncAddr(tag);
    f->n = size;
    f->buffSize = size * VIBUF_ELM_SIZE;

    f->ts = ts;
    f->n_ts = n_ts;

    // ////////////////////////////////
    //
    // Create Semaphore
    //
    param.initCount = 1;
    param.maxCount = 1;
    f->sema = CreateSema(&param);

    // ////////////////////////////////
    //
    // Reset
    //
    viBufReset(f);

    f->totalBytes = 0;

    return TRUE;
}

// ////////////////////////////////////////////////////////////////
//
// Reset video input buffer
//
int viBufReset(ViBuf *f)
{
    int i;

    f->dmaStart = 0;
    f->dmaN = 0;
    f->readBytes = 0;
    f->isActive = TRUE;

    f->count_ts = 0;
    f->wt_ts = 0;
    for (i = 0; i < f->n_ts; i++) {
	f->ts[i].pts = TS_NONE;
	f->ts[i].dts = TS_NONE;
	f->ts[i].pos = 0;
	f->ts[i].len = 0;
    }

    // ////////////////////////////////
    //
    // Init DMA tags
    //
    for (i = 0; i < f->n; i++) {
    	scTag2(
	    (QWORD*)(f->tag + i),
	    DmaAddr((char*)f->data + VIBUF_ELM_SIZE * i),
	    DMA_ID_REF,
	    VIBUF_ELM_SIZE/16
	);
    }
    scTag2(
	(QWORD*)(f->tag + i),
	DmaAddr(f->tag),
	DMA_ID_NEXT,
	0
    );

    *D4_QWC = 0;
    *D4_MADR = (u_int)DmaAddr(f->data);
    *D4_TADR = (u_int)DmaAddr(f->tag);
    setD4_CHCR((0<<8) | (1<<2) | 1);

    return TRUE;
}

// ////////////////////////////////////////////////////////////////
//
// Get areas to put data
//
void viBufBeginPut(ViBuf *f,
	u_char **ptr0, int *len0, u_char **ptr1, int *len1)
{
	
    int es;
    int en;
    int fs;
    int fn;

    WAITSEMA(f->sema);

    fs = FS(f);
    fn = FN(f);

    es = (fs + f->readBytes) % f->buffSize;
    en = fn - f->readBytes;

    if (f->buffSize - es >= en) {	// area0
	*ptr0 = (u_char*)f->data + es;
    	*len0 = en;
	*ptr1 = NULL;
	*len1 = 0;
    } else {				// area0 + area1
	*ptr0 = (u_char*)f->data + es;
    	*len0 = f->buffSize - es;
	*ptr1 = (u_char*)f->data;
	*len1 = en - (f->buffSize - es);
    }

    SIGNALSEMA(f->sema);
}

// ////////////////////////////////////////////////////////////////
//
// Proceed inner pointer
//
void viBufEndPut(ViBuf *f, int size)
{
    WAITSEMA(f->sema);

    f->readBytes += size;
    f->totalBytes += size;

    SIGNALSEMA(f->sema);
}

// ////////////////////////////////////////////////////////////////
//
// Add bitstream data to DMA tag list
//
int viBufAddDMA(ViBuf *f)
{
    int i;
    int index;
    int id;
    int last;
    u_int d4chcr;
    int isNewData = 0;
    int consume;
    int read_start, read_n;

    WAITSEMA(f->sema);

    if (!f->isActive) {
	ErrMessage("DMA ADD not active\n");
        return FALSE;
    }

    // ////////////////////////////////////////
    //
    // STOP DMA ch4
    //
    // d4chcr:
    //	(1) DMA was running
    //	    ORIGINAL DMA tag
    //	(2) DMA was not running
    //	    REFE tag
    setD4_CHCR((DMA_ID_REFE<<28) | (0<<8) | (1<<2) | 1);
    d4chcr = *D4_CHCR;

    // ////////////////////////////////////////
    //
    // update dma pointer using D4_MADR
    //
    index = getFIFOindex(f, (void*)*D4_MADR);
    consume = (index + f->n - f->dmaStart) % f->n;
    f->dmaStart = (f->dmaStart + consume) % f->n;
    f->dmaN -= consume;

    // ////////////////////////////////////////
    //
    // update read pointer
    //
    read_start = (f->dmaStart + f->dmaN) % f->n;
    read_n = f->readBytes/VIBUF_ELM_SIZE;
    f->readBytes %= VIBUF_ELM_SIZE;

    // ////////////////////////////////////////
    //
    // the last REFE -> REF
    //
    if (read_n > 0) {
	last = (f->dmaStart + f->dmaN - 1 + f->n) % f->n;
	scTag2(
	    (QWORD*)(f->tag + last),
	    (char*)f->data + VIBUF_ELM_SIZE * last, 
	    DMA_ID_REF,
	    VIBUF_ELM_SIZE/16
	);
	isNewData = 1;
    }

    index = read_start;
    for (i = 0; i < read_n; i++) {
    	id = (i == read_n - 1)? DMA_ID_REFE: DMA_ID_REF;
	scTag2(
	    (QWORD*)(f->tag + index),
	    (char*)f->data + VIBUF_ELM_SIZE * index, 
	    id,
	    VIBUF_ELM_SIZE/16
	);
	index = (index + 1) % f->n;
    }

    f->dmaN += read_n;

    // ////////////////////////////////////////
    //
    // RESTART DMA ch4
    //
    if (f->dmaN) {
	if (isNewData) {
	    // change ref/refe ----> ref
	    d4chcr = (d4chcr & 0x0fffffff) | (DMA_ID_REF << 28);
	}
	setD4_CHCR(d4chcr | 0x100);
    }

    SIGNALSEMA(f->sema);

    return TRUE;
}

// ////////////////////////////////////////////////////////////////
//
// Stop DMA and save DMA environment
//
int viBufStopDMA(ViBuf *f)
{
    WAITSEMA(f->sema);

    f->isActive = FALSE;

    setD4_CHCR((0<<8) | (1<<2) | 1);		// STR: 0, DIR: 1

    f->env.d4madr = *D4_MADR;
    f->env.d4tadr = *D4_TADR;
    f->env.d4qwc =  *D4_QWC;
    f->env.d4chcr = *D4_CHCR;

    // wait till ofc becomes 0
    while (DGET_IPU_CTRL() & 0xf0)
	;

    // DMA ch3
    setD3_CHCR((0<<8) | 0);		// STR: 0, DIR: 0
    f->env.d3madr = *D3_MADR;
    f->env.d3qwc =  *D3_QWC;
    f->env.d3chcr = *D3_CHCR;

    f->env.ipubp = DGET_IPU_BP();
    f->env.ipuctrl = DGET_IPU_CTRL();

    SIGNALSEMA(f->sema);

    return TRUE;
}

// ////////////////////////////////////////////////////////////////
//
// Restore DMA environment and restart DMA
//
int viBufRestartDMA(ViBuf *f)
{
    int bp = f->env.ipubp & 0x7f;
    int fp = (f->env.ipubp >> 16) & 0x3;
    int ifc = (f->env.ipubp >> 8) & 0xf;
    u_int d4madr_next = f->env.d4madr - ((fp + ifc) << 4);
    u_int d4qwc_next = f->env.d4qwc + (fp + ifc);
    u_int d4tadr_next = f->env.d4tadr;
    u_int d4chcr_next = f->env.d4chcr | 0x100;
    int index;
    int index_next;
    int id;

    WAITSEMA(f->sema);

    //
    // check wrap around
    //
    if (d4madr_next < (u_int)f->data) {
	d4qwc_next = (DATA_ADDR(0) - d4madr_next) >> 4;
    	d4madr_next += (u_int)(f->n * VIBUF_ELM_SIZE);
	d4tadr_next = TAG_ADDR(0);
	id = (f->env.d4madr == DATA_ADDR(0)
		|| f->env.d4madr == DATA_ADDR(f->n))?
	    DMA_ID_REFE:
	    DMA_ID_REF;
	d4chcr_next = (f->env.d4chcr & 0x0fffffff)
				| (id << 28) | 0x100;

	if (!IsInRegion(0, f->dmaStart, f->dmaN, f->n)) {
	    f->dmaStart = f->n - 1;
	    f->dmaN++;
	}
    } else if ((index = getFIFOindex(f, (void*)f->env.d4madr))
    		!= (index_next = getFIFOindex(f, (void*)d4madr_next))) {
	d4tadr_next = TAG_ADDR(index);
	d4qwc_next = (DATA_ADDR(index) - d4madr_next) >> 4;
	id = (WRAP_ADDR(f->env.d4madr)
		== DATA_ADDR((f->dmaStart + f->dmaN) % f->n))?
	    DMA_ID_REFE:
	    DMA_ID_REF;
	d4chcr_next = (f->env.d4chcr & 0x0fffffff)
				| (id << 28) | 0x100;

	if (!IsInRegion(index_next, f->dmaStart, f->dmaN, f->n)) {
	    f->dmaStart = index_next;
	    f->dmaN++;
	}
    }

    // Restart DMA ch3
    if (f->env.d3madr && f->env.d3qwc) {
	*D3_MADR = f->env.d3madr;
	*D3_QWC  = f->env.d3qwc;
	setD3_CHCR(f->env.d3chcr | 0x100);
    }

    if (f->dmaN) {
	while (sceIpuIsBusy())
	    ;
	// BCLR
	sceIpuBCLR(bp);
	while (sceIpuIsBusy())
	    ;
    }

    // Restart DMA ch4
    *D4_MADR = d4madr_next;
    *D4_TADR = d4tadr_next;
    *D4_QWC  = d4qwc_next;
    if (f->dmaN) {
	setD4_CHCR(d4chcr_next);
    }

    *IPU_CTRL = f->env.ipuctrl;

    f->isActive = TRUE;

    SIGNALSEMA(f->sema);

    return TRUE;
}

// ////////////////////////////////////////////////////////////////
//
// Delete video input buffer
//
int viBufDelete(ViBuf *f)
{
    setD4_CHCR((0<<8) | (1<<2) | 1);	// STR: 0, chain, DIR: 1
    *D4_QWC = 0;
    *D4_MADR = 0;
    *D4_TADR = 0;

    DeleteSema(f->sema);
    return TRUE;
}

// ////////////////////////////////////////////////////////////////
//
// Check to see if decoder is in CSC period or not (0: CSC period)
//
int viBufIsActive(ViBuf *f)
{
    int ret;

    WAITSEMA(f->sema);

    ret = f->isActive;

    SIGNALSEMA(f->sema);

    return ret;
}

// ////////////////////////////////////////////////////////////////
//
// Data size in video input buffer
//
int viBufCount(ViBuf *f)
{
    int ret;

    WAITSEMA(f->sema);

    ret = f->dmaN * VIBUF_ELM_SIZE + f->readBytes;

    SIGNALSEMA(f->sema);

    return ret;
}

// ////////////////////////////////////////////////////////////////
//
// Flush video input buffer
//
void viBufFlush(ViBuf *f)
{
    WAITSEMA(f->sema);

    f->readBytes = bound(f->readBytes, VIBUF_ELM_SIZE);

    SIGNALSEMA(f->sema);
}

// ////////////////////////////////////////////////////////////////
//
// Add new time stamp and remove old one from the buffer
//
int viBufModifyPts(ViBuf *f, TimeStamp *new_ts)
{
    TimeStamp *ts;
    int rd = (f->wt_ts - f->count_ts + f->n_ts) % f->n_ts;
    int datasize =  VIBUF_ELM_SIZE * f->n;
    int loop = 1;

    if (f->count_ts > 0) {
	while (loop) {
	    ts = f->ts + rd;

	    if (ts->len == 0 || new_ts->len == 0) {
		break;
	    }

	    if (IsPtsInRegion(ts->pos, new_ts->pos, new_ts->len, datasize)) {
		int len = min(new_ts->pos + new_ts->len - ts->pos, ts->len);

		ts->pos = (ts->pos + len) % datasize;
		ts->len -= len;

		if (ts->len == 0) {
		    if (ts->pts >= 0) {
/*			ErrMessage("pts is not used");*/
			ts->pts = TS_NONE;
			ts->dts = TS_NONE;
			ts->pos = 0;
			ts->len = 0;
		    }
		    f->count_ts = max(f->count_ts - 1, 0);
		}
	    } else {
		loop = 0;
	    }

	    rd = (rd + 1) % f->n_ts;
	}
    }

    return 0;
}

// ////////////////////////////////////////////////////////////////
//
// Add new time stamp
//
int viBufPutTs(ViBuf *f, TimeStamp *ts)
{
    int ret = 0;

    WAITSEMA(f->sema);

    if (f->count_ts < f->n_ts) {

	viBufModifyPts(f, ts);

	if (ts->pts >= 0 || ts->dts >= 0) {

	    f->ts[f->wt_ts].pts = ts->pts;
	    f->ts[f->wt_ts].dts = ts->dts;
	    f->ts[f->wt_ts].pos = ts->pos;
	    f->ts[f->wt_ts].len = ts->len;

	    f->count_ts++;
	    f->wt_ts = (f->wt_ts + 1) % f->n_ts;
	}
	ret = 1;
    } 

    SIGNALSEMA(f->sema);

    return ret;
}
// ////////////////////////////////////////////////////////////////
//
// Get time stamp
//
int viBufGetTs(ViBuf *f, TimeStamp *ts)
{
    u_int d4madr = *D4_MADR;
    u_int ipubp = DGET_IPU_BP();
    int bp = f->env.ipubp & 0x7f;
    int fp = (ipubp >> 16) & 0x3;
    int ifc = (ipubp >> 8) & 0xf;
    u_int d4madr_next = d4madr - ((fp + ifc) << 4);
    u_int stop;
    int datasize =  VIBUF_ELM_SIZE * f->n;
    int isEnd = 0;
    int tscount;
    int wt;
    int i;

    WAITSEMA(f->sema);

    ts->pts = TS_NONE;
    ts->dts = TS_NONE;

    stop = (d4madr_next + (bp >> 3) + datasize - (u_int)f->data)
    							% datasize;

    tscount = f->count_ts;
    wt = f->wt_ts;

    for (i = 0; i < tscount && !isEnd; i++) {

	int rd = (wt - tscount + f->n_ts + i) % f->n_ts;

	if (IsPtsInRegion(stop,
		f->ts[rd].pos, f->ts[rd].len, datasize)) {

	    ts->pts = f->ts[rd].pts;
	    ts->dts = f->ts[rd].dts;
	    f->ts[rd].pts = TS_NONE;
	    f->ts[rd].dts = TS_NONE;

	    isEnd = 1;
	    f->count_ts -= min(1, f->count_ts);
	}
    }

    SIGNALSEMA(f->sema);

    return 1;
}

} // namespace Flx
