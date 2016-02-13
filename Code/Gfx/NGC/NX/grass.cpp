//-----------------------------------------------------------------------------
// File: XBFur.cpp
//
// Desc: routines for generating and displaying a patch of fur,
//       which is a series of layers that give the appearance of
//       hair, fur, grass, or other fuzzy things.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <core\defines.h>
#include <core\crc.h>
#include "nx_init.h"
#include "render.h"
#include "grass.h"
//#include "mipmap.h"

////extern LPDIRECT3DDEVICE8 NxNgc::EngineGlobals.p_Device;
//
//#define irand(a) ((rand()*(a))>>15)
//#define frand(a) ((float)rand()*(a)/32768.0f)
//
//#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
//
//
//float g_fOneInch = 0.01f;
//
//
//extern CXBFur *p_fur;
//
//
//
//
////-----------------------------------------------------------------------------
//// Name: Constructor
//// Desc: 
////-----------------------------------------------------------------------------
//CXBFur::CXBFur()
//{
//    DWORD i;
//    ZeroMemory(this, sizeof(CXBFur));
//
//    // init default patch
//    m_fXSize = 0.1f;
//    m_fZSize = 0.1f;
//
//    // init default fuzzlib
//    m_dwNumFuzzLib = 0;
//    m_pFuzzLib = NULL;
//    m_fuzzCenter.colorBase = D3DXCOLOR(1.f, 1.f, 1.f, 1.f);
//    m_fuzzCenter.colorTip = D3DXCOLOR(1.f, 1.f, 1.f, 0.f);
//    m_fuzzCenter.dp.y = 1.0f;
//
//    // init default fuzz
//    m_dwNumSegments = 4;
//    m_pFuzz = NULL;
//
//    // init default volume
//    m_dwNumSlices = 0;
//    m_dwSliceXSize = 0;
//    m_dwSliceZSize = 0;
//    for(i=0; i<XBFUR_MAXSLICE*2-1; i++)
//        m_apSliceTexture[i] = NULL;
//    m_pHairLightingTexture = NULL;
//    m_pFinTexture = NULL;
//    m_dwNumSlicesLOD = 0;
//    m_pSliceTextureLOD = m_apSliceTexture;
//    
//    InitFuzz(1, 1);
//}
//
////-----------------------------------------------------------------------------
//// Name: Destructor
//// Desc: 
////-----------------------------------------------------------------------------
//CXBFur::~CXBFur()
//{
//    DWORD i;
//    if(m_pFuzzLib)
//        delete m_pFuzzLib;
//    if(m_pFuzz)
//        delete m_pFuzz;
//    for(i=0; i<XBFUR_MAXSLICE*2-1; i++)
//        SAFE_RELEASE(m_apSliceTexture[i]);
//    SAFE_RELEASE(m_pHairLightingTexture);
//}
//
////-----------------------------------------------------------------------------
//// Name: InitFuzz
//// Desc: Initializes the individual strands of fuzz in the patch.
////       Only a small number of individual fuzzes are generated
////       (determined by m_dwNumFuzzLib) because each one in the patch
////       does not need to be unique.
////-----------------------------------------------------------------------------
//void CXBFur::InitFuzz(DWORD nfuzz, DWORD nfuzzlib)
//{
//    DWORD i;
//    float y;
//
//    if(nfuzz<=0 || nfuzzlib<0)
//        return;
//        
//    // handle memory allocation
//    if(m_dwNumFuzz!=nfuzz)                // if nfuzz has changed
//    {
//        if(m_pFuzz)
//            delete m_pFuzz;                // nuke existing
//        m_pFuzz = new FuzzInst[nfuzz];    // and get new fuzz memory
//    }
//
//    if(m_dwNumFuzzLib!=nfuzzlib)
//    {
//        if(m_pFuzzLib)
//            delete m_pFuzzLib;
//
//        m_pFuzzLib = new Fuzz[nfuzzlib];
//    }
//
//    m_dwNumFuzz = nfuzz;
//    m_dwNumFuzzLib = nfuzzlib;
//
//    // generate the individual fuzzes in the library
//    m_fYSize = 0.0f;
//    srand(m_dwSeed);
//    for(i=0; i<m_dwNumFuzzLib; i++)
//    {
//        m_pFuzzLib[i].dp.x = (m_fuzzCenter.dp.x + m_fuzzRandom.dp.x*(2*frand(1.0f)-1.0f))*g_fOneInch;
//        m_pFuzzLib[i].dp.y = (m_fuzzCenter.dp.y + m_fuzzRandom.dp.y*(2*frand(1.0f)-1.0f))*g_fOneInch;
//        m_pFuzzLib[i].dp.z = (m_fuzzCenter.dp.z + m_fuzzRandom.dp.z*(2*frand(1.0f)-1.0f))*g_fOneInch;
//
//        m_pFuzzLib[i].ddp.x = (m_fuzzCenter.ddp.x + m_fuzzRandom.ddp.x*(2*frand(1.0f)-1.0f))*g_fOneInch;
//        m_pFuzzLib[i].ddp.y = (m_fuzzCenter.ddp.y + m_fuzzRandom.ddp.y*(2*frand(1.0f)-1.0f))*g_fOneInch;
//        m_pFuzzLib[i].ddp.z = (m_fuzzCenter.ddp.z + m_fuzzRandom.ddp.z*(2*frand(1.0f)-1.0f))*g_fOneInch;
//
//        m_pFuzzLib[i].colorBase = m_fuzzCenter.colorBase + (2*frand(1.f)-1.f)*m_fuzzRandom.colorBase;
//        m_pFuzzLib[i].colorTip = m_fuzzCenter.colorTip + (2*frand(1.f)-1.f)*m_fuzzRandom.colorTip;
//
//        y = m_pFuzzLib[i].dp.y + 0.5f*m_pFuzzLib[i].ddp.y;
//        if(y>m_fYSize)
//            m_fYSize = y;
//    }
//
//     // initialize the fuzz locations & pick a random fuzz from the library
//    srand(m_dwSeed*54795);
//    for(i=0; i<m_dwNumFuzz; i++)
//    {
//        m_pFuzz[i].x = (frand(1.0f)-0.5f)*m_fXSize;
//        m_pFuzz[i].z = (frand(1.0f)-0.5f)*m_fZSize;
//        m_pFuzz[i].lidx = irand(m_dwNumFuzzLib);
//    }
//}
//
////-----------------------------------------------------------------------------
//// Name: GenSlices
//// Desc: Generate the fuzz volume by rendering the geometry repeatedly
////       with different z-clip ranges.
////-----------------------------------------------------------------------------
//void CXBFur::GenSlices( DWORD nslices, DWORD slicexsize, DWORD slicezsize )
//{
//	assert(nslices <= XBFUR_MAXSLICE);
//    
//    // Check the format of the textures
//    bool bFormatOK = true;
//    if (m_dwNumSlices > 0)
//    {
//        D3DSURFACE_DESC desc;
//        m_apSliceTexture[0]->GetLevelDesc(0, &desc);
//        if (desc.Format != D3DFMT_A8R8G8B8)
//            bFormatOK = false;
//    }
//
//    // make sure volume info is up to date
//    if(m_dwSliceXSize!=slicexsize 
//       || m_dwSliceZSize!=slicezsize
//       || !bFormatOK)
//    {
//        m_dwNumSlices = 0;
//        m_dwNumSlicesLOD = 0;
//        for(UINT i=0; i<XBFUR_MAXSLICE*2-1; i++)
//            SAFE_RELEASE(m_apSliceTexture[i]);
//    }
//
//    m_dwSliceXSize = slicexsize;
//    m_dwSliceZSize = slicezsize;
//    m_dwSliceSize = slicexsize*slicezsize;
//
//    // create textures if necessary
//    if(m_dwNumSlices!=nslices)
//    {
//        UINT i;
//        // count number of level-of-detail layers needed
//        UINT nLOD = 0;
//        for (i = 1; (1u << i) <= nslices; i++)
//            nLOD += nslices >> i;
//        m_dwLODMax = i - 1;
//
//        // create new layers and level-of-detail layers
//        for(i=0; i<nslices+nLOD; i++)
//            if(!m_apSliceTexture[i])
//                NxNgc::EngineGlobals.p_Device->CreateTexture(slicexsize, slicezsize, 0, 0, D3DFMT_A8R8G8B8, 0, &m_apSliceTexture[i]);
//            
//        // release unused layers
//        for(i=nslices+nLOD; i<XBFUR_MAXSLICE*2-1; i++)
//            SAFE_RELEASE(m_apSliceTexture[i]);
//
//    }
//    m_dwNumSlices = nslices;
//
//    // save current back buffer, z buffer, and transforms
//    struct {
//        IDirect3DSurface8 *pBackBuffer, *pZBuffer;
//        D3DMATRIX matWorld, matView, matProjection;
//    } save;
//    NxNgc::EngineGlobals.p_Device->GetRenderTarget(&save.pBackBuffer);
//    NxNgc::EngineGlobals.p_Device->GetDepthStencilSurface(&save.pZBuffer);
//    NxNgc::EngineGlobals.p_Device->GetTransform( D3DTS_WORLD, &save.matWorld);
//    NxNgc::EngineGlobals.p_Device->GetTransform( D3DTS_VIEW, &save.matView);
//    NxNgc::EngineGlobals.p_Device->GetTransform( D3DTS_PROJECTION, &save.matProjection);
//        
//    // make a new depth buffer
//    IDirect3DSurface8 *pZBuffer = NULL;
//    NxNgc::EngineGlobals.p_Device->CreateDepthStencilSurface(slicexsize, slicezsize, D3DFMT_LIN_D24S8, D3DMULTISAMPLE_NONE, &pZBuffer);
//    D3DXVECTOR3 va(-0.5f*m_fXSize, 0.f, -0.5f*m_fZSize), vb(0.5f*m_fXSize, m_fYSize, 0.5f*m_fZSize); // bounds of fuzz patch
//    D3DXVECTOR3 vwidth(vb-va), vcenter(0.5f*(vb+va)); // width and center of fuzz patch
//        
//    // set world transformation to scale model to unit cube [-1,1] in all dimensions
//    D3DXMATRIX matWorld, matTranslate, matScale;
//    D3DXMatrixTranslation(&matTranslate, -vcenter.x, -vcenter.y, -vcenter.z);
//    D3DXMatrixScaling(&matScale, 2.f/vwidth.x, 2.f/vwidth.y, 2.f/vwidth.z);
//    matWorld = matTranslate * matScale;
//    NxNgc::EngineGlobals.p_Device->SetTransform( D3DTS_WORLD, &matWorld);
//        
//    // set view transformation to flip z and look down y axis, with bottom-most slice scaled and translated to map to [0,1]
//    D3DMATRIX matView;
//    matView.m[0][0] = 1.f;    matView.m[0][1] =  0.f;    matView.m[0][2] = 0.f;                matView.m[0][3] = 0.f;
//    matView.m[1][0] = 0.f;    matView.m[1][1] =  0.f;    matView.m[1][2] = 0.5f * nslices;    matView.m[1][3] = 0.f;
//    matView.m[2][0] = 0.f;    matView.m[2][1] = -1.f;    matView.m[2][2] = 0.f;                matView.m[2][3] = 0.f;
//    matView.m[3][0] = 0.f;    matView.m[3][1] =  0.f;    matView.m[3][2] = 0.5f * nslices;    matView.m[3][3] = 1.f;
//        
//    // set projection matrix to orthographic
//    D3DXMATRIX matProjection;
//    D3DXMatrixIdentity(&matProjection);
//    NxNgc::EngineGlobals.p_Device->SetTransform( D3DTS_PROJECTION, &matProjection);
//    UINT numLines = m_dwNumFuzz*m_dwNumSegments;
//    IDirect3DVertexBuffer8 *pVB;
//    GetLinesVertexBuffer(&pVB);
//    NxNgc::EngineGlobals.p_Device->SetTexture(0, NULL);
//    NxNgc::EngineGlobals.p_Device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
//    NxNgc::EngineGlobals.p_Device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
//    NxNgc::EngineGlobals.p_Device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
//    NxNgc::EngineGlobals.p_Device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE); // premultiplied alpha
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR1);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_LIGHTING, FALSE);
//
//    NxNgc::EngineGlobals.p_Device->SetPixelShader( 0 );
//    NxNgc::EngineGlobals.p_Device->SetVertexShader(FVF_XYZDIFF);
//    NxNgc::EngineGlobals.p_Device->SetStreamSource(0, pVB, sizeof(FVFT_XYZDIFF));
//        
//    // draw each slice
//    for (int i = 0; i < (int)nslices; i++)
//    {
//        // get destination surface & set as render target, then draw the fuzz slice
//        LPDIRECT3DTEXTURE8 pTexture = m_apSliceTexture[i];
//        IDirect3DSurface8 *pSurface;
//        pTexture->GetSurfaceLevel(0, &pSurface);
//        NxNgc::EngineGlobals.p_Device->SetRenderTarget(pSurface, pZBuffer);
//        NxNgc::EngineGlobals.p_Device->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, D3DCOLOR_RGBA(0x10,0x20,0x10,0), 1.0f, 0 );
//        matView.m[1][2] = 0.5f * nslices;
//        matView.m[3][2] = 0.5f * nslices - (float)i;    // offset to next slice
//        
//        // We want the texture to wrap, so draw multiple times with offsets in the plane so that 
//        // the boundaries will be filled in by the overlapping geometry.
//        for (int iX = -1; iX <= 1; iX++)
//        {
//            for (int iY = -1; iY <= 1; iY++)
//            {
//                matView.m[3][0] = 2.f * iX;
//                matView.m[3][1] = 2.f * iY;
//                NxNgc::EngineGlobals.p_Device->SetTransform( D3DTS_VIEW, &matView);
//                NxNgc::EngineGlobals.p_Device->DrawPrimitive(D3DPT_LINELIST, 0, numLines);
//            }
//        }
//        pSurface->Release();
//        GenerateMipmaps(pTexture, 0);
//    }
//    
//    // clean up
//    pVB->Release();
//    pZBuffer->Release();
//    NxNgc::EngineGlobals.p_Device->SetRenderTarget(save.pBackBuffer, save.pZBuffer);
//    save.pBackBuffer->Release();
//    save.pZBuffer->Release();
//    NxNgc::EngineGlobals.p_Device->SetTransform( D3DTS_WORLD, &save.matWorld);
//    NxNgc::EngineGlobals.p_Device->SetTransform( D3DTS_VIEW, &save.matView);
//    NxNgc::EngineGlobals.p_Device->SetTransform( D3DTS_PROJECTION, &save.matProjection);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_LIGHTING, TRUE);
//
//
//	// Compress the textures.
//	for( int i = 0; i < (int)nslices; i++ )
//    {
//		LPDIRECT3DTEXTURE8 pTextureSrc = m_apSliceTexture[i];
//        LPDIRECT3DTEXTURE8 pTextureDst = NULL;
//		CompressTexture( &pTextureDst, D3DFMT_DXT4, pTextureSrc );
//		m_apSliceTexture[i] = pTextureDst;
//		pTextureSrc->Release();
//	}
//}
//
////-----------------------------------------------------------------------------
//// Name: GenFin
//// Desc: Generate the fin texture in a similar way as GenSlices.
////-----------------------------------------------------------------------------
//void CXBFur::GenFin(DWORD finWidth, DWORD finHeight, float fFinXFraction, float fFinZFraction)
//{
//    m_fFinXFraction = fFinXFraction;
//    m_fFinZFraction = fFinZFraction;
//
//    // Check the format of the textures
//    bool bFormatOK = true;
//    if (m_pFinTexture != NULL)
//    {
//        D3DSURFACE_DESC desc;
//        m_pFinTexture->GetLevelDesc(0, &desc);
//        if (desc.Format != D3DFMT_A8R8G8B8)
//            bFormatOK = false;
//    }
//
//    // make sure fin info is up to date
//    if(m_finWidth != finWidth
//       || m_finHeight != finHeight
//       || !bFormatOK)
//    {
//        SAFE_RELEASE(m_pFinTexture);
//    }
//    m_finWidth = finWidth;
//    m_finHeight = finHeight;
//
//    // create fin texture if needed
//    if (m_pFinTexture == NULL)
//        NxNgc::EngineGlobals.p_Device->CreateTexture(finWidth, finHeight, 0, 0, D3DFMT_A8R8G8B8, 0, &m_pFinTexture);
//
//    // save current back buffer, z buffer, and transforms
//    struct {
//        IDirect3DSurface8 *pBackBuffer, *pZBuffer;
//        D3DMATRIX matWorld, matView, matProjection;
//    } save;
//    NxNgc::EngineGlobals.p_Device->GetRenderTarget(&save.pBackBuffer);
//    NxNgc::EngineGlobals.p_Device->GetDepthStencilSurface(&save.pZBuffer);
//    NxNgc::EngineGlobals.p_Device->GetTransform( D3DTS_WORLD, &save.matWorld);
//    NxNgc::EngineGlobals.p_Device->GetTransform( D3DTS_VIEW, &save.matView);
//    NxNgc::EngineGlobals.p_Device->GetTransform( D3DTS_PROJECTION, &save.matProjection);
//
//    // make a new depth buffer
//    IDirect3DSurface8 *pZBuffer = NULL;
//    NxNgc::EngineGlobals.p_Device->CreateDepthStencilSurface(finWidth, finHeight, D3DFMT_LIN_D24S8, D3DMULTISAMPLE_NONE, &pZBuffer);
//    D3DXVECTOR3 va(-0.5f*m_fXSize, 0.f, -0.5f*m_fZSize), vb(0.5f*m_fXSize, m_fYSize, 0.5f*m_fZSize); // bounds of fuzz patch
//    D3DXVECTOR3 vwidth(vb-va), vcenter(0.5f*(vb+va)); // width and center of fuzz patch
//
//    // set world transformation to scale model to unit cube [-1,1] in all dimensions
//    D3DXMATRIX matWorld, matTranslate, matScale;
//    D3DXMatrixTranslation(&matTranslate, -vcenter.x, -vcenter.y, -vcenter.z);
//    D3DXMatrixScaling(&matScale, 2.f/vwidth.x, 2.f/vwidth.y, 2.f/vwidth.z);
//    matWorld = matTranslate * matScale;
//    NxNgc::EngineGlobals.p_Device->SetTransform( D3DTS_WORLD, &matWorld);
//
//    // set view transformation to look down z axis, with z range [0,1]
//    float fXFraction = m_fFinXFraction;
//    float fYFraction = 1.f;
//    float fZFraction = m_fFinZFraction; 
//    float fXScale = 1.f / fXFraction;
//    float fYScale = 1.f / fYFraction;
//    float fZScale = 1.f / fZFraction;
//    D3DMATRIX matView;
//    matView.m[0][0] = fXScale;    matView.m[0][1] =  0.f;        matView.m[0][2] = 0.f;                matView.m[0][3] = 0.f;
//    matView.m[1][0] = 0.f;        matView.m[1][1] = -fYScale;    matView.m[1][2] = 0.f;                matView.m[1][3] = 0.f;
//    matView.m[2][0] = 0.f;        matView.m[2][1] =  0.f;        matView.m[2][2] = 0.5f * fZScale;    matView.m[2][3] = 0.f;
//    matView.m[3][0] = 0.f;        matView.m[3][1] =  0.f;     matView.m[3][2] = 0.5f * fZScale;    matView.m[3][3] = 1.f;
//    NxNgc::EngineGlobals.p_Device->SetTransform( D3DTS_VIEW, &matView);
//    // set projection matrix to orthographic
//
//    D3DXMATRIX matProjection;
//    D3DXMatrixIdentity(&matProjection);
//    NxNgc::EngineGlobals.p_Device->SetTransform( D3DTS_PROJECTION, &matProjection);
//    UINT numLines = m_dwNumFuzz*m_dwNumSegments;
//    IDirect3DVertexBuffer8 *pVB;
//    GetLinesVertexBuffer(&pVB);
//    NxNgc::EngineGlobals.p_Device->SetTexture(0, NULL);
//    NxNgc::EngineGlobals.p_Device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
//    NxNgc::EngineGlobals.p_Device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
//    NxNgc::EngineGlobals.p_Device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
//    NxNgc::EngineGlobals.p_Device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE); // premultiplied alpha
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR1);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_LIGHTING, FALSE);
//    NxNgc::EngineGlobals.p_Device->SetVertexShader(FVF_XYZDIFF);
//    NxNgc::EngineGlobals.p_Device->SetStreamSource(0, pVB, sizeof(FVFT_XYZDIFF));
//    
//    // draw fuzz into the fin texture, looking from the side
//    LPDIRECT3DTEXTURE8 pTexture = m_pFinTexture;
//    IDirect3DSurface8 *pSurface;
//    pTexture->GetSurfaceLevel(0, &pSurface);
//    NxNgc::EngineGlobals.p_Device->SetRenderTarget(pSurface, pZBuffer);
//    NxNgc::EngineGlobals.p_Device->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 
//                         D3DCOLOR_RGBA(0,0,0,0), 1.0f, 0 );
//    NxNgc::EngineGlobals.p_Device->DrawPrimitive(D3DPT_LINELIST, 0, numLines);
//    pSurface->Release();
//    GenerateMipmaps(pTexture, 0, D3DTADDRESS_WRAP, D3DTADDRESS_CLAMP);
//
//    // clean up
//    pVB->Release();
//    pZBuffer->Release();
//    NxNgc::EngineGlobals.p_Device->SetRenderTarget(save.pBackBuffer, save.pZBuffer);
//    save.pBackBuffer->Release();
//    save.pZBuffer->Release();
//    NxNgc::EngineGlobals.p_Device->SetTransform( D3DTS_WORLD, &save.matWorld);
//    NxNgc::EngineGlobals.p_Device->SetTransform( D3DTS_VIEW, &save.matView);
//    NxNgc::EngineGlobals.p_Device->SetTransform( D3DTS_PROJECTION, &save.matProjection);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_LIGHTING, TRUE);
//}
//
////-----------------------------------------------------------------------------
//// Name: GetLinesVertexBuffer
//// Desc: Create and fill in a vertex buffer as a series of individual lines from the fuzz library.
////-----------------------------------------------------------------------------
//void CXBFur::GetLinesVertexBuffer(IDirect3DVertexBuffer8 **ppVB)
//{
//    IDirect3DVertexBuffer8 *pVB;
//    UINT numVertices = m_dwNumFuzz*m_dwNumSegments*2;
//    NxNgc::EngineGlobals.p_Device->CreateVertexBuffer(numVertices*sizeof(FVFT_XYZDIFF), 0, FVF_XYZDIFF, 0, &pVB);
//    assert(pVB!=NULL);
//    DWORD i, j, vidx, lidx;
//    FVFT_XYZDIFF *verts;
//    float x0, y0, z0;
//    float dx, dy, dz;
//    float ddx, ddy, ddz;
//    float step, cp;
//    UINT startseg = 0;
//    step = 1.0f/(float)m_dwNumSegments;
//    pVB->Lock(0, numVertices*sizeof(FVFT_XYZDIFF), (BYTE **)&verts, 0);
//    vidx = 0;
//    D3DXCOLOR colorBase, colorDelta;
//    for(i=0; i<m_dwNumFuzz; i++)
//    {
//        // get location and index from fuzz instances
//        lidx = m_pFuzz[i].lidx;
//        x0 = m_pFuzz[i].x;
//        y0 = 0.0f;
//        z0 = m_pFuzz[i].z;
//
//        // get params from fuzz lib
//        dx = m_pFuzzLib[lidx].dp.x;
//        dy = m_pFuzzLib[lidx].dp.y;
//        dz = m_pFuzzLib[lidx].dp.z;
//        ddx = m_pFuzzLib[lidx].ddp.x;
//        ddy = m_pFuzzLib[lidx].ddp.y;
//        ddz = m_pFuzzLib[lidx].ddp.z;
//        colorBase = m_pFuzzLib[lidx].colorBase;
//        colorDelta = m_pFuzzLib[lidx].colorTip - colorBase;
//
//        // build linelist
//        cp = (float)startseg*step;
//        D3DXCOLOR color;
//        for(j=startseg; j<m_dwNumSegments; j++)
//        {
//            verts[vidx].v.x = x0 + cp*dx + 0.5f*cp*cp*ddx;
//            verts[vidx].v.y = y0 + cp*dy + 0.5f*cp*cp*ddy;
//            verts[vidx].v.z = z0 + cp*dz + 0.5f*cp*cp*ddz;
//            color = colorBase + cp * cp * colorDelta;
//            verts[vidx].diff = color;
//            vidx++;
//            cp += step;
//
//            verts[vidx].v.x = x0 + cp*dx + 0.5f*cp*cp*ddx;
//            verts[vidx].v.y = y0 + cp*dy + 0.5f*cp*cp*ddy;
//            verts[vidx].v.z = z0 + cp*dz + 0.5f*cp*cp*ddz;
//            color = colorBase + cp * cp * colorDelta;
//            verts[vidx].diff = color;
//            vidx++;
//        }
//    }
//    pVB->Unlock();
//    *ppVB = pVB;
//}
//
////-----------------------------------------------------------------------------
//// Name: RenderLines
//// Desc: Draw the fuzz patch as a series of individual lines.
////-----------------------------------------------------------------------------
//void CXBFur::RenderLines()
//{
//    UINT numLines = m_dwNumFuzz*m_dwNumSegments;
//    IDirect3DVertexBuffer8 *pVB;
//    GetLinesVertexBuffer(&pVB);
//    NxNgc::EngineGlobals.p_Device->SetTexture(0, NULL);
//    NxNgc::EngineGlobals.p_Device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
//    NxNgc::EngineGlobals.p_Device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
//    NxNgc::EngineGlobals.p_Device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
//    NxNgc::EngineGlobals.p_Device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE); // premultiplied alpha
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_LIGHTING, FALSE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
//    NxNgc::EngineGlobals.p_Device->SetVertexShader(FVF_XYZDIFF);
//    NxNgc::EngineGlobals.p_Device->SetStreamSource(0, pVB, sizeof(FVFT_XYZDIFF));
//    NxNgc::EngineGlobals.p_Device->DrawPrimitive(D3DPT_LINELIST, 0, numLines);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_LIGHTING, TRUE);
//    pVB->Release();
//}
//
//
//
//#if 0
//
////-----------------------------------------------------------------------------
//// Desc: File routines for saving and loading patch files.
////       Saving is only relevant if used on a Windows platform.
////-----------------------------------------------------------------------------
//struct _fphdr
//{
//    char sig[5];
//    int flags;
//};
//
//void CXBFur::Save(char *fname, int flags)
//{
//    HANDLE hFile;
//    struct _fphdr hdr;
//
//    DWORD dwNumBytesWritten;
//    hFile = CreateFile(fname, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
//                       OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
//    if(hFile == INVALID_HANDLE_VALUE)
//        return;
//
//    // write file header
//    strcpy(hdr.sig, "FUZ2");
//    hdr.flags = flags;
//
//    WriteFile(hFile, &hdr, sizeof(struct _fphdr), &dwNumBytesWritten, NULL);
//    
//
//    // write fpatch data
//    WriteFile(hFile, &m_dwSeed, sizeof(DWORD), &dwNumBytesWritten, NULL);
//    
//    WriteFile(hFile, &m_fXSize, sizeof(float), &dwNumBytesWritten, NULL);
//    WriteFile(hFile, &m_fYSize, sizeof(float), &dwNumBytesWritten, NULL);
//    WriteFile(hFile, &m_fZSize, sizeof(float), &dwNumBytesWritten, NULL);
//
//    WriteFile(hFile, &m_dwNumSegments, sizeof(DWORD), &dwNumBytesWritten, NULL);
//    WriteFile(hFile, &m_fFinXFraction, sizeof(float), &dwNumBytesWritten, NULL);
//    WriteFile(hFile, &m_fFinZFraction, sizeof(float), &dwNumBytesWritten, NULL);
//    WriteFile(hFile, &m_fuzzCenter.colorBase, sizeof(D3DXCOLOR), &dwNumBytesWritten, NULL);
//    WriteFile(hFile, &m_fuzzRandom.colorBase, sizeof(D3DXCOLOR), &dwNumBytesWritten, NULL);
//    WriteFile(hFile, &m_fuzzCenter.colorTip, sizeof(D3DXCOLOR), &dwNumBytesWritten, NULL);
//    WriteFile(hFile, &m_fuzzRandom.colorTip, sizeof(D3DXCOLOR), &dwNumBytesWritten, NULL);
//    
//    WriteFile(hFile, &m_fuzzCenter.dp, sizeof(D3DVECTOR), &dwNumBytesWritten, NULL);
//    WriteFile(hFile, &m_fuzzRandom.dp, sizeof(D3DVECTOR), &dwNumBytesWritten, NULL);
//    WriteFile(hFile, &m_fuzzCenter.ddp, sizeof(D3DVECTOR), &dwNumBytesWritten, NULL);
//    WriteFile(hFile, &m_fuzzRandom.ddp, sizeof(D3DVECTOR), &dwNumBytesWritten, 0);
//
//    WriteFile(hFile, &m_dwNumFuzzLib, sizeof(DWORD), &dwNumBytesWritten, NULL);
//    WriteFile(hFile, &m_dwNumFuzz, sizeof(DWORD), &dwNumBytesWritten, NULL);
//
//    WriteFile(hFile, &m_dwNumSlices, sizeof(DWORD), &dwNumBytesWritten, NULL);
//    WriteFile(hFile, &m_dwSliceXSize, sizeof(DWORD), &dwNumBytesWritten, NULL);
//    WriteFile(hFile, &m_dwSliceZSize, sizeof(DWORD), &dwNumBytesWritten, NULL);
//    
//    return;
//
//
//
//    // write volume data if available and flag is set
//    {
//    }
//
//    // cleanup
//    CloseHandle(hFile);
//}
//
//void CXBFur::Load(char *fname)
//{
//    HANDLE hFile;
//    struct _fphdr hdr;
//    DWORD numfuzzlib, numfuzz;
//    DWORD numslices, slicexsize, slicezsize;
//
//    DWORD dwNumBytesRead;
//    hFile = CreateFile(fname, GENERIC_READ, FILE_SHARE_READ, NULL,
//                       OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
//    if(hFile == INVALID_HANDLE_VALUE)
//        return;
//    
//    // read file header
//    ReadFile(hFile, &hdr, sizeof(struct _fphdr), &dwNumBytesRead, 0);
//
//    // verify signature
//    bool bFUZ1 = !strcmp(hdr.sig, "FUZ1");
//    bool bFUZ2 = !strcmp(hdr.sig, "FUZ2");
//    if (!bFUZ1 && !bFUZ2)
//        return;    // signature not understood
//
//    // read patch data
//
//    ReadFile(hFile, &m_dwSeed, sizeof(DWORD), &dwNumBytesRead, NULL);
//    ReadFile(hFile, &m_fXSize, sizeof(float), &dwNumBytesRead, NULL);
//    ReadFile(hFile, &m_fYSize, sizeof(float), &dwNumBytesRead, NULL);
//    ReadFile(hFile, &m_fZSize, sizeof(float), &dwNumBytesRead, NULL);
//    
//
//    ReadFile(hFile, &m_dwNumSegments, sizeof(DWORD), &dwNumBytesRead, NULL);
//    
//    if (bFUZ1)
//    {
//        m_fFinXFraction = 1.f;
//        m_fFinZFraction = 0.05f;
//
//        ReadFile(hFile, &m_fuzzCenter.colorBase, sizeof(D3DXCOLOR), &dwNumBytesRead, NULL);
//        m_fuzzCenter.colorTip = m_fuzzCenter.colorBase;
//        m_fuzzRandom.colorBase = m_fuzzRandom.colorTip = 0ul;
//    }
//    else
//    {
//
//        ReadFile(hFile, &m_fFinXFraction, sizeof(float), &dwNumBytesRead, NULL);
//        ReadFile(hFile, &m_fFinZFraction, sizeof(float), &dwNumBytesRead, NULL);
//        ReadFile(hFile, &m_fuzzCenter.colorBase, sizeof(D3DXCOLOR), &dwNumBytesRead, NULL);
//        ReadFile(hFile, &m_fuzzRandom.colorBase, sizeof(D3DXCOLOR), &dwNumBytesRead, NULL);
//        ReadFile(hFile, &m_fuzzCenter.colorTip, sizeof(D3DXCOLOR), &dwNumBytesRead, NULL);
//        ReadFile(hFile, &m_fuzzRandom.colorTip, sizeof(D3DXCOLOR), &dwNumBytesRead, NULL);
//    }
//
//    ReadFile(hFile, &m_fuzzCenter.dp, sizeof(D3DVECTOR), &dwNumBytesRead, NULL);  // velocity center
//    ReadFile(hFile, &m_fuzzRandom.dp, sizeof(D3DVECTOR), &dwNumBytesRead, NULL);  // velocity random
//    ReadFile(hFile, &m_fuzzCenter.ddp, sizeof(D3DVECTOR), &dwNumBytesRead, NULL); // acceleration center
//    ReadFile(hFile, &m_fuzzRandom.ddp, sizeof(D3DVECTOR), &dwNumBytesRead, NULL); // acceleration random
//    
//
//    ReadFile(hFile, &numfuzzlib, sizeof(DWORD), &dwNumBytesRead, NULL);
//    ReadFile(hFile, &numfuzz, sizeof(DWORD), &dwNumBytesRead, NULL);
//        
//    ReadFile(hFile, &numslices, sizeof(DWORD), &dwNumBytesRead, NULL);
//    ReadFile(hFile, &slicexsize, sizeof(DWORD), &dwNumBytesRead, NULL);
//    ReadFile(hFile, &slicezsize, sizeof(DWORD), &dwNumBytesRead, NULL);
//       
//    // read volume data if available and flag is set
//    {
//    }
//
//    CloseHandle(hFile);
//
//    InitFuzz(numfuzz, numfuzzlib);
//    GenSlices(numslices, slicexsize, slicezsize);
//    ComputeLevelOfDetailTextures();
//    SetLevelOfDetail(0.f);
//}
//
////////////////////////////////////////////////////////////////////////
//// Lighting Model
////////////////////////////////////////////////////////////////////////
//
//inline float
//Luminance(const D3DXCOLOR &c)
//{
//    return (0.229f * c.r) + (0.587f * c.g) + (0.114f * c.b);
//}
//
//inline float
//MaxChannel(const D3DXCOLOR &c)
//{
//    if (c.r > c.g)
//    {
//        if (c.r > c.b)
//            return c.r;
//        else
//            return c.b;
//    }
//    else
//    {
//        if (c.g > c.b)
//            return c.g;
//        else
//            return c.b;
//    }
//}
//
//inline D3DXCOLOR 
//Lerp(const D3DXCOLOR &c1, const D3DXCOLOR &c2, float s)
//{
//    return c1 + s * (c2 - c1);
//}
//
//inline D3DXCOLOR
//Desaturate(const D3DXCOLOR &rgba)
//{
//    float alpha = rgba.a;
//    if (alpha > 1.f)
//        alpha = 1.f;
//    float fMaxChan = MaxChannel(rgba);
//    if (fMaxChan > alpha) 
//    {
//        D3DXCOLOR rgbGray(alpha, alpha, alpha, alpha);
//        float fYOld = Luminance(rgba);
//        if (fYOld >= alpha)
//            return rgbGray;
//        // scale color to preserve hue
//        D3DXCOLOR rgbNew;
//        float fInvMaxChan = 1.f / fMaxChan;
//        rgbNew.r = rgba.r * fInvMaxChan;
//        rgbNew.g = rgba.g * fInvMaxChan;
//        rgbNew.b = rgba.b * fInvMaxChan;
//        rgbNew.a = alpha;
//        float fYNew = Luminance(rgbNew);
//        // add gray to preserve luminance
//        return Lerp(rgbNew, rgbGray, (fYOld - fYNew) / (alpha - fYNew));
//    }
//    return rgba;
//}
//
//inline D3DXCOLOR &operator *=(D3DXCOLOR &p, const D3DXCOLOR &q)
//{
//    p.r *= q.r;
//    p.g *= q.g;
//    p.b *= q.b;
//    p.a *= q.a;
//    return p;
//}
//
//inline D3DXCOLOR operator *(const D3DXCOLOR &p, const D3DXCOLOR &q)
//{
//    D3DXCOLOR r;
//    r.r = p.r * q.r;
//    r.g = p.g * q.g;
//    r.b = p.b * q.b;
//    r.a = p.a * q.a;
//    return r;
//}
//
////-----------------------------------------------------------------------------
////  HairLighting
////-----------------------------------------------------------------------------
//struct HairLighting {
//    // like D3DMATERIAL8, except populated with D3DXCOLOR's so that color arithmetic works
//    D3DXCOLOR m_colorDiffuse;
//    D3DXCOLOR m_colorSpecular;
//    float m_fSpecularExponent;
//    D3DXCOLOR m_colorAmbient;
//    D3DXCOLOR m_colorEmissive;
//
//    HRESULT Initialize(LPDIRECT3DDEVICE8 pDevice, D3DMATERIAL8 *pMaterial);
//    HRESULT CalculateColor(D3DXCOLOR *pColor, float LT, float HT);
//    HRESULT CalculateClampedColor(D3DXCOLOR *pColor, float LT, float HT);
//};
//
//HRESULT HairLighting::Initialize(LPDIRECT3DDEVICE8 pDevice, D3DMATERIAL8 *pMaterial)
//{
//    // Get current colors modulated by light 0 and global ambient
//    // ignore current alphas
//    m_colorDiffuse = D3DXCOLOR(pMaterial->Diffuse.r, pMaterial->Diffuse.g, pMaterial->Diffuse.b, 1.f);
//    m_colorSpecular = D3DXCOLOR(pMaterial->Specular.r, pMaterial->Specular.g, pMaterial->Specular.b, 1.f);
//    m_fSpecularExponent = pMaterial->Power;
//    m_colorAmbient = D3DXCOLOR(pMaterial->Ambient.r, pMaterial->Ambient.g, pMaterial->Ambient.b, 1.f);
//    m_colorEmissive = D3DXCOLOR(pMaterial->Emissive.r, pMaterial->Emissive.g, pMaterial->Emissive.b, 1.f);
//    return S_OK;
//}
//
///* Zockler et al's technique worked with streamlines, and used dot(V,T) instead of the simpler dot(H,T) */
//#define ZOCKLER 0 
//#if ZOCKLER
//
//HRESULT HairLighting::CalculateColor(D3DXCOLOR *pColor, float LT, float VT)
//{
//    // Zockler et al 1996
//    float fDiffuseExponent = 2.f;    // Banks used 4.8
//    float fDiffuse = powf(sqrtf(1.f - LT*LT), fDiffuseExponent);
//    float VR = LT*VT - sqrtf(1.f - LT*LT) * sqrtf(1.f - VT*VT);
//    float fSpecular = powf(VR, m_fSpecularExponent);
//    *pColor = m_colorEmissive + m_colorAmbient + fSpecular * m_colorSpecular + fDiffuse * m_colorDiffuse;
//    return S_OK;
//}
//
//#else
//
//HRESULT HairLighting::CalculateColor(D3DXCOLOR *pColor, float LT, float HT)
//{
//    float fDiffuseExponent = 2.f;    // Banks used 4.8
//    float fDiffuse = powf(sqrtf(1.f - LT*LT), fDiffuseExponent);
//    float fSpecular = powf(sqrtf(1.f - HT*HT), m_fSpecularExponent);
//    *pColor = m_colorEmissive + m_colorAmbient + fSpecular * m_colorSpecular + fDiffuse * m_colorDiffuse;
//    return S_OK;
//}
//
//#endif
//
//HRESULT HairLighting::CalculateClampedColor(D3DXCOLOR *pColor, float LT, float HT)
//{
//    D3DXCOLOR color;
//    HRESULT hr = CalculateColor(&color, LT, HT);
//    *pColor = Desaturate(color);    // bring back to 0,1 range
//    return hr;
//}
//
////////////////////////////////////////////////////////////////////////
//// 
//// Create a hair lighting lookup-table texture
//// 
////////////////////////////////////////////////////////////////////////
//HRESULT FillHairLightingTexture(D3DMATERIAL8 *pMaterial, LPDIRECT3DTEXTURE8 pTexture)
//{
//    /* 
//    The hair lighting texture maps U as the dot product of the hair
//    tangent T with the light direction L and maps V as the dot product
//    of the tangent with the half vector H.  Since the lighting is a
//    maximum when the tangent is perpendicular to L (or H), the maximum
//    is at zero.  The minimum is at 0.5, 0.5 (or -0.5, -0.5, since
//    wrapping is turned on.)  For the mapped T.H, we raise the map
//    value to a specular power.
//    */
//    HRESULT hr;
//    D3DSURFACE_DESC desc;
//    pTexture->GetLevelDesc(0, &desc);
//    if (desc.Format != D3DFMT_A8R8G8B8)
//        return E_NOTIMPL;
//    DWORD dwPixelStride = 4;
//    D3DLOCKED_RECT lockedRect;
//    hr = pTexture->LockRect(0, &lockedRect, NULL, 0l);
//    if (FAILED(hr))
//        return hr;
//    HairLighting lighting;
//    lighting.Initialize(NxNgc::EngineGlobals.p_Device, pMaterial);
//    Swizzler s(desc.Width, desc.Height, 0);
//    s.SetV(s.SwizzleV(0));
//    s.SetU(s.SwizzleU(0));
//    for (UINT v = 0; v < desc.Height; v++)
//    {
//        for (UINT u = 0; u < desc.Width; u++)
//        {
//            BYTE *p = (BYTE *)lockedRect.pBits + dwPixelStride * s.Get2D();
//            // vertical is specular lighting
//            // horizontal is diffuse lighting
//            D3DXCOLOR color;
//#if ZOCKLER
//            // Zockler et al 1996
//            float LT = 2.f * u / desc.Width - 1.0f;
//            float VT = 2.f * v / desc.Height - 1.0f;
//            lighting.CalculateClampedColor(&color, LT, VT);
//#else
//            float LT = 2.f * u / desc.Width;
//            if (LT > 1.f) LT -= 2.f;
//            float HT = 2.f * v / desc.Height;
//            if (HT > 1.f) HT -= 2.f;
//            lighting.CalculateClampedColor(&color, LT, HT);
//#endif
//            int r, g, b;
//            r = (BYTE)(255 * color.r);
//            g = (BYTE)(255 * color.g);
//            b = (BYTE)(255 * color.b);
//            *p++ = (BYTE)b;
//            *p++ = (BYTE)g;
//            *p++ = (BYTE)r;
//            *p++ = 255;    // alpha
//            s.IncU();
//        }
//        s.IncV();
//    }
//    pTexture->UnlockRect(0);
//    return S_OK;
//}
//
////-----------------------------------------------------------------------------
//// Name: SetHairLightingMaterial
//// Desc: Create and fill the hair lighting texture
////-----------------------------------------------------------------------------
//HRESULT CXBFur::SetHairLightingMaterial(D3DMATERIAL8 *pMaterial)
//{
//    // Create and fill the hair lighting texture
//    HRESULT hr;
//    m_HairLightingMaterial = *pMaterial;
//    if (!m_pHairLightingTexture)
//    {
//        DWORD dwWidth = 8; // an extremely small texture suffices when the exponents are low
//        DWORD dwHeight = 8;
//        D3DFORMAT surfaceFormat = D3DFMT_A8R8G8B8;
//        DWORD nMipMap = 1;
//        hr = D3DXCreateTexture(NxNgc::EngineGlobals.p_Device, dwWidth, dwHeight, nMipMap, 0, surfaceFormat, 0, &m_pHairLightingTexture);
//        if (FAILED(hr)) goto e_Exit;
//    }
//    hr = FillHairLightingTexture(pMaterial, m_pHairLightingTexture);
//    if (FAILED(hr)) goto e_Exit;
//e_Exit:
//    if (FAILED(hr))
//        SAFE_RELEASE(m_pHairLightingTexture);
//    return hr;
//}
//
////-----------------------------------------------------------------------------
//// Choose level of detail 
////
//// 0       = finest detail, all slices of original source textures
//// ...
//// i       = reduced number of slices, N / (1 << i)
//// i + f   = odd slices fade to clear, texLOD[2*j+1] = tex[2*j+1] * (1-f) + clear * f
////           even slices compensate,   texLOD[2*j] = (tex[2*j+1] * f) OVER tex[2*j]
//// i + 1   = reduced number of slices, N / (1 << (i+1))
//// ...
//// log2(N) = coarsest, one slice with composite of all source textures
////-----------------------------------------------------------------------------
//HRESULT CXBFur::SetLevelOfDetail(float fLevelOfDetail)
//{
//    // Choose number of LOD slices
//    if (fLevelOfDetail < 0.f)
//    {
//        m_fLevelOfDetail = 0.f;
//        m_iLOD = 0;
//        m_fLODFraction = 0.f;
//    }
//    else if (fLevelOfDetail > (float)m_dwLODMax)
//    {
//        m_fLevelOfDetail = (float)m_dwLODMax;
//        m_iLOD = m_dwLODMax;
//        m_fLODFraction = 0.f;
//    }
//    else
//    {
//        m_fLevelOfDetail = fLevelOfDetail;
//        m_iLOD = (UINT)floorf(fLevelOfDetail);
//        m_fLODFraction = fLevelOfDetail - (float)m_iLOD;
//    }
//    m_dwNumSlicesLOD = LevelOfDetailCount(m_iLOD);
//    UINT index = LevelOfDetailIndex(m_iLOD);
//    m_pSliceTextureLOD = m_apSliceTexture + index;
//    return S_OK;
//}
//
////-----------------------------------------------------------------------------
////
//// Generate level-of-detail textures by compositing together alternating layers.
////
////-----------------------------------------------------------------------------
//HRESULT CXBFur::ComputeLevelOfDetailTextures()
//{
//    // All the textures must have the same number of mip levels.
//    DWORD nMip = m_apSliceTexture[0]->GetLevelCount();
//
//    // save current back buffer and z buffer
//    struct {
//        IDirect3DSurface8 *pBackBuffer, *pZBuffer;
//    } save;
//    NxNgc::EngineGlobals.p_Device->GetRenderTarget(&save.pBackBuffer);
//    NxNgc::EngineGlobals.p_Device->GetDepthStencilSurface(&save.pZBuffer);
//
//    // set render state for compositing textures
//    NxNgc::EngineGlobals.p_Device->SetVertexShader(D3DFVF_XYZRHW|D3DFVF_TEX1);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_LIGHTING, FALSE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_FOGENABLE, FALSE);
//
//    // use pixel shaders to composite two or three layers at a time
//#pragma warning(push)
//#pragma warning(disable: 4245)    // conversion from int to DWORD
//    DWORD dwPS2 = 0;
//    {
//#include "comp2.inl"
//        NxNgc::EngineGlobals.p_Device->CreatePixelShader(&psd, &dwPS2);
//    }
//    DWORD dwPS3 = 0;
//    {
//#include "comp3.inl"
//        NxNgc::EngineGlobals.p_Device->CreatePixelShader(&psd, &dwPS3);
//    }
//#pragma warning(pop)
//
//    // set default texture stage states
//    UINT xx; // texture stage index
//    for (xx = 0; xx < 4; xx++)
//    {
//        NxNgc::EngineGlobals.p_Device->SetTexture(xx, NULL);
//        NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_COLOROP, D3DTOP_DISABLE);    // Are the COLOROP and ALPHAOP needed since we're using a pixel shader?
//        NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
//        NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);    // pass texture coords without transformation
//        NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_TEXCOORDINDEX, 0);            // all the textures use the same tex coords
//        NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
//        NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
//        NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_MAGFILTER, D3DTEXF_POINT);
//        NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_MINFILTER, D3DTEXF_POINT);
//        NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_MIPFILTER, D3DTEXF_POINT);
//        NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_MIPMAPLODBIAS, 0);
//        NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_MAXMIPLEVEL, 0);
//        NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_COLORKEYOP, D3DTCOLORKEYOP_DISABLE);
//        NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_COLORSIGN, 0);
//        NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_ALPHAKILL, D3DTALPHAKILL_DISABLE);
//    }
//    
//    // Compute all the level-of-detail textures
//    for (UINT iLOD = 1; m_dwNumSlices >> iLOD; iLOD++)
//    {
//        UINT nSliceSrc = LevelOfDetailCount(iLOD-1);
//        LPDIRECT3DTEXTURE8 *apTextureSrc = m_apSliceTexture + LevelOfDetailIndex(iLOD - 1);
//        UINT nSliceDst = LevelOfDetailCount(iLOD);
//        LPDIRECT3DTEXTURE8 *apTextureDst = m_apSliceTexture + LevelOfDetailIndex(iLOD);
//        
//        // Composite source textures into LOD textures
//        UINT iMipNotHandled = (UINT)-1;
//        for (UINT iSliceDst = 0; iSliceDst < nSliceDst; iSliceDst++)
//        {
//            LPDIRECT3DTEXTURE8 pTextureDst = apTextureDst[iSliceDst];
//            UINT nComp;
//            if (iSliceDst == nSliceDst-1 && nSliceSrc > nSliceDst * 2)
//            {
//                // composite 3 textures into the top-most level when number of source textures is odd
//                nComp = 3;
//                NxNgc::EngineGlobals.p_Device->SetPixelShader(dwPS3);
//            }
//            else
//            {
//                // composite 2 textures (this is the default)
//                nComp = 2;
//                NxNgc::EngineGlobals.p_Device->SetPixelShader(dwPS2);
//            }
//            for (xx = 0; xx < nComp; xx++)
//            {
//                NxNgc::EngineGlobals.p_Device->SetTexture(xx, apTextureSrc[ iSliceDst * 2 + xx]);
//                NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
//                NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
//            }
//            for (; xx<4; xx++)
//            {
//                NxNgc::EngineGlobals.p_Device->SetTexture(xx, NULL);
//                NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_COLOROP, D3DTOP_DISABLE);
//                NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
//            }
//            for (UINT iMip = 0; iMip < nMip; iMip++)
//            {
//                DWORD width = m_dwSliceXSize / (1 << iMip);
//                if (width == 0) width = 1;
//                DWORD height = m_dwSliceZSize / (1 << iMip);
//                if (height == 0) height = 1;
//
//                // Ngc render target must of be at least 16x16
//                if (width*4 < 64 || width * height < 64)            
//                {
//                    iMipNotHandled = iMip;
//                    break; // skip rest of coarser mipmaps and go to next slice
//                }
//
//                // Use a screen space quad to do the compositing.
//                struct quad {
//                    float x, y, z, w;
//                    float u, v;
//                } aQuad[4] =
//                {
//                    {-0.5f,        -0.5f,         1.0f, 1.0f, 0.0f, 0.0f},
//                    {width - 0.5f, -0.5f,         1.0f, 1.0f, 1.0f, 0.0f},
//                    {-0.5f,        height - 0.5f, 1.0f, 1.0f, 0.0f, 1.0f},
//                    {width - 0.5f, height - 0.5f, 1.0f, 1.0f, 1.0f, 1.0f}
//                };
//
//                // get destination surface and set as render target
//                IDirect3DSurface8 *pSurface;
//                pTextureDst->GetSurfaceLevel(iMip, &pSurface);
//                NxNgc::EngineGlobals.p_Device->SetRenderTarget(pSurface, NULL); // no depth buffering
//                NxNgc::EngineGlobals.p_Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, aQuad, sizeof(quad)); // one quad blends 2 or 3 textures
//                pSurface->Release();
//            }
//        }
//        if (iMipNotHandled > 0 && iMipNotHandled != -1)
//        {
//            // fill in the small mips with filtered versions of the previous levels
//            for (UINT iSliceDst = 0; iSliceDst < nSliceDst; iSliceDst++)
//            {
//                LPDIRECT3DTEXTURE8 pTextureDst = apTextureDst[iSliceDst];
//                GenerateMipmaps(pTextureDst, iMipNotHandled - 1, D3DTADDRESS_MIRROR, D3DTADDRESS_MIRROR);
//            }
//        }
//    }
//
//    // clean up pixel shaders
//    NxNgc::EngineGlobals.p_Device->SetPixelShader(0);
//    NxNgc::EngineGlobals.p_Device->DeletePixelShader(dwPS2);
//    NxNgc::EngineGlobals.p_Device->DeletePixelShader(dwPS3);
//    
//    // restore render states
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_LIGHTING, TRUE);
//    NxNgc::EngineGlobals.p_Device->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
//
//    // restore texture stage states
//    for (xx=0; xx<4; xx++)
//    {
//        NxNgc::EngineGlobals.p_Device->SetTexture(xx, NULL);
//        NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_COLOROP, D3DTOP_DISABLE);
//        NxNgc::EngineGlobals.p_Device->SetTextureStageState(xx, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
//    }
//
//    // restore back buffer and z buffer
//    NxNgc::EngineGlobals.p_Device->SetRenderTarget(save.pBackBuffer, save.pZBuffer);
//    save.pBackBuffer->Release();
//    save.pZBuffer->Release();
//    return S_OK;
//}
//
////-----------------------------------------------------------------------------
////  Divide alpha channel into color channel.  Leave alpha unchanged.
////-----------------------------------------------------------------------------
//HRESULT AlphaDivide(LPDIRECT3DTEXTURE8 pTexture)
//{
//    HRESULT hr;
//    DWORD nMip = pTexture->GetLevelCount();
//    for (UINT iMip = 0; iMip < nMip; iMip++)
//    {
//        D3DSURFACE_DESC desc;
//        hr = pTexture->GetLevelDesc(iMip, &desc);
//        if (FAILED(hr))
//            return hr;
//        if (desc.Format != D3DFMT_A8R8G8B8)
//            return E_NOTIMPL;
//        D3DLOCKED_RECT lockedRect;
//        hr = pTexture->LockRect(iMip, &lockedRect, NULL, 0l);
//        if (FAILED(hr))
//            return hr;
//        UINT dwPixelSize = 4;
//        UINT nPixel = desc.Size / dwPixelSize;
//        DWORD *pPixel = (DWORD *)lockedRect.pBits;
//        while (nPixel--)
//        {
//            D3DXCOLOR c(*pPixel);
//            if (c.a > 0.f)
//            {
//                D3DXCOLOR d;
//                d.r = c.r / c.a;
//                d.g = c.g / c.a;
//                d.b = c.b / c.a;
//                d.a = c.a;
//                *pPixel = d;
//            }
//            pPixel++;
//        }
//        pTexture->UnlockRect(iMip);
//    }
//    return S_OK;
//}
//
////-----------------------------------------------------------------------------
////  Multiply alpha channel into color channel.  Leave alpha unchanged.
////-----------------------------------------------------------------------------
//HRESULT AlphaMultiply(LPDIRECT3DTEXTURE8 pTexture)
//{
//    HRESULT hr;
//    DWORD nMip = pTexture->GetLevelCount();
//    for (UINT iMip = 0; iMip < nMip; iMip++)
//    {
//        D3DSURFACE_DESC desc;
//        hr = pTexture->GetLevelDesc(iMip, &desc);
//        if (FAILED(hr))
//            return hr;
//        if (desc.Format != D3DFMT_A8R8G8B8)
//            return E_NOTIMPL;
//        D3DLOCKED_RECT lockedRect;
//        hr = pTexture->LockRect(iMip, &lockedRect, NULL, 0l);
//        if (FAILED(hr))
//            return hr;
//        UINT dwPixelSize = 4;
//        UINT nPixel = desc.Size / dwPixelSize;
//        DWORD *pPixel = (DWORD *)lockedRect.pBits;
//        while (nPixel--)
//        {
//            D3DXCOLOR c(*pPixel);
//            D3DXCOLOR d;
//            d.r = c.r * c.a;
//            d.g = c.g * c.a;
//            d.b = c.b * c.a;
//            d.a = c.a;
//            *pPixel = d;
//            pPixel++;
//        }
//        pTexture->UnlockRect(iMip);
//    }
//    return S_OK;
//}
//
////-----------------------------------------------------------------------------
////
//// Copy textures to new texture format
////
////-----------------------------------------------------------------------------
//HRESULT CXBFur::CompressNextTexture(D3DFORMAT fmtNew, UINT *pIndex)
//{
//    HRESULT hr;
//    for (UINT iTexture = 0; iTexture < XBFUR_MAXSLICE * 2 - 1; iTexture++)    // convert all the slice textures
//    {
//        LPDIRECT3DTEXTURE8 pTextureDst = NULL;
//        LPDIRECT3DTEXTURE8 pTextureSrc = m_apSliceTexture[iTexture];
//        if (pTextureSrc == NULL)
//            break;    // we're done
//
//        // See if this texture needs to be compressed
//        D3DSURFACE_DESC desc0;
//        pTextureSrc->GetLevelDesc(0, &desc0);
//        if (desc0.Format == fmtNew)
//            continue;    // we've already compressed this one, go to the next
//
//        // We've found a texture that needs to be compressed
//        hr = CompressTexture(&pTextureDst, fmtNew, pTextureSrc);
//        if (FAILED(hr))
//            return hr;
//        m_apSliceTexture[iTexture] = pTextureDst; // already addref'd
//        pTextureSrc->Release();    // we're done with the old texture
//        if (pIndex != NULL)
//            *pIndex = iTexture;    // return index of the texture we just compressed
//        return S_FALSE;    // more textures are pending
//    }
//    return S_OK;
//}
//
//
//#endif
//
//
//static bool grass_mats = false;
//static NxNgc::sMaterial*	p_grass_mats[16];
//
//
//static void createGrassMats( void )
//{
//	if( !grass_mats )
//	{
//		grass_mats = true;
//
//		for( int layer = 0; layer < 8; ++layer )
//		{
//			NxNgc::sTexture *p_tex		= new NxNgc::sTexture();
//			p_tex->pD3DTexture			= p_fur->m_apSliceTexture[layer];
//			p_tex->pD3DPalette			= NULL;
//
//			
//			NxNgc::sMaterial *p_mat	= new NxNgc::sMaterial();
//
//			p_mat->m_no_bfc				= false;
//			p_mat->Passes				= 1;
//			p_mat->pTex[0]				= p_tex;
//
//			if( layer < 3 )
//			{
//				p_mat->Flags[0]				= 0x00;
//				p_mat->RegALPHA[0]			= NxNgc::vBLEND_MODE_DIFFUSE;
//			}
//			else
//			{
//				p_mat->Flags[0]				= 0x40;	// Transparent.
//				p_mat->RegALPHA[0]			= NxNgc::vBLEND_MODE_BLEND;
//			}
//
//			p_mat->UVAddressing[0]		= 0x00000000UL;
//			p_mat->AlphaCutoff			= 1;
//			p_mat->Color[0][0]			= 1.0f;
//			p_mat->Color[0][1]			= 1.0f;
//			p_mat->Color[0][2]			= 1.0f;
//			p_mat->Color[0][3]			= 1.0f;
//			p_mat->m_sorted				= false;
//			p_mat->m_draw_order			= 0;
//			p_mat->mp_wibble_vc_colors	= NULL;
//
//			p_grass_mats[layer]			= p_mat;
//		}
//	}
//}
//



bool AddGrass( Nx::CNgcGeom *p_geom, NxNgc::sMesh *p_mesh )
{
#if 0
	bool add_grass = false;
	
	if( p_mesh->mp_material && p_mesh->mp_material->m_grassify )
	{
		add_grass = true;
	}
	else
	{
		return false;
	}

	// At this stage we know we want to add grass.
	Dbg_Assert( p_mesh->mp_material->m_grass_layers > 0 );
//	float height_per_layer = p_mesh->mp_material->m_grass_height / p_mesh->mp_material->m_grass_layers;
	
	for( int layer = 0; layer < p_mesh->mp_material->m_grass_layers; ++layer )
	{
		Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().BottomUpHeap());
		NxNgc::sMesh *p_grass_mesh = p_mesh->Clone( false );
		Mem::Manager::sHandle().PopContext();

		// No longer any shadow cast on the base mesh.
//		p_mesh->m_flags |= NxNgc::sMesh::MESH_FLAG_NO_SKATER_SHADOW;
		
		if( layer != 2 )
			p_grass_mesh->m_flags |= NxNgc::sMesh::MESH_FLAG_NO_SKATER_SHADOW;
		
		// Now point the mesh to a different material, first build the material checksum from the name of the parent
		// material (Grass) and the name of the sub material (Grass_LayerX)...
		char material_name[64];
		sprintf( material_name, "Grass-Grass_Layer%d", layer );
		uint32 material_checksum	= Crc::GenerateCRCCaseSensitive( material_name, strlen( material_name ));

		// ...then use the checksum to lookup the material.
		p_grass_mesh->mp_material	= p_geom->mp_scene->GetEngineScene()->GetMaterial( material_checksum );

		// We need also to override the mesh pixel shader, which may have been setup for a multipass material.
//		NxNgc::GetPixelShader( p_grass_mesh->mp_material, &p_grass_mesh->m_pixel_shader, &p_grass_mesh->m_pixel_shader_no_mcm );
		
		// Add transparent flag to the material.
		p_grass_mesh->mp_material->Flags[0]		|= 0x40;

		// Set the draw order to ensure meshes are drawn from the bottom up.
		p_grass_mesh->mp_material->m_sorted		 = false;
		p_grass_mesh->mp_material->m_draw_order	 = layer * 0.1f;
		
		// Go through and move all the vertices up a little, and calculate new texture coordinates based on the x-z space of the vertex positions.
//		char *p_byte_dest;
//		p_grass_mesh->mp_vertex_buffer[0]->Lock( 0, 0, &p_byte_dest, 0 );
//
//		for( uint32 vert = 0; vert < p_grass_mesh->m_num_vertices; ++vert )
//		{
//			D3DXVECTOR3 *p_pos = (D3DXVECTOR3*)( p_byte_dest + ( vert * p_grass_mesh->m_vertex_stride ));
//			p_pos->y += ( height_per_layer * ( layer + 1 ));
//
//			float u			= fabsf( p_pos->x ) / 48.0f;
//			float v			= fabsf( p_pos->z ) / 48.0f;
//			float *p_tex	= (float*)( p_byte_dest + p_grass_mesh->m_uv0_offset + ( vert * p_grass_mesh->m_vertex_stride ));
//			p_tex[0]		= u;		
//			p_tex[1]		= v;
//		}

		p_geom->AddMesh( p_grass_mesh );
	}
	
/*	
	
	if( p_mesh->mp_material && p_mesh->mp_material->pTex[0] )
	{
		if( p_mesh->mp_material->Passes == 1 )
		{
			if( p_mesh->mp_material->pTex[0]->Checksum == 0x8d9b6a64 )
				add_grass = true;
		}
	}

	if( !add_grass )
		return false;

	createGrassMats();

	// At this stage we know we want to add grass.
	for( int layer = 0; layer < 6; ++layer )
	{
		Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().BottomUpHeap());
		NxNgc::sMesh *p_grass_mesh = p_mesh->Clone( false );
		Mem::Manager::sHandle().PopContext();

		// No longer any shadow cast on the base mesh.
//		p_mesh->m_flags |= NxNgc::sMesh::MESH_FLAG_NO_SKATER_SHADOW;
		
		if( layer != 2 )
			p_grass_mesh->m_flags |= NxNgc::sMesh::MESH_FLAG_NO_SKATER_SHADOW;
		
		// Now point the mesh to a different material.
		p_grass_mesh->mp_material = p_grass_mats[layer];

		// Go through and move all the vertices up a little, and calculate new texture coordinates based on the x-z space of the vertex positions.
		BYTE *p_byte_dest;
		p_grass_mesh->mp_vertex_buffer[0]->Lock( 0, 0, &p_byte_dest, 0 );

		for( uint32 vert = 0; vert < p_grass_mesh->m_num_vertices; ++vert )
		{
			D3DXVECTOR3 *p_pos = (D3DXVECTOR3*)( p_byte_dest + ( vert * p_grass_mesh->m_vertex_stride ));
			p_pos->y += ( 1.0f * ( layer + 1 ));

			float u			= fabsf( p_pos->x ) / 48.0f;
			float v			= fabsf( p_pos->z ) / 48.0f;
			float *p_tex	= (float*)( p_byte_dest + p_grass_mesh->m_uv0_offset + ( vert * p_grass_mesh->m_vertex_stride ));
			p_tex[0]		= u;		
			p_tex[1]		= v;
		}

		p_geom->AddMesh( p_grass_mesh );
	}
*/
#endif
	return true;
}
