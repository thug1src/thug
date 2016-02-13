#ifndef __NXPS2_LOADSCREEN_H
#define __NXPS2_LOADSCREEN_H


namespace NxPs2
{

// Enable and disable loading bar interrupt
void StartLoadingBar(int frames);
void RemoveLoadingBar();


// Loading bar data
extern int		gLoadBarTotalFrames;	// Number of frames it takes for loading bar to go to 100%
extern int		gLoadBarNumFrames;		// Number of frames so far
extern float	gLoadBarX;				// Bar position
extern float	gLoadBarY;
extern float	gLoadBarWidth;			// Bar size
extern float	gLoadBarHeight;
extern int		gLoadBarStartColor[4];	// 0 = red; 1 = green; 2 = blue; 3 = alpha
extern int		gLoadBarDeltaColor[4];	// 0 = delta red; 1 = delta green; 2 = delta blue; 3 = delta alpha
extern float	gLoadBarBorderWidth;	// Border width
extern float	gLoadBarBorderHeight;	// Border height
extern int		gLoadBarBorderColor[4];	// 0 = red; 1 = green; 2 = blue; 3 = alpha



} // namespace NxPs2

#endif // __NXPS2_LOADSCREEN_H
