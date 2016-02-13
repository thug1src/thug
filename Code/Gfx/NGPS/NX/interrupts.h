#ifndef __INTERRUPTS_H
#define __INTERRUPTS_H

namespace NxPs2
{

int TextureSyncHandler(int Cause);
int DMATextureSyncHandler(int Cause);
int GifHandler(int Cause);
int GsHandler(int Cause);
int Vif1Handler(int Cause);

extern volatile uint32 UploadStalled, RenderStalled;

// Inlines

// If you need to use floating point in an interrupt, you MUST use the functions saveFloatRegs()
// and restoreFloatRegs() or some equivalent.

//----------------------------------------------------------------------------
//
inline void saveFloatRegs(unsigned int *buffer)
{
    asm volatile ("
      swc1    $f00,0x00(%0)
      swc1    $f01,0x04(%0)
      swc1    $f02,0x08(%0)
      swc1    $f03,0x0c(%0)
      swc1    $f04,0x10(%0)
      swc1    $f05,0x14(%0)
      swc1    $f06,0x18(%0)
      swc1    $f07,0x1c(%0)

      swc1    $f08,0x20(%0)
      swc1    $f09,0x24(%0)
      swc1    $f10,0x28(%0)
      swc1    $f11,0x2c(%0)
      swc1    $f12,0x30(%0)
      swc1    $f13,0x34(%0)
      swc1    $f14,0x38(%0)
      swc1    $f15,0x3c(%0)

      swc1    $f16,0x40(%0)
      swc1    $f17,0x44(%0)
      swc1    $f18,0x48(%0)
      swc1    $f19,0x4c(%0)
      swc1    $f20,0x50(%0)
      swc1    $f21,0x54(%0)
      swc1    $f22,0x58(%0)
      swc1    $f23,0x5c(%0)

      swc1    $f24,0x60(%0)
      swc1    $f25,0x64(%0)
      swc1    $f26,0x68(%0)
      swc1    $f27,0x6c(%0)
      swc1    $f28,0x70(%0)
      swc1    $f29,0x74(%0)
      swc1    $f30,0x78(%0)
      swc1    $f31,0x7c(%0)

    ": : "r" (buffer));
}

//----------------------------------------------------------------------------
//
inline void restoreFloatRegs(unsigned int *buffer)
{
    asm volatile ("
      lwc1    $f00,0x00(%0)
      lwc1    $f01,0x04(%0)
      lwc1    $f02,0x08(%0)
      lwc1    $f03,0x0c(%0)
      lwc1    $f04,0x10(%0)
      lwc1    $f05,0x14(%0)
      lwc1    $f06,0x18(%0)
      lwc1    $f07,0x1c(%0)

      lwc1    $f08,0x20(%0)
      lwc1    $f09,0x24(%0)
      lwc1    $f10,0x28(%0)
      lwc1    $f11,0x2c(%0)
      lwc1    $f12,0x30(%0)
      lwc1    $f13,0x34(%0)
      lwc1    $f14,0x38(%0)
      lwc1    $f15,0x3c(%0)

      lwc1    $f16,0x40(%0)
      lwc1    $f17,0x44(%0)
      lwc1    $f18,0x48(%0)
      lwc1    $f19,0x4c(%0)
      lwc1    $f20,0x50(%0)
      lwc1    $f21,0x54(%0)
      lwc1    $f22,0x58(%0)
      lwc1    $f23,0x5c(%0)

      lwc1    $f24,0x60(%0)
      lwc1    $f25,0x64(%0)
      lwc1    $f26,0x68(%0)
      lwc1    $f27,0x6c(%0)
      lwc1    $f28,0x70(%0)
      lwc1    $f29,0x74(%0)
      lwc1    $f30,0x78(%0)
      lwc1    $f31,0x7c(%0)

    ": : "r" (buffer));
}

} // namespace NxPs2


#endif // __INTERRUPTS_H

