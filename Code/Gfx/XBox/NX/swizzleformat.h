//////////////////////////////////////////////////////////////////////
// Swizzled to linear and back format mapping.
//
//  Copyright (C) 2001 Microsoft Corporation
//  All rights reserved.
//////////////////////////////////////////////////////////////////////
inline D3DFORMAT MapLinearToSwizzledFormat(D3DFORMAT fmt)
{
    switch (fmt)
    {
    case D3DFMT_LIN_A1R5G5B5:     return D3DFMT_A1R5G5B5;
    case D3DFMT_LIN_A4R4G4B4:     return D3DFMT_A4R4G4B4;
    case D3DFMT_LIN_A8:         return D3DFMT_A8;
    case D3DFMT_LIN_A8B8G8R8:     return D3DFMT_A8B8G8R8;
    case D3DFMT_LIN_A8R8G8B8:     return D3DFMT_A8R8G8B8;
    case D3DFMT_LIN_B8G8R8A8:     return D3DFMT_B8G8R8A8;
    case D3DFMT_LIN_G8B8:         return D3DFMT_G8B8;
    case D3DFMT_LIN_R4G4B4A4:     return D3DFMT_R4G4B4A4;
    case D3DFMT_LIN_R5G5B5A1:     return D3DFMT_R5G5B5A1;
    case D3DFMT_LIN_R5G6B5:     return D3DFMT_R5G6B5;
    case D3DFMT_LIN_R6G5B5:     return D3DFMT_R6G5B5;
    case D3DFMT_LIN_R8B8:         return D3DFMT_R8B8;
    case D3DFMT_LIN_R8G8B8A8:     return D3DFMT_R8G8B8A8;
    case D3DFMT_LIN_X1R5G5B5:     return D3DFMT_X1R5G5B5;
    case D3DFMT_LIN_X8R8G8B8:     return D3DFMT_X8R8G8B8;
    case D3DFMT_LIN_A8L8:         return D3DFMT_A8L8;
    case D3DFMT_LIN_AL8:         return D3DFMT_AL8;
    case D3DFMT_LIN_L16:         return D3DFMT_L16;
    case D3DFMT_LIN_L8:         return D3DFMT_L8;
    case D3DFMT_LIN_V16U16:     return D3DFMT_V16U16;
    case D3DFMT_LIN_D24S8:         return D3DFMT_D24S8;
    case D3DFMT_LIN_F24S8:         return D3DFMT_F24S8;
    case D3DFMT_LIN_D16:         return D3DFMT_D16;
    case D3DFMT_LIN_F16:         return D3DFMT_F16;
    default:
        return fmt;
    }
}

inline D3DFORMAT MapSwizzledToLinearFormat(D3DFORMAT fmt)
{
    switch (fmt)
    {
    case D3DFMT_A1R5G5B5:     return D3DFMT_LIN_A1R5G5B5;
    case D3DFMT_A4R4G4B4:     return D3DFMT_LIN_A4R4G4B4;
    case D3DFMT_A8:         return D3DFMT_LIN_A8;
    case D3DFMT_A8B8G8R8:     return D3DFMT_LIN_A8B8G8R8;
    case D3DFMT_A8R8G8B8:     return D3DFMT_LIN_A8R8G8B8;
    case D3DFMT_B8G8R8A8:     return D3DFMT_LIN_B8G8R8A8;
    case D3DFMT_G8B8:         return D3DFMT_LIN_G8B8;
    case D3DFMT_R4G4B4A4:     return D3DFMT_LIN_R4G4B4A4;
    case D3DFMT_R5G5B5A1:     return D3DFMT_LIN_R5G5B5A1;
    case D3DFMT_R5G6B5:     return D3DFMT_LIN_R5G6B5;
    case D3DFMT_R6G5B5:     return D3DFMT_LIN_R6G5B5;
    case D3DFMT_R8B8:         return D3DFMT_LIN_R8B8;
    case D3DFMT_R8G8B8A8:     return D3DFMT_LIN_R8G8B8A8;
    case D3DFMT_X1R5G5B5:     return D3DFMT_LIN_X1R5G5B5;
    case D3DFMT_X8R8G8B8:     return D3DFMT_LIN_X8R8G8B8;
    case D3DFMT_A8L8:         return D3DFMT_LIN_A8L8;
    case D3DFMT_AL8:         return D3DFMT_LIN_AL8;
    case D3DFMT_L16:         return D3DFMT_LIN_L16;
    case D3DFMT_L8:         return D3DFMT_LIN_L8;
    case D3DFMT_V16U16:     return D3DFMT_LIN_V16U16;
    case D3DFMT_D24S8:         return D3DFMT_LIN_D24S8;
    case D3DFMT_F24S8:         return D3DFMT_LIN_F24S8;
    case D3DFMT_D16:         return D3DFMT_LIN_D16;
    case D3DFMT_F16:         return D3DFMT_LIN_F16;
    default:
        return fmt;
    }
}
