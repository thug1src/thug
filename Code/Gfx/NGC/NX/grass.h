//-----------------------------------------------------------------------------
// File: XBFur.h
//
// Copyright (c) 2000-2001 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
//#include "xfvf.h"
#include <gfx/Ngc/p_nxgeom.h>

//#define XBFUR_MAXSLICE_LOG2 5
//#define XBFUR_MAXSLICE (1 << XBFUR_MAXSLICE_LOG2)
//
//extern float g_fOneInch;
//
//#define FVF_XYZDIFF (D3DFVF_XYZ|D3DFVF_DIFFUSE)
//
//typedef struct sFVFT_XYZDIFF
//{
//    D3DVECTOR v;
//    DWORD diff;
//} FVFT_XYZDIFF;
//
//
//// patch generation
//
//// a fuzz is a single hair follicle, blade of grass, etc.
//struct Fuzz
//{
//    D3DVECTOR dp;            // velocity
//    D3DVECTOR ddp;            // acceleration
//    D3DXCOLOR colorBase;
//    D3DXCOLOR colorTip;
//};
//
//// a fuzz instance is a single instance of a fuzz
//// located at x, z on the patch
//// we create only a limited number of unique fuzzes
//// and index the library with lidx.
//struct FuzzInst
//{
//    float x, z;                // fuzz location
//    int lidx;                // library index
//};
//
//// a fur patch is a volume that holds fuzzes.
//// xsize and zsize are chosen by the user
//// ysize is calculated using the height of the 
//// tallest fuzz
//class CXBFur
//{
//    friend class CXBFurMesh;
//public:
//    DWORD m_dwSeed;            // patch seed
//    
//    float m_fXSize;            // patch size in world coords
//    float m_fYSize;
//    float m_fZSize;
//
//    // fuzz library
//    DWORD m_dwNumSegments;    // # of segments in highest LOD
//    Fuzz m_fuzzCenter;        // fuzz constant
//    Fuzz m_fuzzRandom;        // random offset around center
//    DWORD m_dwNumFuzzLib;    // # of fuzz in the library
//    Fuzz *m_pFuzzLib;        // fuzz library
//
//    // fuzz instances
//    DWORD m_dwNumFuzz;        // # of fuzz in this patch
//    FuzzInst *m_pFuzz;
//
//    // patch volume
//    DWORD m_dwNumSlices;    // # of layers in the volume
//    DWORD m_dwSliceSize;        // width*height
//    DWORD m_dwSliceXSize;        // width of volume texture slice
//    DWORD m_dwSliceZSize;        // height of volume texture slice
//    LPDIRECT3DTEXTURE8 m_apSliceTexture[XBFUR_MAXSLICE * 2 - 1];    // slices of volume texture
//                    // ... followed by level-of-detail textures  N/2, N/4, N/8, ... 1
//
//    // LOD textures
//    DWORD m_dwNumSlicesLOD; // number of slices in current level of detail
//    float m_fLevelOfDetail;    // current LOD value
//    DWORD m_iLOD;            // current integer LOD value
//    float m_fLODFraction;    // fraction towards next coarser level-of-detail
//    DWORD m_dwLODMax;        // maximum LOD index
//    LPDIRECT3DTEXTURE8 *m_pSliceTextureLOD; // current level of detail pointer into m_apSliceTexture array
//
//    // hair lighting texture
//    D3DMATERIAL8 m_HairLightingMaterial;
//    LPDIRECT3DTEXTURE8 m_pHairLightingTexture;
//
//    // fin texture
//    DWORD m_finWidth, m_finHeight;        // size of fin texture
//    float m_fFinXFraction, m_fFinZFraction;    // portion of hair texture to put into fin
//    LPDIRECT3DTEXTURE8 m_pFinTexture;    // texture projected from the side
//
//    CXBFur();
//    ~CXBFur();
//    void InitFuzz(DWORD nfuzz, DWORD nfuzzlib);
//    void GenSlices(DWORD nslices, DWORD slicexsize, DWORD slicezsize);
//    void GenFin(DWORD finWidth, DWORD finHeight, float fFinXFraction, float fFinZFraction);
//    void GetLinesVertexBuffer(IDirect3DVertexBuffer8 **ppVB);
//    void RenderLines();
//    void Save(char *fname, int flags);
//    void Load(char *fname);
//    HRESULT SetHairLightingMaterial(D3DMATERIAL8 *pMaterial);
//    void SetPatchSize(float x, float z)
//    {
//        m_fXSize = x;
//        m_fZSize = z;
//        InitFuzz(m_dwNumFuzz, m_dwNumFuzzLib);    // re-init the fuzz. automatically sets ysize
//    };
//    void SetFVel(float cx, float cy, float cz, float rx, float ry, float rz)
//    {
//        m_fuzzCenter.dp.x = cx; m_fuzzCenter.dp.y = cy; m_fuzzCenter.dp.z = cz;
//        m_fuzzRandom.dp.x = rx; m_fuzzRandom.dp.y = ry; m_fuzzRandom.dp.z = rz;
//    };
//    void SetFAcc(float cx, float cy, float cz, float rx, float ry, float rz)
//    {
//        m_fuzzCenter.ddp.x = cx; m_fuzzCenter.ddp.y = cy; m_fuzzCenter.ddp.z = cz;
//        m_fuzzRandom.ddp.x = rx; m_fuzzRandom.ddp.y = ry; m_fuzzRandom.ddp.z = rz;
//    };
//
//    // fLevelOfDetail can range from 0 to log2(NumSlices)
//    HRESULT SetLevelOfDetail(float fLevelOfDetail);
//    HRESULT ComputeLevelOfDetailTextures();
//    inline UINT LevelOfDetailCount(UINT iLOD)
//    {
//        return m_dwNumSlices >> iLOD;
//    }
//    inline UINT LevelOfDetailIndex(UINT iLOD)
//    {
//        UINT offset = 0;
//        for (UINT i = 1; i <= iLOD; i++)
//            offset += LevelOfDetailCount(i-1);
//        return offset;
//    }
//    inline UINT TotalTextureCount()
//    {
//        UINT TextureCount = 0;
//        for (UINT iLOD = 0; m_dwNumSlices >> iLOD; iLOD++)
//            TextureCount += LevelOfDetailCount(iLOD);
//        return TextureCount;
//    }
//
//    // Compress textures one at a time until all are done.
//    // Returns S_OK when all the textures are in fmtNew format.
//    // Returns S_FALSE if there are textures still to be done.
//    HRESULT CompressNextTexture(D3DFORMAT fmtNew, UINT *pTextureIndex);
//};
//
//HRESULT FillHairLightingTexture(D3DMATERIAL8 *pMaterial, LPDIRECT3DTEXTURE8 pTexture);
bool AddGrass( Nx::CNgcGeom *p_geom, NxNgc::sMesh *p_mesh );

