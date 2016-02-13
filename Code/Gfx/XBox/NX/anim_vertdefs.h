#ifndef ANIM_VERTDEFS_H
#define ANIM_VERTDEFS_H

// Used in the vertex shader descriptor.
#define VSD_REG_POS                     0
#define VSD_REG_WEIGHTS                 1
#define VSD_REG_INDICES                 2
#define VSD_REG_NORMAL                  3
#define VSD_REG_COLOR                   4
#define VSD_REG_TEXCOORDS0              5
#define VSD_REG_TEXCOORDS1              6
#define VSD_REG_TEXCOORDS2              7
#define VSD_REG_TEXCOORDS3              8

// Input register - used in the vertex shader code.
#define VSIN_REG_POS                    v0
#define VSIN_REG_WEIGHTS                v1
#define VSIN_REG_INDICES                v2
#define VSIN_REG_NORMAL                 v3
#define VSIN_REG_COLOR                  v4
#define VSIN_REG_TEXCOORDS0             v5
#define VSIN_REG_TEXCOORDS1             v6
#define VSIN_REG_TEXCOORDS2             v7
#define VSIN_REG_TEXCOORDS3             v8

// Temporary register - used in the vertex shader code.
#define VSTMP_REG_POS_TMP               r0
#define VSTMP_REG_POS_ACCUM             r1
#define VSTMP_REG_NORMAL_TMP            r2
#define VSTMP_REG_NORMAL_ACCUM          r3
#define VSTMP_REG_MAT0					r8
#define VSTMP_REG_MAT1					r9
#define VSTMP_REG_MAT2					r10

// Vertex shader defines.
#define VSCONST_REG_BASE					-96	// Don't have to worry about the constant -38 and -37, we are not using them.

#define VSCONST_REG_TRANSFORM_OFFSET		VSCONST_REG_BASE
#define VSCONST_REG_TRANSFORM_SIZE			4

#define VSCONST_REG_WORLD_TRANSFORM_OFFSET	VSCONST_REG_TRANSFORM_OFFSET + VSCONST_REG_TRANSFORM_SIZE
#define VSCONST_REG_WORLD_TRANSFORM_SIZE	3

#define VSCONST_REG_DIR_LIGHT_OFFSET		VSCONST_REG_WORLD_TRANSFORM_OFFSET + VSCONST_REG_WORLD_TRANSFORM_SIZE
#define VSCONST_REG_DIR_LIGHT_SIZE			6	// Support for 3 directional lights.

#define VSCONST_REG_AMB_LIGHT_OFFSET		VSCONST_REG_DIR_LIGHT_OFFSET + VSCONST_REG_DIR_LIGHT_SIZE
#define VSCONST_REG_AMB_LIGHT_SIZE			1

#define VSCONST_REG_CAM_POS_OFFSET			VSCONST_REG_AMB_LIGHT_OFFSET + VSCONST_REG_AMB_LIGHT_SIZE
#define VSCONST_REG_CAM_POS_SIZE			1

#define VSCONST_REG_SPECULAR_COLOR_OFFSET	VSCONST_REG_CAM_POS_OFFSET + VSCONST_REG_CAM_POS_SIZE
#define VSCONST_REG_SPECULAR_COLOR_SIZE		1

#define VSCONST_REG_UV_MAT_OFFSET			VSCONST_REG_SPECULAR_COLOR_OFFSET + VSCONST_REG_SPECULAR_COLOR_SIZE
#define VSCONST_REG_UV_MAT_SIZE				8


//#define VSCONST_REG_MATRIX_OFFSET			( VSCONST_REG_UV_MAT_OFFSET + VSCONST_REG_UV_MAT_SIZE )
#define VSCONST_REG_MATRIX_OFFSET			-72

// Constant registers - used in the vertex shader code.
#define VSCONST_REG_TRANSFORM_X					c[0 + VSCONST_REG_BASE]
#define VSCONST_REG_TRANSFORM_Y					c[1 + VSCONST_REG_BASE]
#define VSCONST_REG_TRANSFORM_Z					c[2 + VSCONST_REG_BASE]
#define VSCONST_REG_TRANSFORM_W					c[3 + VSCONST_REG_BASE]
#define VSCONST_REG_WORLD_TRANSFORM_X			c[4 + VSCONST_REG_BASE]
#define VSCONST_REG_WORLD_TRANSFORM_Y			c[5 + VSCONST_REG_BASE]
#define VSCONST_REG_WORLD_TRANSFORM_Z			c[6 + VSCONST_REG_BASE]

#define VSCONST_REG_LIGHT_DIR0					c[7 + VSCONST_REG_BASE]
#define VSCONST_REG_LIGHT_COLOR0				c[8 + VSCONST_REG_BASE]
#define VSCONST_REG_LIGHT_DIR1					c[9 + VSCONST_REG_BASE]
#define VSCONST_REG_LIGHT_COLOR1				c[10 + VSCONST_REG_BASE]
#define VSCONST_REG_LIGHT_DIR2					c[11 + VSCONST_REG_BASE]
#define VSCONST_REG_LIGHT_COLOR2				c[12 + VSCONST_REG_BASE]

// Shadowbuffer transform shares space with lighting params, since both not required at the same time.
#define VSCONST_REG_SHADOWBUFFER_TRANSFORM_X	c[7 + VSCONST_REG_BASE]
#define VSCONST_REG_SHADOWBUFFER_TRANSFORM_Y	c[8 + VSCONST_REG_BASE]
#define VSCONST_REG_SHADOWBUFFER_TRANSFORM_Z	c[9 + VSCONST_REG_BASE]
#define VSCONST_REG_SHADOWBUFFER_TRANSFORM_W	c[10 + VSCONST_REG_BASE]

#define VSCONST_REG_AMB_LIGHT_COLOR				c[13 + VSCONST_REG_BASE]
#define VSCONST_REG_CAM_POS						c[14 + VSCONST_REG_BASE]
#define VSCONST_REG_SPECULAR_COLOR				c[15 + VSCONST_REG_BASE]
#define VSCONST_REG_UV_MAT00					c[16 + VSCONST_REG_BASE]
#define VSCONST_REG_UV_MAT01					c[17 + VSCONST_REG_BASE]
#define VSCONST_REG_UV_MAT10					c[18 + VSCONST_REG_BASE]
#define VSCONST_REG_UV_MAT11					c[19 + VSCONST_REG_BASE]
#define VSCONST_REG_UV_MAT20					c[20 + VSCONST_REG_BASE]
#define VSCONST_REG_UV_MAT21					c[21 + VSCONST_REG_BASE]
#define VSCONST_REG_UV_MAT30					c[22 + VSCONST_REG_BASE]
#define VSCONST_REG_UV_MAT31					c[23 + VSCONST_REG_BASE]

#endif // ANIM_VERTDEFS_H