#include <core/defines.h>
#include "texture.h"
#include "texturemem.h"
#include "dma.h"
#include "vif.h"
#include "vu1.h"
#include "gif.h"
#include "gs.h"

#include <core/math/math.h>

#define min(x, y) (((x) > (y))? (y): (x))
#define max(x, y) (((x) < (y))? (y): (x))

namespace NxPs2
{

const TextureMemoryLayout::PixelFormatLayout TextureMemoryLayout::pixelFormat[NUM_PIXEL_TYPES] = {
    // PIXEL_32
    {   { 64, 32 },     // page size
        { 8, 8 },       // block size
        true       },   // adjacentBlockWidth
    // PIXEL_16
    {   { 64, 64 },     // page size
        { 16, 8 },      // block size
        false      },   // adjacentBlockWidth
    // PIXEL_8
    {   { 128, 64 },    // page size
        { 16, 16 },     // block size
        true       },   // adjacentBlockWidth
    // PIXEL_4
    {   { 128, 128 },   // page size
        { 32, 16 },     // block size
        false      },   // adjacentBlockWidth
};

const int TextureMemoryLayout::blockArrangement[4][8] = {
    {  0,  1,  4,  5, 16, 17, 20, 21 },
    {  2,  3,  6,  7, 18, 19, 22, 23 },
    {  8,  9, 12, 13, 24, 25, 28, 29 },
    { 10, 11, 14, 15, 26, 27, 30, 31 }
};

//----------------------------------------------------------------------------
//
TextureMemoryLayout::TextureMemoryLayout()
{
    for (int i = PIXEL_32; i < NUM_PIXEL_TYPES; i++)
        initMinSize(minSize[i], PixelType(i));
}

//----------------------------------------------------------------------------
//
void
TextureMemoryLayout::initMinSize(MinSizeArray& mArray, PixelType pType)
{
    int tw, th;

    for (th = 0; th < MAX_PIXEL_SIZE_BITS; th++) {
        for (tw = 0; tw < MAX_PIXEL_SIZE_BITS; tw++) {
            mArray[tw][th].width = 1 << tw;
            mArray[tw][th].height = 1 << th;

            setMinSize(mArray[tw][th].width, mArray[tw][th].height, pType);
        }
    }
}

//----------------------------------------------------------------------------
//
void
TextureMemoryLayout::setMinSize(int& width, int& height, PixelType pType)
{
    const PixelFormatLayout &pf = pixelFormat[pType];

    // All widths and heights are powers of 2

    // If texture dimension is bigger than a page dimension, then
    // make sure whole pages are allocated and exit.
    if (width > pf.pageSize.width) {
        height = Mth::Max(height, pf.pageSize.height);
        return;
    } else if (height > pf.pageSize.height) {
        width = Mth::Max(width, pf.pageSize.width);
        return;
    }

    // Make sure width and height are at least block size
    if (width < pf.blockSize.width)
        width = pf.blockSize.width;
    if (height < pf.blockSize.height)
        height = pf.blockSize.height;

#if 0
    // Now we are only dealing with sub-pages.  Make sure
    // sizes don't go beyond allowable aspect ratios
    
    TexSize twoBlock;

    if (pf.adjacentBlockWidth) {    // width
        twoBlock.width = pf.blockSize.width * 2;
        twoBlock.height = pf.blockSize.height;
    } else {                        // height
        twoBlock.width = pf.blockSize.width;
        twoBlock.height = pf.blockSize.height * 2;
    }
#endif

    // It just happens that, given a sub-page,
    // the height cannot be bigger than the
    // width and the width cannot be more than
    // double the height
    if (height > width)
        width = height;
    if (width > (height * 2))
        height = width / 2;
}

//----------------------------------------------------------------------------
//
TextureMemoryLayout::PixelType
TextureMemoryLayout::modeToPixelType(int pixMode)
{
    switch(pixMode) {
    case PSMCT32:
    case PSMCT24:
        return PIXEL_32;

    case PSMCT16:
        return PIXEL_16;

    case PSMT8:
        return PIXEL_8;

    case PSMT4:
        return PIXEL_4;

    default:
        Dbg_MsgAssert(0, ("TextureMemoryLayout: PixelMode %d not supported.", pixMode));
        return PIXEL_INVALID;
    }
}

//----------------------------------------------------------------------------
//
void
TextureMemoryLayout::getMinSize(int& width, int& height, int pixMode)
{
    PixelType pType = modeToPixelType(pixMode);

    // Now get the minimum size
    int tw = numBits(width) - 1;
    int th = numBits(height) - 1;

    width = minSize[pType][tw][th].width;
    height = minSize[pType][tw][th].height;
}

//----------------------------------------------------------------------------
// Returns size in blocks.  Must calculate the minimum width and height first.
int
TextureMemoryLayout::getImageSize(int width, int height, int pixMode)
{
    PixelType pType = modeToPixelType(pixMode);
    const PixelFormatLayout &pf = pixelFormat[pType];
    int bw, bh;

    bw = width / pf.blockSize.width;
    bh = height / pf.blockSize.height;

    return bw * bh;
}

//----------------------------------------------------------------------------
//
int
TextureMemoryLayout::getBlockNumber(int blockX, int blockY, int pixMode)
{

    switch (pixMode) {
    case PSMCT32:
    case PSMCT24:
    case PSMT8:
        return blockArrangement[blockY][blockX];

    case PSMCT16:
    case PSMT4:
        return blockArrangement[blockX][blockY];

    default:
        Dbg_MsgAssert(0, ("TextureMemoryLayout: getBlockNumber %d not supported.", pixMode));
        return -1;
    }
}

//----------------------------------------------------------------------------
//
void
TextureMemoryLayout::getPosFromBlockNumber(int& posX, int& posY, int blockNumber, int pixMode)
{
    int blockRow = -1, blockCol = -1;

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 8; j++)
            if (blockArrangement[i][j] == blockNumber) {
                blockRow = i;
                blockCol = j;
                break;
            }

    if (blockRow < 0)
        Dbg_MsgAssert(0, ("TextureMemoryLayout: getPosFromBlockNumber"));

    PixelType pType = modeToPixelType(pixMode);
    const PixelFormatLayout &pf = pixelFormat[pType];

    switch (pixMode) {
    case PSMCT32:
    case PSMCT24:
    case PSMT8:
        posX = blockCol * pf.blockSize.width;
        posY = blockRow * pf.blockSize.height;
        break;

    case PSMCT16:
    case PSMT4:
        posX = blockRow * pf.blockSize.width;
        posY = blockCol * pf.blockSize.height;
        break;

    default:
        Dbg_MsgAssert(0, ("TextureMemoryLayout: getPosFromBlockNumber %d not supported.", pixMode));
    }
}

//----------------------------------------------------------------------------
//
int
TextureMemoryLayout::getBlockOffset(int bufferWidth, int posX, int posY, int pixMode)
{
    int horizPages, vertPages;
    int numPages = 0;
    int blockNumber = 0;
    
    PixelType pType = modeToPixelType(pixMode);
    const PixelFormatLayout &pf = pixelFormat[pType];

    // First figure out how many pages are above
    vertPages = posY / pf.pageSize.height;
    if (vertPages) {
        horizPages = bufferWidth / pf.pageSize.width;
        posY -= vertPages * pf.pageSize.height;
        numPages += vertPages * horizPages;
    }

    // Now how many are to the left
    horizPages = posX / pf.pageSize.width;
    if (horizPages) {
        posX -= horizPages * pf.pageSize.width;
        numPages += horizPages;
    }

    // Now figure out where we are within the page
    int horizBlocks, vertBlocks;
    horizBlocks = posX / pf.blockSize.width;
    vertBlocks = posY / pf.blockSize.height;
    blockNumber = getBlockNumber(horizBlocks, vertBlocks, pixMode);

    return (numPages * 32) + blockNumber;
}

//----------------------------------------------------------------------------
//
void
TextureMemoryLayout::getPosFromBlockOffset(int& posX, int& posY, int blockOffset, int bufferWidth, int pixMode)
{
    int horizPages, vertPages;
    
    PixelType pType = modeToPixelType(pixMode);
    const PixelFormatLayout &pf = pixelFormat[pType];

    posX = posY = 0;

    // First figure out how many pages are above
    int horizPageWidth = bufferWidth / pf.pageSize.width;
    if (horizPageWidth) {
        vertPages = blockOffset / (horizPageWidth * 32);
        if (vertPages) {
            posY += vertPages * pf.pageSize.height;
            blockOffset -= vertPages * horizPageWidth * 32;
        }
    }

    // Now how many are to the left
    horizPages = blockOffset / 32;
    if (horizPages) {
        posX += horizPages * pf.pageSize.width;
        blockOffset -= horizPages * 32;
    }

    // Now figure out where we are within the page
    int blockPosX, blockPosY;
    getPosFromBlockNumber(blockPosX, blockPosY, blockOffset, pixMode);
    
    posX += blockPosX;
    posY += blockPosY;
}

//----------------------------------------------------------------------------
//
bool
TextureMemoryLayout::isPageSize(int width, int height, int pixMode)
{
    PixelType pType = modeToPixelType(pixMode);
    const PixelFormatLayout &pf = pixelFormat[pType];

    return !(width % pf.pageSize.width) && !(height % pf.pageSize.height);
}

//----------------------------------------------------------------------------
//
bool
TextureMemoryLayout::isBlockSize(int width, int height, int pixMode)
{
    PixelType pType = modeToPixelType(pixMode);
    const PixelFormatLayout &pf = pixelFormat[pType];

    return !(width % pf.blockSize.width) && !(height % pf.blockSize.height);
}

//----------------------------------------------------------------------------
//
bool
TextureMemoryLayout::isMinPageSize(int width, int height, int pixMode)
{
    PixelType pType = modeToPixelType(pixMode);
    const PixelFormatLayout &pf = pixelFormat[pType];

    getMinSize(width, height, pixMode);

    return (width >= pf.pageSize.width) && (height >= pf.pageSize.height);
}

//----------------------------------------------------------------------------
//
bool
TextureMemoryLayout::isMinBlockSize(int width, int height, int pixMode)
{
    PixelType pType = modeToPixelType(pixMode);
    const PixelFormatLayout &pf = pixelFormat[pType];

    getMinSize(width, height, pixMode);

    return (width >= pf.blockSize.width) && (height >= pf.blockSize.height);
}

//----------------------------------------------------------------------------
//
int
TextureMemoryLayout::numBits(unsigned int size)
{
    int bits = 0;

    while (size > 0) {
        size >>= 1;
        bits++;
    }

    return bits;
}


/////////////////////////////////////////////////////////////
// GS Simulator code
//

uint32 *p_gsmem;
int gsmem_size = 0;

int block16[32] =
{
	 0,  2,  8, 10,
	 1,  3,  9, 11,
	 4,  6, 12, 14,
	 5,  7, 13, 15,
	16, 18, 24, 26,
	17, 19, 25, 27,
	20, 22, 28, 30,
	21, 23, 29, 31
};

int columnWord16[32] = 
{
	 0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,
	 2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15
};

int columnHalf16[32] =
{
	0, 0, 0, 0, 0, 0, 0, 0,  1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0,  1, 1, 1, 1, 1, 1, 1, 1
};


void readTexPSMCT16(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void *data)
{
	//dbw >>= 1;
	unsigned short *src = (unsigned short *)data;
	int startBlockPos = dbp * 64;

	for(int y = dsay; y < dsay + rrh; y++)
	{
		int pageY = y / 64;
		int py = y - (pageY * 64);
		int blockY = py / 8;
		int by = py - blockY * 8;

		int column = by / 2;
		int cy = by - column * 2;

		for(int x = dsax; x < dsax + rrw; x++)
		{
			int pageX = x / 64;
			int page  = pageX + pageY * dbw;

			int px = x - (pageX * 64);

			int blockX = px / 16;
			int block  = block16[blockX + blockY * 4];

			int bx = px - blockX * 16;

			int cx = bx;
			int cw = columnWord16[cx + cy * 16];
			int ch = columnHalf16[cx + cy * 16];

			int gs_index = startBlockPos + page * 2048 + block * 64 + column * 16 + cw;
			Dbg_MsgAssert(gs_index < gsmem_size, ("GS simulator memory size: %d; addressing: %d", gsmem_size, gs_index));
			unsigned short *dst = (unsigned short *)&p_gsmem[gs_index];
			*src = dst[ch];
			src++;
		}
	}
}

int block4[32] =
{
	 0,  2,  8, 10,
	 1,  3,  9, 11,
	 4,  6, 12, 14,
	 5,  7, 13, 15,
	16, 18, 24, 26,
	17, 19, 25, 27,
	20, 22, 28, 30,
	21, 23, 29, 31
};

int columnWord4[2][128] =
{
	{
		 0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,
		 2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15,

		 8,  9, 12, 13,  0,  1,  4,  5,   8,  9, 12, 13,  0,  1,  4,  5,   8,  9, 12, 13,  0,  1,  4,  5,   8,  9, 12, 13,  0,  1,  4,  5,
		10, 11, 14, 15,  2,  3,  6,  7,  10, 11, 14, 15,  2,  3,  6,  7,  10, 11, 14, 15,  2,  3,  6,  7,  10, 11, 14, 15,  2,  3,  6,  7
	},
	{
		 8,  9, 12, 13,  0,  1,  4,  5,   8,  9, 12, 13,  0,  1,  4,  5,   8,  9, 12, 13,  0,  1,  4,  5,   8,  9, 12, 13,  0,  1,  4,  5,
		10, 11, 14, 15,  2,  3,  6,  7,  10, 11, 14, 15,  2,  3,  6,  7,  10, 11, 14, 15,  2,  3,  6,  7,  10, 11, 14, 15,  2,  3,  6,  7,

		 0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,   0,  1,  4,  5,  8,  9, 12, 13,
		 2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15,   2,  3,  6,  7, 10, 11, 14, 15
	}
};

int columnByte4[128] =
{
	0, 0, 0, 0, 0, 0, 0, 0,  2, 2, 2, 2, 2, 2, 2, 2,  4, 4, 4, 4, 4, 4, 4, 4,  6, 6, 6, 6, 6, 6, 6, 6,
	0, 0, 0, 0, 0, 0, 0, 0,  2, 2, 2, 2, 2, 2, 2, 2,  4, 4, 4, 4, 4, 4, 4, 4,  6, 6, 6, 6, 6, 6, 6, 6,

	1, 1, 1, 1, 1, 1, 1, 1,  3, 3, 3, 3, 3, 3, 3, 3,  5, 5, 5, 5, 5, 5, 5, 5,  7, 7, 7, 7, 7, 7, 7, 7,
	1, 1, 1, 1, 1, 1, 1, 1,  3, 3, 3, 3, 3, 3, 3, 3,  5, 5, 5, 5, 5, 5, 5, 5,  7, 7, 7, 7, 7, 7, 7, 7
};

void writeTexPSMT4(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void *data)
{
	dbw >>= 1;
	unsigned char *src = (unsigned char *)data;
	int startBlockPos = dbp * 64;

	bool odd = false;

	for(int y = dsay; y < dsay + rrh; y++)
	{
		int pageY = y / 128;
		int py = y - (pageY * 128);
		int blockY = py / 16;
		int by = py - blockY * 16;

		int column = by / 4;
		int cy = by - column * 4;

		for(int x = dsax; x < dsax + rrw; x++)
		{
			int pageX = x / 128;
			int page  = pageX + pageY * dbw;

			int px = x - (pageX * 128);

			int blockX = px / 32;
			int block  = block4[blockX + blockY * 4];

			int bx = px - blockX * 32;

			int cx = bx;
			int cw = columnWord4[column & 1][cx + cy * 32];
			int cb = columnByte4[cx + cy * 32];

			int gs_index = startBlockPos + page * 2048 + block * 64 + column * 16 + cw;
			Dbg_MsgAssert(gs_index < gsmem_size, ("GS simulator memory size: %d; addressing: %d", gsmem_size, gs_index));
			unsigned char *dst = (unsigned char *)&p_gsmem[gs_index];

			if(cb & 1)
			{
				if(odd)
					dst[cb >> 1] = (dst[cb >> 1] & 0x0f) | ((*src) & 0xf0);
				else
					dst[cb >> 1] = (dst[cb >> 1] & 0x0f) | (((*src) << 4) & 0xf0);
			}
			else
			{
				if(odd)
					dst[cb >> 1] = (dst[cb >> 1] & 0xf0) | (((*src) >> 4) & 0x0f);
				else
					dst[cb >> 1] = (dst[cb >> 1] & 0xf0) | ((*src) & 0x0f);
			}

			if(odd)
				src++;

			odd = !odd;
		}
	}
}

// -------------------------------------------------------------------------------------------
//
//
int TextureMemoryLayout::BlockConv4to32(u_char *p_input, u_char *p_output) {

	static int lut[] = {
		// even column
		0, 68, 8,  76, 16, 84, 24, 92,
		1, 69, 9,  77, 17, 85, 25, 93,
		2, 70, 10, 78, 18, 86, 26, 94,
		3, 71, 11, 79, 19, 87, 27, 95,
		4, 64, 12, 72, 20, 80, 28, 88,
		5, 65, 13, 73, 21, 81, 29, 89,
		6, 66, 14, 74, 22, 82, 30, 90,
		7, 67, 15, 75, 23, 83, 31, 91,

		32, 100, 40, 108, 48, 116, 56, 124,
		33, 101, 41, 109, 49, 117, 57, 125,
		34, 102, 42, 110, 50, 118, 58, 126,
		35, 103, 43, 111, 51, 119, 59, 127,
		36, 96,  44, 104, 52, 112, 60, 120,
		37, 97,  45, 105, 53, 113, 61, 121,
		38, 98,  46, 106, 54, 114, 62, 122,
		39, 99,  47, 107, 55, 115, 63, 123,


		// odd column
		4, 64, 12, 72, 20, 80, 28, 88,
		5, 65, 13, 73, 21, 81, 29, 89,
		6, 66, 14, 74, 22, 82, 30, 90,
		7, 67, 15, 75, 23, 83, 31, 91,
		0, 68, 8,  76, 16, 84, 24, 92,
		1, 69, 9,  77, 17, 85, 25, 93,
		2, 70, 10, 78, 18, 86, 26, 94,
		3, 71, 11, 79, 19, 87, 27, 95,

		36, 96,  44, 104, 52, 112, 60, 120,
		37, 97,  45, 105, 53, 113, 61, 121,
		38, 98,  46, 106, 54, 114, 62, 122,
		39, 99,  47, 107, 55, 115, 63, 123,
		32, 100, 40, 108, 48, 116, 56, 124,
		33, 101, 41, 109, 49, 117, 57, 125,
		34, 102, 42, 110, 50, 118, 58, 126,
		35, 103, 43, 111, 51, 119, 59, 127
	};

	unsigned int i, j, k, i0, i1, i2;
	unsigned int index0, index1;
	unsigned char c_in, c_out, *pIn;

	pIn = p_input;

	// for first step, we only think for a single block. (4bits, 32x16)
	index1 = 0;

	for(k = 0; k < 4; k++) {
		index0 = (k % 2) * 128;

		for(i = 0; i < 16; i++) {

			for(j = 0; j < 4; j++) {

				c_out = 0x00;

				// lower 4bit.
				i0 = lut[index0++];
				i1 = i0 / 2;
				i2 = (i0 & 0x1) * 4;
				c_in = (pIn[i1] & (0x0f << i2)) >> i2;
				c_out = c_out | c_in;

				// uppper 4bit
				i0 = lut[index0++];
				i1 = i0 / 2;
				i2 = (i0 & 0x1) * 4;
				c_in = (pIn[i1] & (0x0f << i2)) >> i2;
				c_out = c_out | ((c_in << 4) & 0xf0);

				p_output[index1++] = c_out;
			}
		}
		pIn += 64;
	}

	return 0;
}


// -------------------------------------------------------------------------------------------
//
//
int TextureMemoryLayout::BlockConv8to32(u_char *p_input, u_char *p_output) {

	static int lut[] = {
		// even column
		0, 36, 8,  44,
		1, 37, 9,  45,
		2, 38, 10, 46,
		3, 39, 11, 47,
		4, 32, 12, 40,
		5, 33, 13, 41,
		6, 34, 14, 42,
		7, 35, 15, 43,

		16, 52, 24, 60,
		17, 53, 25, 61,
		18, 54, 26, 62,
		19, 55, 27, 63, 
		20, 48, 28, 56,
		21, 49, 29, 57,
		22, 50, 30, 58,
		23, 51, 31, 59,

		// odd column
		4, 32, 12, 40,
		5, 33, 13, 41,
		6, 34, 14, 42,
		7, 35, 15, 43,
		0, 36, 8,  44,
		1, 37, 9,  45,
		2, 38, 10, 46,
		3, 39, 11, 47,

		20, 48, 28, 56,
		21, 49, 29, 57,
		22, 50, 30, 58,
		23, 51, 31, 59,
		16, 52, 24, 60,
		17, 53, 25, 61,
		18, 54, 26, 62,
		19, 55, 27, 63
	};

	unsigned int i, j, k, i0;
	unsigned int index0, index1;
	unsigned char *pIn;

	pIn = p_input;

	// for first step, we only think for a single block. (4bits, 32x16)
	index1 = 0;

	for(k = 0; k < 4; k++) {

		index0 = (k % 2) * 64;

		for(i = 0; i < 16; i++) {
			for(j = 0; j < 4; j++) {
				i0 = lut[index0++];
				p_output[index1++] = pIn[i0];
			}
		}
		pIn += 64;
	}

	return 0;
}


// -------------------------------------------------------------------------------
// send page size 4bit texture and get each block
// 
//
//
#define PSMT4_BLOCK_WIDTH 32
#define PSMT4_BLOCK_HEIGHT 16

#define PSMCT32_BLOCK_WIDTH 8
#define PSMCT32_BLOCK_HEIGHT 8

int TextureMemoryLayout::PageConv4to32(int width, int height, u_char *p_input, u_char *p_output) {

	static u_int block_table4[] = {
		0,  2,  8, 10,
		1,  3,  9, 11,
		4,  6, 12, 14,
		5,  7, 13, 15,
		16, 18, 24, 26,
		17, 19, 25, 27,
		20, 22, 28, 30,
		21, 23, 29, 31
	};

	static u_int block_table32[] = {
		0,  1,  4,  5, 16, 17, 20, 21,
		2,  3,  6,  7, 18, 19, 22, 23,
		8,  9, 12, 13, 24, 25, 28, 29,
		10, 11, 14, 15, 26, 27, 30, 31
	};

	u_int *index32_h, *index32_v, in_block_nb;

	u_char input_block[16 * 16], output_block[16 * 16];
	u_char *pi0, *pi1, *po0, *po1;
	int index0, index1, i, j, k;
	int n_width, n_height, input_page_line_size;
	int output_page_line_size;





	// --- create table for output 32bit buffer ---
	index32_h = (u_int*) malloc(8 * 4 * sizeof(u_int));
	index32_v = (u_int*) malloc(8 * 4 * sizeof(u_int));
	index0 = 0;
	for(i = 0; i < 4; i++) {
		for(j = 0; j < 8; j++) {
			index1 = block_table32[index0];
			index32_h[index1] = j;
			index32_v[index1] = i;
			index0++;
		}
	}




	n_width = width / 32;
	n_height = height / 16;

	memset(input_block, 0, 16 *16);
	memset(output_block, 0, 16 * 16);

	input_page_line_size = 128 / 2;    // PSMT4 page width (byte)
	output_page_line_size = 256;       // PSMCT32 page width (byte)

	// now assume copying from page top. 
	for(i = 0; i < n_height; i++) {

		for(j = 0; j < n_width; j++) {

			pi0 = input_block;
			pi1 = p_input + 16 * i * input_page_line_size + j * 16;

			in_block_nb = block_table4[i * n_width + j];

			for(k = 0; k < PSMT4_BLOCK_HEIGHT; k++) {
				memcpy(pi0, pi1, PSMT4_BLOCK_WIDTH / 2); // copy full 1 line of 1 block.
				pi0 += PSMT4_BLOCK_WIDTH / 2;
				pi1 += input_page_line_size;
			}

			BlockConv4to32(input_block, output_block);

			po0 = output_block;
			po1 = p_output + 8 * index32_v[in_block_nb] * output_page_line_size + index32_h[in_block_nb] * 32;
			for(k = 0; k < PSMCT32_BLOCK_HEIGHT; k++) {
				memcpy(po1, po0, PSMCT32_BLOCK_WIDTH * 4);
				po0 += PSMCT32_BLOCK_WIDTH * 4;
				po1 += output_page_line_size;   
			}

		}
	}

	free(index32_h);
	free(index32_v);

	return 0;
}



// -------------------------------------------------------------------------------
// send page size 8bit texture and get each block
// 
//
//
#define PSMT8_BLOCK_WIDTH  16
#define PSMT8_BLOCK_HEIGHT 16



int TextureMemoryLayout::PageConv8to32(int width, int height, u_char *p_input, u_char *p_output) {

	static u_int block_table8[] = {
		0,  1,  4,  5, 16, 17, 20, 21,
		2,  3,  6,  7, 18, 19, 22, 23,
		8,  9, 12, 13, 24, 25, 28, 29,
		10, 11, 14, 15, 26, 27, 30, 31
	};

	static u_int block_table32[] = {
		0,  1,  4,  5, 16, 17, 20, 21,
		2,  3,  6,  7, 18, 19, 22, 23,
		8,  9, 12, 13, 24, 25, 28, 29,
		10, 11, 14, 15, 26, 27, 30, 31
	};

	u_int *index32_h, *index32_v, in_block_nb;

	u_char input_block[16 * 16], output_block[16 * 16];
	u_char *pi0, *pi1, *po0, *po1;
	int index0, index1, i, j, k;
	int n_width, n_height, input_page_line_size;
	int output_page_line_size;



	// --- create table for output 32bit buffer ---
	index32_h = (u_int*) malloc(8 * 4 * sizeof(u_int));
	index32_v = (u_int*) malloc(8 * 4 * sizeof(u_int));
	index0 = 0;
	for(i = 0; i < 4; i++) {
		for(j = 0; j < 8; j++) {
			index1 = block_table32[index0];
			index32_h[index1] = j;
			index32_v[index1] = i;
			index0++;
		}
	}


	// how many blocks we should calc (width/height)
	n_width = width / PSMT8_BLOCK_WIDTH;
	n_height = height / PSMT8_BLOCK_HEIGHT;

	memset(input_block, 0, 16 *16);
	memset(output_block, 0, 16 * 16);

	input_page_line_size  = 128;    // PSMT8 page width (byte)
	output_page_line_size = 256;    // PSMCT32 page width (byte)

	// now assume copying from page top. 
	for(i = 0; i < n_height; i++) {

		for(j = 0; j < n_width; j++) {

			pi0 = input_block;
			pi1 = p_input + PSMT8_BLOCK_HEIGHT * i * input_page_line_size + j * PSMT8_BLOCK_WIDTH; // byte

			in_block_nb = block_table8[i * n_width + j];

			for(k = 0; k < PSMT8_BLOCK_HEIGHT; k++) {
				memcpy(pi0, pi1, PSMT8_BLOCK_WIDTH); // copy full 1 line of 1 block.
				pi0 += PSMT8_BLOCK_WIDTH;  
				pi1 += input_page_line_size;
			}

			BlockConv8to32(input_block, output_block);

			po0 = output_block;
			po1 = p_output + 8 * index32_v[in_block_nb] * output_page_line_size + index32_h[in_block_nb] * 32;
			for(k = 0; k < PSMCT32_BLOCK_HEIGHT; k++) {
				memcpy(po1, po0, PSMCT32_BLOCK_WIDTH * 4);
				po0 += PSMCT32_BLOCK_WIDTH * 4;
				po1 += output_page_line_size;   
			}

		}
	}

	free(index32_h);
	free(index32_v);

	return 0;
}


// -------------------------------------------------------------
//
//

#define PSMT4_PAGE_WIDTH    128
#define PSMT4_PAGE_HEIGHT   128
#define PSMCT32_PAGE_WIDTH  64
#define PSMCT32_PAGE_HEIGHT 32

static bool sCanConvert4to32[10][10] =
{
#if 1	// This one only allows for dimensions that are easy to calculate the 32-bit dimensions
	//  1      2      4      8     16     32     64     128    256    512
	{ false, false, false, false, false, false, false, false, false, false },	// 1
	{ false, false, false, false, false, false, false, false, false, false },	// 2
	{ false, false, false, false, false, false, false, false, false, false },	// 4
	{ false, false, false, false, false, false , false, false, false, false },	// 8
	{ false, false, false, false, false, false , false, false, false, false },	// 16
	{ false, false, false, false, false, true , false, false, false, false },	// 32
	{ false, false, false, false, false, false, true , false, false, false },	// 64
	{ false, false, false, false, false, false, false, true , true , true  },	// 128
	{ false, false, false, false, false, false, false, true , true , true  },	// 256
	{ false, false, false, false, false, false, false, true , true , true  },	// 512
#else
	//  1      2      4      8     16     32     64     128    256    512
	{ false, false, false, false, false, false, false, false, false, false },	// 1
	{ false, false, false, false, false, false, false, false, false, false },	// 2
	{ false, false, false, false, false, false, false, false, false, false },	// 4
	{ false, false, false, false, false, true , false, false, false, false },	// 8
	{ false, false, false, false, false, true , false, false, false, false },	// 16
	{ false, false, false, false, false, true , true , false, false, false },	// 32
	{ false, false, false, false, false, true , true , true , false, false },	// 64
	{ false, false, false, false, false, false, true , true , true , true  },	// 128
	{ false, false, false, false, false, false, false, true , true , true  },	// 256
	{ false, false, false, false, false, false, false, true , true , true  },	// 512
#endif
};

bool TextureMemoryLayout::CanConv4to32(int width, int height)
{
    int tw = numBits(width) - 1;
    int th = numBits(height) - 1;

	return sCanConvert4to32[th][tw];
}

bool TextureMemoryLayout::Conv4to32(int width, int height, u_char *p_input, u_char *p_output) {


	int i, j, k;
	int n_page_h, n_page_w, n_page4_width_byte, n_page32_width_byte;
	u_char *pi0, *pi1, *po0, *po1;
	int n_input_width_byte, n_output_width_byte, n_input_height, n_output_height;
	u_char input_page[PSMT4_PAGE_WIDTH / 2 * PSMT4_PAGE_HEIGHT];
	u_char output_page[PSMCT32_PAGE_WIDTH * 4 * PSMCT32_PAGE_HEIGHT];

	#ifdef __NOPT_ASSERT__
	int output_buffer_size = width * height / 2;
	#endif

	// ----- check width -----
	for(i = 0; i < 11; i++) {
		if(width == (0x400 >> i)) break;
	}
	if(i == 11) {
		fprintf(stderr, "Error : width is not 2^n\n");
		return false;
	}

	//printf("input_page: %d\n", PSMT4_PAGE_WIDTH / 2 * PSMT4_PAGE_HEIGHT);
	//printf("output_page: %d\n", PSMCT32_PAGE_WIDTH * 4 * PSMCT32_PAGE_HEIGHT);

	memset(input_page, 0, PSMT4_PAGE_WIDTH / 2 * PSMT4_PAGE_HEIGHT);
	memset(output_page, 0, PSMCT32_PAGE_WIDTH * 4 * PSMCT32_PAGE_HEIGHT);

	// ----- check height -----
	for(i = 0; i < 11; i++) {
		if(height == (0x400 >> i)) break;
	}
	if(i == 11) {
		fprintf(stderr, "Error : width is not 2^n\n");
		return false;
	}


	n_page_w = (width - 1) / PSMT4_PAGE_WIDTH + 1;
	n_page_h = (height - 1) / PSMT4_PAGE_HEIGHT + 1;

	n_page4_width_byte = PSMT4_PAGE_WIDTH / 2;
	n_page32_width_byte = PSMCT32_PAGE_WIDTH * 4;

	//printf("n_page_w : %d\n", n_page_w );
	//printf("n_page_h : %d\n", n_page_h );

	//printf("n_page4_width_byte : %d\n", n_page4_width_byte );
	//printf("n_page32_width_byte : %d\n", n_page32_width_byte );


	// --- set in/out buffer size (for image smaller than one page) ---
	if(n_page_w == 1) {
		n_input_width_byte = width / 2;
		n_output_height = width / 4;
	} else {
		n_input_width_byte = n_page4_width_byte;
		n_output_height = PSMCT32_PAGE_HEIGHT;
	}

	if(n_page_h == 1) {
		n_input_height = height;
		n_output_width_byte = height * 2;
	} else {
		n_input_height = PSMT4_PAGE_HEIGHT;
		n_output_width_byte = n_page32_width_byte;
	}


	for(i = 0; i < n_page_h; i++) {
		for(j = 0; j < n_page_w; j++) {
			pi0 = p_input + (n_input_width_byte * PSMT4_PAGE_HEIGHT) * n_page_w * i 
				+ n_input_width_byte * j;
			pi1 = input_page;

			for(k = 0; k < n_input_height; k++) {
				memcpy(pi1, pi0, n_input_width_byte);
				pi0 += n_input_width_byte * n_page_w;
				pi1 += n_page4_width_byte;
			}

			PageConv4to32(PSMT4_PAGE_WIDTH, PSMT4_PAGE_HEIGHT, input_page, output_page);

			po0 = p_output + (n_output_width_byte * PSMCT32_PAGE_HEIGHT) * n_page_w * i
				+ n_output_width_byte * j;
			po1 = output_page;
			for(k = 0; k < n_output_height; k++) {
				Dbg_MsgAssert(((int) (po0 - p_output) + n_output_width_byte) <= output_buffer_size, ("Went outside output buffer for texture of size (%d, %d)", width, height));
				memcpy(po0, po1, n_output_width_byte);
				po0 += n_output_width_byte * n_page_w;
				po1 += n_page32_width_byte;
			}          
		}
	}

	return CanConv4to32(width, height);
}


// -------------------------------------------------------------
//
//

#define PSMT8_PAGE_WIDTH    128
#define PSMT8_PAGE_HEIGHT   64

static bool sCanConvert8to32[10][10] =
{
	//  1      2      4      8     16     32     64     128    256    512
	{ false, false, false, false, false, false, false, false, false, false },	// 1
	{ false, false, false, false, false, false, false, false, false, false },	// 2
	{ false, false, false, false, true , true , true , true , true , false },	// 4
	{ false, false, false, false, true , true , true , true , true , false },	// 8
	{ false, false, false, false, true , true , true , true , true , false },	// 16
	{ false, false, false, false, true , true , true , true , true , false },	// 32
	{ false, false, false, false, true , true , true , true , true , true  },	// 64
	{ false, false, false, false, false, false, false, true , true , true  },	// 128
	{ false, false, false, false, false, false, false, true , true , true  },	// 256
	{ false, false, false, false, false, false, false, true , true , false },	// 512
};

bool TextureMemoryLayout::CanConv8to32(int width, int height)
{
    int tw = numBits(width) - 1;
    int th = numBits(height) - 1;

	return sCanConvert8to32[th][tw];
}

bool TextureMemoryLayout::Conv8to32(int width, int height, u_char *p_input, u_char *p_output) {

	int i, j, k;
	int n_page_h, n_page_w, n_page8_width_byte, n_page32_width_byte;
	int n_input_width_byte, n_output_width_byte, n_input_height, n_output_height;
	u_char *pi0, *pi1, *po0, *po1;
	u_char input_page[PSMT8_PAGE_WIDTH * PSMT8_PAGE_HEIGHT];
	u_char output_page[PSMCT32_PAGE_WIDTH * 4 * PSMCT32_PAGE_HEIGHT];

	#ifdef __NOPT_ASSERT__
	int output_buffer_size = width * height;
	#endif
	
	// ----- check width -----
	for(i = 0; i < 11; i++) {
		if(width == (0x400 >> i)) break;
	}
	if(i == 11) {
		fprintf(stderr, "Error : width is not 2^n\n");
		return false;
	}

	// ----- check height -----
	for(i = 0; i < 11; i++) {
		if(height == (0x400 >> i)) break;
	}
	if(i == 11) {
		fprintf(stderr, "Error : width is not 2^n\n");
		return false;
	}

	memset(input_page, 0, PSMT8_PAGE_WIDTH * PSMT8_PAGE_HEIGHT);
	memset(output_page, 0, PSMCT32_PAGE_WIDTH * 4 * PSMCT32_PAGE_HEIGHT);

	n_page_w = (width - 1) / PSMT8_PAGE_WIDTH + 1;
	n_page_h = (height - 1) / PSMT8_PAGE_HEIGHT + 1;

	n_page8_width_byte = PSMT8_PAGE_WIDTH;
	n_page32_width_byte = PSMCT32_PAGE_WIDTH * 4;

	// --- set in/out buffer size (for image smaller than one page) ---
	if(n_page_w == 1) {
		n_input_width_byte = width;
		n_output_width_byte = width * 2;
	} else {
		n_input_width_byte = n_page8_width_byte;
		n_output_width_byte = n_page32_width_byte;
	}

	if(n_page_h == 1) {
		n_input_height = height;
		n_output_height = height / 2;
	} else {
		n_input_height = PSMT8_PAGE_HEIGHT;
		n_output_height = PSMCT32_PAGE_HEIGHT;
	}

	// --- conversion ---
	for(i = 0; i < n_page_h; i++) {
		for(j = 0; j < n_page_w; j++) {
			pi0 = p_input + (n_input_width_byte * PSMT8_PAGE_HEIGHT) * n_page_w * i 
				+ n_input_width_byte * j;
			pi1 = input_page;

			for(k = 0; k < n_input_height; k++) {
				memcpy(pi1, pi0, n_input_width_byte);
				pi0 += n_input_width_byte * n_page_w;
				pi1 += n_page8_width_byte;
			}

			// --- convert a page ---
			PageConv8to32(PSMT8_PAGE_WIDTH, PSMT8_PAGE_HEIGHT, input_page, output_page);

			po0 = p_output + (n_output_width_byte * n_output_height) * n_page_w * i
				+ n_output_width_byte * j;
			po1 = output_page;
			for(k = 0; k < n_output_height; k++) {
				Dbg_MsgAssert(((int) (po0 - p_output) + n_output_width_byte) <= output_buffer_size, ("Went outside output buffer for texture of size (%d, %d)", width, height));
				memcpy(po0, po1, n_output_width_byte);
				po0 += n_output_width_byte * n_page_w;
				po1 += n_page32_width_byte;
			}          
		}
	}

	return CanConv4to16(width, height);
}

// -------------------------------------------------------------
//
//

static bool sCanConvert4to16[10][10] =
{
	//  1      2      4      8     16     32     64     128    256    512
	{ false, false, false, false, false, false, false, false, false, false },	// 1
	{ false, false, false, false, false, false, false, false, false, false },	// 2
	{ false, false, false, false, false, false, false, false, false, false },	// 4
	{ false, false, false, false, true , true , true , true , true , false },	// 8
	{ false, false, false, false, true , true , true , true , true , false },	// 16
	{ false, false, false, false, true , true , true , true , true , false },	// 32
	{ false, false, false, false, true , true , true , true , true , false },	// 64
	{ false, false, false, false, true , true , true , true , true , true  },	// 128
	{ false, false, false, false, false, false, false, true , true , true  },	// 256
	{ false, false, false, false, false, false, false, true , true , true  },	// 512
};

bool TextureMemoryLayout::CanConv4to16(int width, int height)
{
    int tw = numBits(width) - 1;
    int th = numBits(height) - 1;

	return sCanConvert4to16[th][tw];
}

bool TextureMemoryLayout::Conv4to16(int width, int height, u_char *p_input, u_char *p_output) {


	int i;

	// ----- check width -----
	for(i = 0; i < 11; i++) {
		if(width == (0x400 >> i)) break;
	}
	if(i == 11) {
		fprintf(stderr, "Error : width is not 2^n\n");
		return false;
	}

	// ----- check height -----
	for(i = 0; i < 11; i++) {
		if(height == (0x400 >> i)) break;
	}
	if(i == 11) {
		fprintf(stderr, "Error : width is not 2^n\n");
		return false;
	}
	
	// Find size needed for buffers
	int min_4_width = width;
	int min_4_height = height;
	int min_16_width = width / 2;
	int min_16_height = height / 2;
	getMinSize(min_4_width, min_4_height, PSMT4);
	getMinSize(min_16_width, min_16_height, PSMCT16);

	int blocks_needed = max(getImageSize(min_4_width, min_4_height, PSMT4), getImageSize(min_16_width, min_16_height, PSMCT16));
	int words_needed = blocks_needed * 64;

	// Allocate memory
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	p_gsmem = new uint32[words_needed];
	gsmem_size = words_needed;
	Mem::Manager::sHandle().PopContext();

	// Swizzle through GS simulator code
	writeTexPSMT4(0, max(width / 128, 1) * 2, 0, 0, width, height, p_input);
	readTexPSMCT16(0, max(width / 2 / 128, 1) * 2, 0, 0, width / 2, height / 2, p_output);

	// Free memory
	delete [] p_gsmem;
	p_gsmem = NULL;
	gsmem_size = 0;

	return CanConv4to16(width, height);
}

} // namespace NxPs2

