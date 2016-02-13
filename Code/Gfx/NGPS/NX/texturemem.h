#ifndef __TEXTUREMEM_H
#define __TEXTUREMEM_H


namespace NxPs2
{

class TextureMemoryLayout {
public:

    ///
    enum {
        MAX_PIXEL_SIZE_BITS = 10
    };

    
    /// PIXEL_16S won't work with the algorithms here because
    /// of it's wierd layout.  Hopefully we won't need it.
    
    /// Pixel type.  Will add more types later when needed.
    enum PixelType {
        PIXEL_INVALID = -1,
        PIXEL_32 = 0,       // Covers 32 and 24 bit
        PIXEL_16,
        PIXEL_8,
        PIXEL_4,
        NUM_PIXEL_TYPES
    };
    
    ///
    struct TexSize {
        int width;
        int height;
    };

    ///
    struct PixelFormatLayout {
        TexSize pageSize;
        TexSize blockSize;
        
        bool adjacentBlockWidth;  // if not width, then height
    };

    ///
    typedef TexSize MinSizeArray[MAX_PIXEL_SIZE_BITS][MAX_PIXEL_SIZE_BITS];
    
    ///
    TextureMemoryLayout();

    ///
    void getMinSize(int& width, int& height, int pixMode);
    ///
    int getImageSize(int width, int height, int pixMode);
    ///
    int getBlockNumber(int blockX, int blockY, int pixMode);
    ///
    int getBlockOffset(int bufferWidth, int posX, int posY, int pixMode);
    ///
    void getPosFromBlockNumber(int& posX, int& posY, int blockNumber, int pixMode);
    ///
    void getPosFromBlockOffset(int& posX, int& posY, int blockOffset, int bufferWidth, int pixMode);
    ///
    bool isPageSize(int width, int height, int pixMode);
    ///
    bool isBlockSize(int width, int height, int pixMode);
    ///
    bool isMinPageSize(int width, int height, int pixMode);
    ///
    bool isMinBlockSize(int width, int height, int pixMode);

	// Converts memory layout (not format)
    int BlockConv4to32(uint8 *p_input, uint8 *p_output);
	int BlockConv8to32(uint8 *p_input, uint8 *p_output);
	int PageConv4to32(int width, int height, uint8 *p_input, uint8 *p_output);
	int PageConv8to32(int width, int height, uint8 *p_input, uint8 *p_output);
	bool Conv4to32(int width, int height, uint8 *p_input, uint8 *p_output);
	bool Conv8to32(int width, int height, uint8 *p_input, uint8 *p_output);
	bool Conv4to16(int width, int height, uint8 *p_input, uint8 *p_output);

	bool CanConv4to32(int width, int height);
	bool CanConv8to32(int width, int height);
	bool CanConv4to16(int width, int height);

    ///
    static int numBits(unsigned int size);
    ///
    static PixelType modeToPixelType(int pMode);

private:
    ///
    MinSizeArray minSize[NUM_PIXEL_TYPES];

    ///
    static const PixelFormatLayout pixelFormat[NUM_PIXEL_TYPES];
    ///
    static const int blockArrangement[4][8];

    ///
    void initMinSize(MinSizeArray& mArray, PixelType pType);
    ///
    void setMinSize(int& width, int& height, PixelType pType);

};

} // namespace NxPs2


#endif // __TEXTUREMEM_H

