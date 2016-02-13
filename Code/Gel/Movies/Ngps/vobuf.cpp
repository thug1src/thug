#include <eekernel.h>
#include "gel/movies/ngps/defs.h"
#include "gel/movies/ngps/vobuf.h"

namespace Flx
{




// ////////////////////////////////////////////////////////////////
//
// Functions called from decoding thread
//
void voBufCreate(
    VoBuf *f,
    VoData *data,
    VoTag *tag,
    int size
)
{
    int i;

    f->data = data;
    f->tag = tag;
    f->size = size;
    f->count = 0;
    f->write = 0;

    for (i = 0; i < size; i++) {
        f->tag[i].status = VOBUF_STATUS_;
    }
}

void voBufDelete(VoBuf *f)
{
}

void voBufReset(VoBuf *f)
{
    f->count = 0;
    f->write = 0;
}

int voBufIsFull(VoBuf *f)
{
    return f->count == f->size;
}

void voBufIncCount(VoBuf *f)
{
    // disable interrupt
    DI();

    f->tag[f->write].status = VOBUF_STATUS_FULL;
    f->count++;
    f->write = (f->write + 1) % f->size;

    // enable interrupt
    EI();
}

VoData *voBufGetData(VoBuf *f)
{
    return voBufIsFull(f)? (VoData*)NULL: f->data + f->write;
}

// ////////////////////////////////////////////////////////////////
//
// Functions called from interrupt handler
//
int voBufIsEmpty(VoBuf *f)
{
    return f->count == 0;
}

VoTag *voBufGetTag(VoBuf *f)
{
    return voBufIsEmpty(f)? (VoTag*)NULL:
    	f->tag + ((f->write - f->count + f->size) % f->size);
}

void voBufDecCount(VoBuf *f)
{
    if (f->count > 0) {
	f->count--;
    }
}

} // namespace Flx
