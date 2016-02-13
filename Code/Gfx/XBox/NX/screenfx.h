#ifndef __SCREENFX_H
#define __SCREENFX_H

namespace NxXbox
{
	void	start_screen_blur( void );
	void	finish_screen_blur( void );

	void	start_focus_blur( void );
	void	finish_focus_blur( void );
	void	set_focus_blur_focus( Mth::Vector & focal_point, float offset, float near_depth, float far_depth );

	void	draw_rain( void );
} // namespace NxXbox

#endif // __SCREENFX_H
