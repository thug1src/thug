//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Register layout:                                                     //
//                                                                      //
// Register			  | Type		| Used for:                         //
// -------------------+-------------+---------------------------------- //
// R0				  | Volatile	| Language Specific                 //
// R1				  | Dedicated	| Stack Pointer (SP)                //
// R2				  | Dedicated	| Read-only small data area anchor  //
// R3 - R4			  | Volatile	| Parameter passing / return values //
// R5 - R10			  | Volatile	| Parameter passing                 //
// R11 - R12		  | Volatile    |                                   //
// R13				  | Dedicated	| Read-write small data area anchor //
// R14 - R31		  | Nonvolatile |                                   //
// F0				  | Volatile	| Language specific                 //
// F1				  | Volatile	| Parameter passing / return values //
// F2 - F8			  | Volatile	| Parameter passing                 //
// F9 - F13			  | Volatile    |                                   //
// F14 - F31		  | Nonvolatile |                                   //
// Fields CR2 - CR4	  | Nonvolatile |                                   //
// Other CR fields	  | Volatile    |                                   //
// Other registers	  | Volatile    |                                   //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#	.include "core/defs.s"			# equates

	.section .code

#	.extern blah

	.equ	SKN_GQR_POS,6
	.equ	SKN_GQR_NORM,7

	.equ	SKN_GQR_UV,0

	.equ	SKN_GQR_U8,2
	.equ	SKN_GQR_U16,3
	.equ	SKN_GQR_S8,4
	.equ	SKN_GQR_S16,5

#---------------------------------------------------------------------------
# Particle rendering.
#---------------------------------------------------------------------------

	.equ	m_num_particles,	(4*0)
	.equ	m_rate,             (4*1)
	.equ	m_interval,         (4*2)
	.equ	m_oldest_age,       (4*3)
//	.equ	m_rand_seed,        (4*4)
//	.equ	m_rand_a,           (4*5)
//	.equ	m_rand_b,           (4*6)
	.equ	m_rand_current,		(4*4)

	.scope
	.global	RenderNewParticles
RenderNewParticles:
	# Input parameters.
	.equr		@p_stream,		%r3				# CParticleStream* p_stream
	.equr		@lifetime,		%f1				# float lifetime
	.equr		@midpercent,	%f2				# float midpercent
	.equr		@use_mid_color,	%r4				# bool use_mid_color
	.equr		@p_color0,		%r5				# GXColor p_color0
	.equr		@s0,			%r6				# Mth::Vector * s0
	.equr		@sr,			%r7				# Mth::Vector * sr
	.equr		@su,			%r8				# Mth::Vector * su
	.equr		@p_params,		%r9				# float * p_params
	.equr		@nearz,			%f3				# float nearz

	.equr		@t2,			%f2				# Temporary
	.equr		@t,				%f31
	.equr		@midpoint_time,	%f4
	.equr		@fone,			%f5
	.equr		@rXY,			%f6
	.equr		@rZW,			%f7
	.equr		@s0XY,			%f8
	.equr		@s0ZW,			%f9
	.equr		@s1XY,			%f10
	.equr		@s1ZW,			%f11
	.equr		@s2XY,			%f12
	.equr		@s2ZW,			%f13
	.equr		@p0XY,			%f14
	.equr		@p0ZW,			%f15
	.equr		@p1XY,			%f16
	.equr		@p1ZW,			%f17
	.equr		@p2XY,			%f18
	.equr		@p2ZW,			%f19
	.equr		@srXY,			%f20
	.equr		@srZW,			%f21
	.equr		@suXY,			%f22
	.equr		@suZW,			%f23
	.equr		@posXY,			%f24
	.equr		@posZW,			%f25
	.equr		@interval,		%f26
//	.equr		@v0XY,			%f27
//	.equr		@v0ZW,			%f28
//	.equr		@v1XY,			%f29
//	.equr		@v1ZW,			%f30
//	.equr		@fzero,			%f31

//	.equr		@srsXY,			@t2
//	.equr		@srsZW,			@lifetime
//	.equr		@susXY,			@rXY
//	.equr		@susZW,			@rZW
	.equr		@srsXY,			%f27
	.equr		@srsZW,			%f28
	.equr		@susXY,			%f29
	.equr		@susZW,			%f30

	.equr		@quad,			%r6
	.equr		@four,			%r7
	.equr		@rand_current,	%r8
//	.equr		@bit_pattern,	%r8
	.equr		@p_rand,		%r14
	.equr		@zero,			%r10
	.equr		@xfreg,			%r11
	.equr		@xfaddr,		%r12
//	.equr		@color0,		%r11
//	.equr		@color1,		%r12
//	.equr		@color2,		%r14
	.equr		@fifo,			%r15
	.equr		@temp,			%r16
	.equr		@bp,			%r17
	.equr		@psize,			%r18
	.equr		@point,			%r19
	.equr		@one,			%r20

	stwu    %r1, -256(%r1)
	stfd    %f14, ( 2*4)(%r1)
	stfd    %f15, ( 4*4)(%r1)
	stfd    %f16, ( 6*4)(%r1)
	stfd    %f17, ( 8*4)(%r1)
	stfd    %f18, (10*4)(%r1)
	stfd    %f19, (12*4)(%r1)
	stfd    %f20, (14*4)(%r1)
	stfd    %f21, (16*4)(%r1)
	stfd    %f22, (18*4)(%r1)
	stfd    %f23, (20*4)(%r1)
	stfd    %f24, (22*4)(%r1)
	stfd    %f25, (24*4)(%r1)
	stfd    %f26, (26*4)(%r1)
	stfd    %f27, (28*4)(%r1)
	stfd    %f28, (30*4)(%r1)
	stfd    %f29, (32*4)(%r1)
	stfd    %f30, (34*4)(%r1)
	stfd    %f31, (36*4)(%r1)
	stw		%r14, (38*4)(%r1)  
	stw		%r15, (39*4)(%r1)  
	stw		%r16, (40*4)(%r1)  
	stw		%r17, (41*4)(%r1)  
	stw		%r18, (42*4)(%r1)  
	stw		%r19, (43*4)(%r1)  
	stw		%r20, (44*4)(%r1)  
						   
	// Load up immediate values.
	psq_l		@s0XY,			0(@s0),						0,				SKN_GQR_UV
	psq_l		@s0ZW,			8(@s0),						0,				SKN_GQR_UV
	psq_l		@s1XY,			16(@s0),					0,				SKN_GQR_UV
	psq_l		@s1ZW,			24(@s0),					0,				SKN_GQR_UV
	psq_l		@s2XY,			32(@s0),					0,				SKN_GQR_UV
	psq_l		@s2ZW,			40(@s0),					0,				SKN_GQR_UV

	psq_l		@p0XY,			48(@s0),					0,				SKN_GQR_UV
	psq_l		@p0ZW,			56(@s0),					0,				SKN_GQR_UV
	psq_l		@p1XY,			64(@s0),					0,				SKN_GQR_UV
	psq_l		@p1ZW,			72(@s0),					0,				SKN_GQR_UV
	psq_l		@p2XY,			80(@s0),					0,				SKN_GQR_UV
	psq_l		@p2ZW,			88(@s0),					0,				SKN_GQR_UV

	psq_l		@suXY,			0(@su),						0,				SKN_GQR_UV
	psq_l		@suZW,			8(@su),						0,				SKN_GQR_UV
	psq_l		@srXY,			0(@sr),						0,				SKN_GQR_UV
	psq_l		@srZW,			8(@sr),						0,				SKN_GQR_UV

	lis			@temp,			@six_data@h
	ori			@temp,			@temp,						@six_data@l
	psq_l		@t,				0(@temp),					0,				SKN_GQR_UV
	ps_merge00	@nearz,			@nearz,						@t		// nearz now has 6 in 1.

//	lwz			@color0,		0(@p_color0)
//	lwz			@color1,		4(@p_color0)
//	lwz			@color2,		8(@p_color0)

	lis			@temp,			@one_data@h
	ori			@temp,			@temp,						@one_data@l
	psq_l		@fone,			0(@temp),					0,				SKN_GQR_UV
//	ps_sub		@fzero,			@fone,						@fone

//	lis			@bit_pattern,	0x1010
//	ori			@bit_pattern,	@bit_pattern,				0x1010

	lis			@p_rand,		@rand_data@h
	ori			@p_rand,		@p_rand,					@rand_data@l

	lis			@fifo,			0xcc01
	addi		@fifo,			@fifo,						-0x8000

	ps_merge00	@midpercent,	@midpercent,				@midpercent
	ps_merge00	@lifetime,		@lifetime,					@lifetime

	li			@zero,			0x0
	li			@quad,			0x80
	li			@four,			4
	li			@xfreg,			0x10
	li			@xfaddr,		0x100c
	li			@bp,			0x61
	lis			@psize,			0x2228
	li			@point,			0xb8
	li			@one,			1

	psq_l		@interval,		m_interval(@p_stream),		1,				SKN_GQR_UV
	ps_merge00	@interval,		@interval,					@interval

//	float t				= p_stream->m_oldest_age;
	psq_l		@t,				m_oldest_age(@p_stream),	1,				SKN_GQR_UV
	ps_merge00	@t,				@t,							@t

//	float midpoint_time = m_params.m_Lifetime * ( m_params.m_ColorMidpointPct * 0.01f );
	lis			@temp,			@tenth_data@h
	ori			@temp,			@temp,						@tenth_data@l
	psq_l		@midpoint_time,	0(@temp),					0,				SKN_GQR_UV
	ps_mul		@midpoint_time,	@midpoint_time,				@midpercent
	ps_mul		@midpoint_time,	@midpoint_time,				@lifetime

//	seed_particle_rnd( p_stream->m_rand_seed, p_stream->m_rand_a, p_stream->m_rand_b );
//	lwz			@rand_seed,		m_rand_seed(@p_stream)
//	lwz			@rand_a,		m_rand_a(@p_stream)
//	lwz			@rand_b,		m_rand_b(@p_stream)
	lwz			@rand_current,	m_rand_current(@p_stream)

//	for( int p = 0; p < p_stream->m_num_particles; ++p )
	lwz			@temp,			m_num_particles(@p_stream)
	mtctr   	@temp

@particle_loop:
//		Mth::Vector r( 1.0f + ((float)particle_rnd( 16384 ) / 16384 ), 
//		1.0f + ((float)particle_rnd( 16384 ) / 16384 ),
//		1.0f + ((float)particle_rnd( 16384 ) / 16384 ),
//		1.0f + ((float)particle_rnd( 16384 ) / 16384 ));

//		rand_seed	= rand_seed * rand_a + rand_b;
//		rand_a		= ( rand_a ^ rand_seed ) + ( rand_seed >> 4 );
//		rand_b		+= ( rand_seed >> 3 ) - 0x10101010L;
//		return (int)(( rand_seed & 0xffff ) * n ) >> 16;

//	mullw		@temp,			@rand_seed,					@rand_a
//	add			@rand_seed,		@temp,						@rand_b
//	xor			@rand_a,		@rand_seed,					@rand_a
//	srwi		@temp,			@rand_seed,					4
//	add			@rand_a,		@rand_a,					@temp
//	srwi		@temp,			@rand_seed,					3
//	sub			@temp,			@temp,						@bit_pattern
//	add			@rand_b,		@rand_b,					@temp
//	andi.		@temp,			@rand_seed,					0xffff
//	srwi		@temp,			@temp,						2
//	sth			@temp,			0(@p_rand)
//
//	mullw		@temp,			@rand_seed,					@rand_a
//	add			@rand_seed,		@temp,						@rand_b
//	xor			@rand_a,		@rand_seed,					@rand_a
//	srwi		@temp,			@rand_seed,					4
//	add			@rand_a,		@rand_a,					@temp
//	srwi		@temp,			@rand_seed,					3
//	sub			@temp,			@temp,						@bit_pattern
//	add			@rand_b,		@rand_b,					@temp
//	andi.		@temp,			@rand_seed,					0xffff
//	srwi		@temp,			@temp,						2
//	sth			@temp,			2(@p_rand)
//
//	mullw		@temp,			@rand_seed,					@rand_a
//	add			@rand_seed,		@temp,						@rand_b
//	xor			@rand_a,		@rand_seed,					@rand_a
//	srwi		@temp,			@rand_seed,					4
//	add			@rand_a,		@rand_a,					@temp
//	srwi		@temp,			@rand_seed,					3
//	sub			@temp,			@temp,						@bit_pattern
//	add			@rand_b,		@rand_b,					@temp
//	andi.		@temp,			@rand_seed,					0xffff
//	srwi		@temp,			@temp,						2
//	sth			@temp,			4(@p_rand)
//
//	mullw		@temp,			@rand_seed,					@rand_a
//	add			@rand_seed,		@temp,						@rand_b
//	xor			@rand_a,		@rand_seed,					@rand_a
//	srwi		@temp,			@rand_seed,					4
//	add			@rand_a,		@rand_a,					@temp
//	srwi		@temp,			@rand_seed,					3
//	sub			@temp,			@temp,						@bit_pattern
//	add			@rand_b,		@rand_b,					@temp
//	andi.		@temp,			@rand_seed,					0xffff
//	srwi		@temp,			@temp,						2
//	sth			@temp,			6(@p_rand)


//			if( m_params.m_UseMidcolor )
//			{
//				if( t > midpoint_time )
//				{
//					color_interpolator	= ( t - midpoint_time ) * ReciprocalEstimate_ASM( m_params.m_Lifetime - midpoint_time );
//					col0				= m_params.m_Color[1];
//					col1				= m_params.m_Color[2];
//				}
//				else
//				{
//					color_interpolator	= t * ReciprocalEstimate_ASM( midpoint_time );
//					col0				= m_params.m_Color[0];
//					col1				= m_params.m_Color[1];
//				}
//			}
//			else 
//			{
//				color_interpolator		= t * ReciprocalEstimate_ASM( m_params.m_Lifetime );
//				col0					= m_params.m_Color[0];
//				col1					= m_params.m_Color[2];
//			}
	andi.		@temp,			@use_mid_color,				1
	beq			@no_mid

	ps_cmpu0	%cr0,			@t,							@midpoint_time
	ble			@after_mid
// before midpoint
	ps_sub		@t2,			@lifetime,					@midpoint_time
	ps_res		@t2,			@t2
	ps_sub		@rXY,			@t,							@midpoint_time 
	ps_mul		@t2,			@t2,						@rXY
	psq_l		@susXY,			4(@p_color0),				0,				SKN_GQR_U8
	psq_l		@susZW,			6(@p_color0),				0,				SKN_GQR_U8
	psq_l		@srsXY,			8(@p_color0),				0,				SKN_GQR_U8
	psq_l		@srsZW,			10(@p_color0),				0,				SKN_GQR_U8

	b			@over_mid
@after_mid:
// after midpoint
	ps_res		@t2,			@midpoint_time
	ps_mul		@t2,			@t2,						@t
	psq_l		@susXY,			0(@p_color0),				0,				SKN_GQR_U8
	psq_l		@susZW,			2(@p_color0),				0,				SKN_GQR_U8
	psq_l		@srsXY,			4(@p_color0),				0,				SKN_GQR_U8
	psq_l		@srsZW,			6(@p_color0),				0,				SKN_GQR_U8

	b			@over_mid

@no_mid:
// no midpoint.
	ps_res		@t2,			@lifetime
	ps_mul		@t2,			@t2,						@t
	psq_l		@susXY,			0(@p_color0),				0,				SKN_GQR_U8
	psq_l		@susZW,			2(@p_color0),				0,				SKN_GQR_U8
	psq_l		@srsXY,			8(@p_color0),				0,				SKN_GQR_U8
	psq_l		@srsZW,			10(@p_color0),				0,				SKN_GQR_U8

@over_mid:
	ps_sub		@rXY,			@srsXY,						@susXY
	ps_sub		@rZW,			@srsZW,						@susZW
	ps_madd		@rXY,			@rXY,						@t2,			@susXY
	ps_madd		@rZW,			@rZW,						@t2,            @susZW

// Store material color here.
	stb			@xfreg,			0(@fifo)
	sth			@zero,			0(@fifo)
	sth			@xfaddr,		0(@fifo)
	ps_merge11	@t2,			@rXY,						@rXY
	psq_st		@rXY,			0(@fifo),					1,				SKN_GQR_U8
	psq_st		@t2,			0(@fifo),					1,				SKN_GQR_U8
	ps_merge11	@t2,			@rZW,						@rZW
	psq_st		@rZW,			0(@fifo),					1,				SKN_GQR_U8
	psq_st		@t2,			0(@fifo),					1,				SKN_GQR_U8

//		Mth::Vector r;
//		r[X] = 1.0f + ((float)( rand_current = ( ( rand_current & 1 ) ? ( rand_current >> 1 ) ^ 0x3500 : ( rand_current >> 1 ) ) ) / 16384.0f );
//		r[Y] = 1.0f + ((float)( rand_current = ( ( rand_current & 1 ) ? ( rand_current >> 1 ) ^ 0x3500 : ( rand_current >> 1 ) ) ) / 16384.0f );
//		r[Z] = 1.0f + ((float)( rand_current = ( ( rand_current & 1 ) ? ( rand_current >> 1 ) ^ 0x3500 : ( rand_current >> 1 ) ) ) / 16384.0f );
//		r[W] = 1.0f + ((float)( rand_current = ( ( rand_current & 1 ) ? ( rand_current >> 1 ) ^ 0x3500 : ( rand_current >> 1 ) ) ) / 16384.0f );

	andi.		@temp,			@rand_current,				1
	srwi		@rand_current,	@rand_current,				1		// Doesn't affect CR
	beq			@over_0
	xori		@rand_current,	@rand_current,				0x3500
@over_0:
	sth			@rand_current,	0(@p_rand)

	andi.		@temp,			@rand_current,				1
	srwi		@rand_current,	@rand_current,				1		// Doesn't affect CR
	beq			@over_1
	xori		@rand_current,	@rand_current,				0x3500
@over_1:
	sth			@rand_current,	2(@p_rand)

	andi.		@temp,			@rand_current,				1
	srwi		@rand_current,	@rand_current,				1		// Doesn't affect CR
	beq			@over_2
	xori		@rand_current,	@rand_current,				0x3500
@over_2:
	sth			@rand_current,	4(@p_rand)

	andi.		@temp,			@rand_current,				1
	srwi		@rand_current,	@rand_current,				1		// Doesn't affect CR
	beq			@over_3
	xori		@rand_current,	@rand_current,				0x3500
@over_3:
	sth			@rand_current,	6(@p_rand)

	psq_l		@rXY, 			0(@p_rand),					0,				SKN_GQR_NORM
	psq_l		@rZW, 			4(@p_rand),					0,				SKN_GQR_NORM
	ps_add		@rXY,			@rXY,						@fone
	ps_add		@rZW,			@rZW,						@fone

//	Mth::Vector pos = ( m_p0 + ( t * m_p1 ) + (( t * t ) * m_p2 )) + ( m_s0 + ( t * m_s1 ) + (( t * t ) * m_s2 )).Scale( r );
//pos = ( m_p0 +
//		( t * m_p1 ) +
//		(( t * t ) * m_p2 )) +
//
//		( m_s0 +
//		( t * m_s1 ) +
//		(( t * t ) * m_s2 ))
//		.Scale( r );
	
	ps_mul		@t2,			@t,							@t

	ps_madd		@posXY,			@t,							@s1XY,			@s0XY
	ps_madd		@posZW,			@t,							@s1ZW,			@s0ZW
	ps_madd		@posXY,			@t2,						@s2XY,			@posXY
	ps_madd		@posZW,			@t2,						@s2ZW,			@posZW

	ps_mul		@posXY,			@posXY,						@rXY
	ps_mul		@posZW,			@posZW,						@rZW

	ps_madd		@rXY,			@t,							@p1XY,			@p0XY
	ps_madd		@rZW,			@t,							@p1ZW,			@p0ZW
	ps_madd		@rXY,			@t2,						@p2XY,			@rXY
	ps_madd		@rZW,			@t2,						@p2ZW,			@rZW

	ps_add		@posXY,			@posXY,						@rXY
	ps_add		@posZW,			@posZW,						@rZW


	.equr		@mXY,			@srsXY
	.equr		@mWZ,			@srsZW
	.equr		@pm1vp2,		@susXY
	.equr		@ftemp,			@susZW

//	float z = p_mtx->getAtX()*pos[X] + p_mtx->getAtY()*pos[Y] + p_mtx->getAtZ()*pos[Z] + p_mtx->getPosZ();
	psq_l		@mXY, 			0(@p_params),				0,				SKN_GQR_UV
	psq_l		@mWZ, 			8(@p_params),				0,				SKN_GQR_UV

	ps_mul		@mXY,			@posXY,						@mXY
	ps_madds1	@mWZ,			@posZW,						@mWZ,			@mWZ
	ps_add		@mWZ,			@mWZ,						@mXY
	ps_merge11  @mXY,			@mXY,						@mXY
	ps_add		@mWZ,			@mWZ,						@mXY			// mZW0 = z
	ps_neg		@mWZ,			@mWZ                                        // mZW0 = -z

//	float xc = pos[W] * pm[1];	// + z * pm[2];
	psq_l		@pm1vp2,		16(@p_params),				0,				SKN_GQR_UV

	ps_muls1	@ftemp,			@pm1vp2,					@posZW			// @ftemp0=pm1*w
	ps_div		@mXY,			@fone,						@mWZ			// @mXY0=1/z
	ps_muls1	@ftemp,			@ftemp,						@pm1vp2			// @ftemp0=xc*vp2
	ps_mul		@ftemp,			@ftemp,						@mXY			// sc
	
	ps_muls1	@mXY,			@ftemp,						@nearz			// sc * 6
	psq_st		@mXY,			0(@p_rand),					1,				5 // s16, no scaling.
	lhz			@temp,			0(@p_rand)
	cmpi		%cr0,			@temp,						256
	bge			@notile

	slwi		@temp,			@temp,						8
	or			@temp,			@psize,						@temp

	stb			@bp,			0(@fifo)
	stw			@temp,			0(@fifo)
	stb			@point,			0(@fifo)
	sth			@one,			0(@fifo)

	ps_sub		@t2,			@fone,						@fone
	psq_st		@posXY,			0(@fifo),					1,				SKN_GQR_UV
	ps_merge11	@rXY,			@posXY,						@posXY
	psq_st		@rXY,			0(@fifo),					1,				SKN_GQR_UV
	psq_st		@posZW,			0(@fifo),					1,				SKN_GQR_UV
	psq_st		@t2,			0(@fifo),					1,				SKN_GQR_UV
	psq_st		@t2,			0(@fifo),					1,				SKN_GQR_UV

	b			@skip

@notile:
	ps_cmpu0	%cr0,			@ftemp,						@nearz
	ble			@ok

//	pos[W] = ( pos[W] * 256.0f ) / sc;
	ps_muls1	@mXY,			@nearz,						@posZW
	ps_div		@mXY,			@mXY,						@ftemp
	ps_merge00	@posZW,			@posZW,						@mXY
@ok:

//	NsVector sr;
//	sr.x = screen_right.x * pos[W];
//	sr.y = screen_right.y * pos[W];
//	sr.z = screen_right.z * pos[W];
//	NsVector su;
//	su.x = screen_up.x * pos[W];
//	su.y = screen_up.y * pos[W];
//	su.z = screen_up.z * pos[W];
	ps_merge11  @susZW,			@posZW,						@posZW
	ps_mul		@srsXY,			@srXY,						@susZW
	ps_mul		@srsZW,			@srZW,						@susZW
	ps_mul		@susXY,			@suXY,						@susZW
	ps_mul		@susZW,			@suZW,						@susZW

	stb			@quad,			0(@fifo)
	sth			@four,			0(@fifo)

	ps_add		@posXY,			@posXY,						@susXY
	ps_add		@posZW,			@posZW,						@susZW
	ps_sub		@posXY,			@posXY,						@srsXY
	ps_sub		@posZW,			@posZW,						@srsZW

	ps_add		@susXY,			@susXY,						@susXY
	ps_add		@susZW,			@susZW,						@susZW
	ps_add		@srsXY,			@srsXY,						@srsXY
	ps_add		@srsZW,			@srsZW,						@srsZW

	ps_sub		@t2,			@fone,						@fone
	// Store pos, 0,0
	psq_st		@posXY,			0(@fifo),					1,				SKN_GQR_UV
	ps_merge11	@rXY,			@posXY,						@posXY
	psq_st		@rXY,			0(@fifo),					1,				SKN_GQR_UV
	psq_st		@posZW,			0(@fifo),					1,				SKN_GQR_UV
	psq_st		@t2,			0(@fifo),					1,				SKN_GQR_UV
	psq_st		@t2,			0(@fifo),					1,				SKN_GQR_UV

	ps_add		@posXY,			@posXY,						@srsXY
	ps_add		@posZW,			@posZW,						@srsZW
	// Store pos, 1,0
	psq_st		@posXY,			0(@fifo),					1,				SKN_GQR_UV
	ps_merge11	@rXY,			@posXY,						@posXY
	psq_st		@rXY,			0(@fifo),					1,				SKN_GQR_UV
	psq_st		@posZW,			0(@fifo),					1,				SKN_GQR_UV
	psq_st		@fone,			0(@fifo),					1,				SKN_GQR_UV
	psq_st		@t2,				0(@fifo),					1,				SKN_GQR_UV

	ps_sub		@posXY,			@posXY,						@susXY
	ps_sub		@posZW,			@posZW,						@susZW
	// Store pos, 1,1
	psq_st		@posXY,			0(@fifo),					1,				SKN_GQR_UV
	ps_merge11	@rXY,			@posXY,						@posXY
	psq_st		@rXY,			0(@fifo),					1,				SKN_GQR_UV
	psq_st		@posZW,			0(@fifo),					1,				SKN_GQR_UV
	psq_st		@fone,			0(@fifo),					1,				SKN_GQR_UV
	psq_st		@fone,			0(@fifo),					1,				SKN_GQR_UV

	ps_sub		@posXY,			@posXY,						@srsXY
	ps_sub		@posZW,			@posZW,						@srsZW
	// Store pos, 0,1
	psq_st		@posXY,			0(@fifo),					1,				SKN_GQR_UV
	ps_merge11	@rXY,			@posXY,						@posXY
	psq_st		@rXY,			0(@fifo),					1,				SKN_GQR_UV
	psq_st		@posZW,			0(@fifo),					1,				SKN_GQR_UV
	psq_st		@t2,			0(@fifo),					1,				SKN_GQR_UV
	psq_st		@fone,			0(@fifo),					1,				SKN_GQR_UV



//	ps_add		@v1XY,			@posXY,						@susXY
//	ps_add		@v1ZW,			@posZW,						@susZW
//	ps_sub		@v0XY,			@v1XY,						@srsXY
//	ps_sub		@v0ZW,			@v1ZW,						@srsZW
//	// Store pos, 0,0
//	psq_st		@v0XY,			0(@fifo),					1,				SKN_GQR_UV
//	ps_merge11	@v0XY,			@v0XY,						@v0XY
//	psq_st		@v0XY,			0(@fifo),					1,				SKN_GQR_UV
//	psq_st		@v0ZW,			0(@fifo),					1,				SKN_GQR_UV
//	psq_st		@fzero,			0(@fifo),					1,				SKN_GQR_UV
//	psq_st		@fzero,			0(@fifo),					1,				SKN_GQR_UV
//
//	ps_add		@v0XY,			@v1XY,						@srsXY
//	ps_add		@v0ZW,			@v1ZW,						@srsZW
//	// Store pos, 1,0
//	psq_st		@v0XY,			0(@fifo),					1,				SKN_GQR_UV
//	ps_merge11	@v0XY,			@v0XY,						@v0XY
//	psq_st		@v0XY,			0(@fifo),					1,				SKN_GQR_UV
//	psq_st		@v0ZW,			0(@fifo),					1,				SKN_GQR_UV
//	psq_st		@fone,			0(@fifo),					1,				SKN_GQR_UV
//	psq_st		@fzero,			0(@fifo),					1,				SKN_GQR_UV
//
//	ps_sub		@v1XY,			@posXY,						@susXY
//	ps_sub		@v1ZW,			@posZW,						@susZW
//	ps_add		@v0XY,			@v1XY,						@srsXY
//	ps_add		@v0ZW,			@v1ZW,						@srsZW
//	// Store pos, 1,1
//	psq_st		@v0XY,			0(@fifo),					1,				SKN_GQR_UV
//	ps_merge11	@v0XY,			@v0XY,						@v0XY
//	psq_st		@v0XY,			0(@fifo),					1,				SKN_GQR_UV
//	psq_st		@v0ZW,			0(@fifo),					1,				SKN_GQR_UV
//	psq_st		@fone,			0(@fifo),					1,				SKN_GQR_UV
//	psq_st		@fone,			0(@fifo),					1,				SKN_GQR_UV
//
//	ps_sub		@v0XY,			@v1XY,						@srsXY
//	ps_sub		@v0ZW,			@v1ZW,						@srsZW
//	// Store pos, 0,1
//	psq_st		@v0XY,			0(@fifo),					1,				SKN_GQR_UV
//	ps_merge11	@v0XY,			@v0XY,						@v0XY
//	psq_st		@v0XY,			0(@fifo),					1,				SKN_GQR_UV
//	psq_st		@v0ZW,			0(@fifo),					1,				SKN_GQR_UV
//	psq_st		@fzero,			0(@fifo),					1,				SKN_GQR_UV
//	psq_st		@fone,			0(@fifo),					1,				SKN_GQR_UV

//	t -= p_stream->m_interval;
@skip:
	ps_sub		@t,				@t,							@interval



//	cmpw		@use_mid_color,	@zero
//	beq			@no_mid
//	
//	b			@over_mid
//@no_mid:
//	
//@over_mid:





//	fres		@in, @in
	bdnz+		@particle_loop

	lfd		%f14, ( 2*4)(%r1)
	lfd		%f15, ( 4*4)(%r1)
	lfd		%f16, ( 6*4)(%r1)
	lfd		%f17, ( 8*4)(%r1)
	lfd		%f18, (10*4)(%r1)
	lfd		%f19, (12*4)(%r1)
	lfd		%f20, (14*4)(%r1)
	lfd		%f21, (16*4)(%r1)
	lfd		%f22, (18*4)(%r1)
	lfd		%f23, (20*4)(%r1)
	lfd		%f24, (22*4)(%r1)
	lfd		%f25, (24*4)(%r1)
	lfd		%f26, (26*4)(%r1)
	lfd		%f27, (28*4)(%r1)
	lfd		%f28, (30*4)(%r1)
	lfd		%f29, (32*4)(%r1)
	lfd		%f30, (34*4)(%r1)
	lfd		%f31, (36*4)(%r1)
	lwz		%r14, (38*4)(%r1)
	lwz		%r15, (39*4)(%r1)
	lwz		%r16, (40*4)(%r1)
	lwz		%r17, (41*4)(%r1)
	lwz		%r18, (42*4)(%r1)
	lwz		%r19, (43*4)(%r1)
	lwz		%r20, (44*4)(%r1)
	addi    %r1, %r1, 256
	blr

@tenth_data:
	.float	(0.01)
	.float	(0.01)
	.float	(0.01)
	.float	(0.01)

@one_data:
	.float	(1.0)
	.float	(1.0)
	.float	(1.0)
	.float	(1.0)

@rand_data:
	.float	(1.0)		// garbage
	.float	(1.0)
	.float	(1.0)
	.float	(1.0)

@six_data:
	.float	(6.0)
	.float	(6.0)
	.float	(6.0)
	.float	(6.0)

@temp_data:
	.float	(0.0)
	.float	(0.0)
	.float	(0.0)
	.float	(0.0)

	.endscope

#---------------------------------------------------------------------------
# Reciprocal estimate.
#---------------------------------------------------------------------------

	.scope
	.global	ReciprocalEstimate_ASM
ReciprocalEstimate_ASM:
	# Input parameters.
	.equr		@in,			%f1				# float f

	fres		@in, @in
	blr
	.endscope

#---------------------------------------------------------------------------
# Convert an array of fixed point shorts (1.9.6) to floating point.
#---------------------------------------------------------------------------

	.scope
	.global	ConvertFixed196ToFloat
ConvertFixed196ToFloat:
	# Input parameters.
	.equr		@in,			%r3				# s16 * p_source
	.equr		@out,			%r4				# float * p_dest
	.equr		@count,			%r5				# int count

	.equr		@X_Y,			%f0
	.equr		@Z_n,			%f1

	addi    	@in,			@in,			-8
	addi    	@out,			@out,			-4
	
	mtctr   	@count

@mloop:
	psq_lu  	@X_Y, 			8(@in), 		0, 			SKN_GQR_POS
	psq_lu  	@Z_n, 			4(@in), 		1, 			SKN_GQR_POS
				   
	psq_stu 	@X_Y, 			4(@out), 		0, 			SKN_GQR_UV
	psq_stu 	@Z_n, 			8(@out), 		1, 			SKN_GQR_UV

	bdnz+ @mloop
	blr
	.endscope

#---------------------------------------------------------------------------
# Calculate the dot product for every face.
#---------------------------------------------------------------------------

	.scope
	.global	CalculateDotProducts
CalculateDotProducts:
	.equr		@vert,			%r3				# s16 * p_xyz
	.equr		@idx,			%r4				# u16 * p_index_list
	.equr		@dot,			%r5				# float * p_dot
	.equr		@count,			%r6				# int count
	.equr		@px,			%f1				# float px
	.equr		@py,			%f2				# float py
	.equr		@pz,			%f3				# float pz

	.equr		@idx0,			%r7
	.equr		@idx1,			%r8
	.equr		@idx2,			%r9

	.equr		@address,		%r10

	.equr		@tmp0,			%r11
	.equr		@tmp1,			%r12

	.equr		@X0_Y0,			%f0
	.equr		@Z0_nn,			%f4
	.equr		@X1_Y1,			%f5
	.equr		@Z1_nn,			%f6
	.equr		@X2_Y2,			%f7
	.equr		@Z2_nn,			%f8

	.equr		@XY10,			%f9
	.equr		@Zn10,			%f10
	.equr		@XY20,			%f11
	.equr		@Zn20,			%f12

	.equr		@P_1X2Z_1Y2Z,	%f0		// Can re-use 1st set of floats now.
	.equr		@P_2X1Z_2Y1Z,	%f4
	.equr		@P_nnnn_1Y2X,	%f5
	.equr		@P_1X2Y_nnnn,	%f6

	.equr		@NX_nn,			%f7
	.equr		@NY_nn,			%f8
	.equr		@NZ_nn,			%f9

	addi		@idx,			@idx,			-2
	addi		@dot,			@dot,			-4

	mtctr   	@count
@mloop:
	// Read & Multiply indices by 12.
	lhzu		@idx0,			2(@idx)
	slwi		@tmp0,			@idx0,			2
	slwi		@tmp1,			@tmp0,			1
	add			@idx0,			@tmp0,			@tmp1
	add			@idx0,			@vert,			@idx0

	lhzu		@idx1,			2(@idx)
	slwi		@tmp0,			@idx1,			2
	slwi		@tmp1,			@tmp0,			1
	add			@idx1,			@tmp0,			@tmp1
	add			@idx1,			@vert,			@idx1

	lhzu		@idx2,			2(@idx)
	slwi		@tmp0,			@idx2,			2
	slwi		@tmp1,			@tmp0,			1
	add			@idx2,			@tmp0,			@tmp1
	add			@idx2,			@vert,			@idx2

	// Read vertices for each corner (sticking with shorts as less data will be read).
	psq_l		@X0_Y0,			0(@idx0),		0,			SKN_GQR_POS
	psq_l		@Z0_nn,			4(@idx0),		1,			SKN_GQR_POS
	psq_l		@X1_Y1,			0(@idx1),		0,			SKN_GQR_POS
	psq_l		@Z1_nn,			4(@idx1),		1,			SKN_GQR_POS
	psq_l		@X2_Y2,			0(@idx2),		0,			SKN_GQR_POS
	psq_l		@Z2_nn,			4(@idx2),		1,			SKN_GQR_POS
	
	// Do subtractions for cross product.
	ps_sub		@XY10,			@X1_Y1,			@X0_Y0
	ps_sub		@Zn10,			@Z1_nn,			@Z0_nn
	ps_sub		@XY20,			@X2_Y2,			@X0_Y0
	ps_sub		@Zn20,			@Z2_nn,			@Z0_nn
	
	// Do multiplications for cross product.
	ps_muls0	@P_1X2Z_1Y2Z,	@XY10,			@Zn20
	ps_muls0	@P_2X1Z_2Y1Z,	@XY20,			@Zn10
	ps_muls0	@P_nnnn_1Y2X,	@XY10,			@XY20
	ps_muls1	@P_1X2Y_nnnn,	@XY10,			@XY20
	ps_merge11	@P_nnnn_1Y2X,	@P_nnnn_1Y2X,	@P_nnnn_1Y2X

	// Finish cross product to give surface normal.
	ps_sub		@NX_nn,			@P_1X2Z_1Y2Z,	@P_2X1Z_2Y1Z		// X=1
	ps_sub		@NY_nn,			@P_2X1Z_2Y1Z,	@P_1X2Z_1Y2Z		// Y=0
	ps_sub		@NZ_nn,			@P_1X2Y_nnnn,	@P_nnnn_1Y2X
	ps_merge11	@NX_nn,			@NX_nn,			@NX_nn

	// Do dot product of projection direction and normal.
	ps_mul		@NX_nn,			@px,			@NX_nn
	ps_madd		@NY_nn,			@py,			@NY_nn,		@NX_nn
	ps_madd		@NZ_nn,			@pz,			@NZ_nn,		@NY_nn

	// Write out dot product.
	psq_stu 	@NZ_nn, 			4(@dot), 		1, 			SKN_GQR_UV

	bdnz+ @mloop
	blr
	.endscope

#---------------------------------------------------------------------------
# Render shadow volumes.
#---------------------------------------------------------------------------

	.scope
	.global	RenderShadows
RenderShadows:
	.equr		@vert,			%r3				# s16 * p_xyz
	.equr		@idx,			%r4				# u16 * p_index_list
	.equr		@edge,			%r5				# NxNgc::sShadowEdge * p_neighbor_list 
	.equr		@dot,			%r6				# float * p_dot
	.equr		@count,			%r7				# int count
	.equr		@px,			%f1				# float px
	.equr		@py,			%f2				# float py
	.equr		@pz,			%f3				# float pz
	.equr		@tx,			%f4				# float tx
	.equr		@ty,			%f5				# float ty
   	.equr		@tz,			%f6				# float tz

	.equr		@PX_PY,			%f1				# frees up f2.
	.equr		@PZ_nn,			%f3
	.equr		@TX_TY,			%f4				# frees up f5.
	.equr		@TZ_nn,			%f6

	.equr		@idx0,			%r12
	.equr		@idx1,			%r8
	.equr		@idx2,			%r9

	.equr		@address,		%r10

	.equr		@tmp0,			%r11
//	.equr		@tmp1,			%r12

	.equr		@FIFO,			%r14
	.equr		@BP,			%r15
//	.equr		@XF,			%r16
	.equr		@BLEND_ADD,		%r17
	.equr		@BLEND_SUB,		%r18
//	.equr		@CULL_FRONT,	%r19
//	.equr		@CULL_BACK,		%r20
	.equr		@RZERO,			%r21
//	.equr		@XF_NUMTEX_ID,	%r22
//	.equr		@XF_NUMCOL_ID,	%r23
//	.equr		@RONE,			%r24
	.equr		@TRISTRIP,		%r25
	.equr		@NUMVERTS10,	%r26
	.equr		@NUMVERTS8,		%r22
	.equr		@NUMVERTS6,		%r23
	.equr		@NUMVERTS3,		%r24

	.equr		@shape,			%r19
	.equr		@dot_base,		%r20

	.equr		@X0_Y0,			%f0
	.equr		@Z0_nn,			%f2
	.equr		@X1_Y1,			%f17
	.equr		@Z1_nn,			%f5
	.equr		@X2_Y2,			%f18
	.equr		@Z2_nn,			%f7

	.equr		@X3_Y3,			%f8
	.equr		@Z3_nn,			%f9
	.equr		@X4_Y4,			%f10
	.equr		@Z4_nn,			%f11
	.equr		@X5_Y5,			%f12
	.equr		@Z5_nn,			%f13

	.equr		@DOT,			%f14
	.equr		@ZERO,			%f15
	.equr		@FTEMP,			%f16

	stwu    %r1, -160(%r1)
	stfd    %f14, 8(%r1)
	stfd    %f15, 16(%r1)
	stfd    %f16, 24(%r1)
	stfd    %f17, 32(%r1)
	stfd    %f18, 40(%r1)
	stw		%r14, 48(%r1)
	stw		%r15, 52(%r1)
	stw		%r16, 56(%r1)
	stw		%r17, 60(%r1)
	stw		%r18, 64(%r1)
	stw		%r19, 68(%r1)
	stw		%r20, 72(%r1)
	stw		%r21, 76(%r1)
	stw		%r22, 76(%r1)
	stw		%r23, 80(%r1)
	stw		%r24, 84(%r1)
	stw		%r25, 88(%r1)
	stw		%r25, 92(%r1)
	stw		%r26, 96(%r1)
	stw		%r27, 100(%r1)

	// 33222222222211111111110000000000
	// 10987654321098765432109876543210
	//
	// ADD: 0x0131
	//
	// 00000000000000000000000100110001
	//
	// SUB: 0x0931
	//
	// 00000000000000000000100100110001

	// 33222222222211111111110000000000
	// 10987654321098765432109876543210
	//
	// FRONT: 0x8010
	//
	// 00000000000000001000000000010000
	//
	// BACK: 0x4010
	//
	// 00000000000000000100000000010000

	li			@RZERO,			0x0
	lis			@FIFO,			0xcc01
	addi		@FIFO,			@FIFO,			-0x8000
	li			@BP,			0x61
//	li			@XF,			0x10
	lis			@BLEND_SUB,		0x4100
	addi		@BLEND_ADD,		@BLEND_SUB,		0x0131 
	addi		@BLEND_SUB,		@BLEND_SUB,		0x0931
//	ori			@CULL_FRONT,	@RZERO,			0x8010
//	li			@CULL_BACK,		0x4010
//	li			@XF_NUMTEX_ID,	0x103f
//	li			@XF_NUMCOL_ID,	0x1009
	li			@TRISTRIP,		0x98
	li			@NUMVERTS10,	10
	li			@NUMVERTS8,		8
	li			@NUMVERTS6,		6
	li			@NUMVERTS3,		3

	ps_merge00	@PX_PY,			@px,			@py
	ps_merge00	@TX_TY,			@tx,			@ty

	add			@dot_base,		@dot,			@RZERO

	addi		@edge,			@edge,			-2
	addi		@idx,			@idx,			-2
	addi		@dot,			@dot,			-4

	ps_sub		@ZERO,			@ZERO,			@ZERO	// Create 0.0f.

//	// Set cull mode.
//	li			@tmp0,			0x10
//	stb			@tmp0,			(@FIFO)
//	sth			@RZERO,			(@FIFO)
//	li			@tmp0,			0x103f
//	sth			@tmp0,			(@FIFO)
//	stw			@RZERO,			(@FIFO)		// Num Tex Gens
//
//	li			@tmp0,			0x10
//	stb			@tmp0,			(@FIFO)
//	sth			@RZERO,			(@FIFO)
//	li			@tmp0,			0x1009
//	sth			@tmp0,			(@FIFO)
//	li			@tmp0,			0x1
//	stw			@tmp0,			(@FIFO)		// Num Color Channels.
//
//	stb			@BP,			(@FIFO)
//	ori			@tmp0,			@RZERO,			0x8010		// 8010=FRONT, 4010=BACK
//	stw			@tmp0,	(@FIFO)

	// Write out some bullshit data to fix the XF Stall bug.
	stb			@TRISTRIP,		(@FIFO)
	sth			@NUMVERTS10,	(@FIFO)

	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@ZERO,			0(@FIFO),       1,          SKN_GQR_UV



	mtctr   	@count

@mloopadd:
	// Read dot product & see if face is visible.
	psq_lu		@DOT,			4(@dot),		1,			SKN_GQR_UV

	ps_cmpu0	%cr0,			@DOT,			@ZERO
	ble			@skipadd

	// Read neighbor values.
	lhau		@idx0,			2(@edge)
	lhau		@idx1,			2(@edge)
	lhau		@idx2,			2(@edge)

	add			@shape,			@RZERO,			@RZERO

	// Check side 01.
	cmpw		@idx0,			@RZERO
	blt			@set01
	slwi		@idx0,			@idx0,			2
	add			@idx0,			@idx0,			@dot_base
	psq_l		@DOT,			0(@idx0),		1,			SKN_GQR_UV
	ps_cmpu0	%cr0,			@DOT,			@ZERO
	bgt			@skip01
@set01:
	ori			@shape,			@shape,			1

@skip01:

	// Check side 12.
	cmpw		@idx1,			@RZERO
	blt			@set12
	slwi		@idx1,			@idx1,			2
	add			@idx1,			@idx1,			@dot_base
	psq_l		@DOT,			0(@idx1),		1,			SKN_GQR_UV
	ps_cmpu0	%cr0,			@DOT,			@ZERO
	bgt			@skip12
@set12:
	ori			@shape,			@shape,			2

@skip12:

	// Check side 20.
	cmpw		@idx2,			@RZERO
	blt			@set20
	slwi		@idx2,			@idx2,			2
	add			@idx2,			@idx2,			@dot_base
	psq_l		@DOT,			0(@idx2),		1,			SKN_GQR_UV
	ps_cmpu0	%cr0,			@DOT,			@ZERO
	bgt			@skip20
@set20:
	ori			@shape,			@shape,			4

@skip20:

	// Read & Multiply indices by 12.
	lhzu		@idx0,			2(@idx)
	slwi		@tmp0,			@idx0,			2
	slwi		@idx0,			@tmp0,			1
	add			@idx0,			@tmp0,			@idx0
	add			@idx0,			@vert,			@idx0

	lhzu		@idx1,			2(@idx)
	slwi		@tmp0,			@idx1,			2
	slwi		@idx1,			@tmp0,			1
	add			@idx1,			@tmp0,			@idx1
	add			@idx1,			@vert,			@idx1

	lhzu		@idx2,			2(@idx)
	slwi		@tmp0,			@idx2,			2
	slwi		@idx2,			@tmp0,			1
	add			@idx2,			@tmp0,			@idx2
	add			@idx2,			@vert,			@idx2

	// Read vertices for each corner (sticking with shorts as less data will be read).
	psq_l		@X0_Y0,			0(@idx0),		0,			SKN_GQR_POS
	psq_l		@Z0_nn,			4(@idx0),		1,			SKN_GQR_POS
	psq_l		@X1_Y1,			0(@idx1),		0,			SKN_GQR_POS
	psq_l		@Z1_nn,			4(@idx1),		1,			SKN_GQR_POS
	psq_l		@X2_Y2,			0(@idx2),		0,			SKN_GQR_POS
	psq_l		@Z2_nn,			4(@idx2),		1,			SKN_GQR_POS

	// Fabricate points 3, 4 & 5.
	ps_add		@X3_Y3,			@X0_Y0,			@PX_PY
	ps_add		@Z3_nn,			@Z0_nn,			@PZ_nn
	ps_add		@X4_Y4,			@X1_Y1,			@PX_PY
	ps_add		@Z4_nn,			@Z1_nn,			@PZ_nn
	ps_add		@X5_Y5,			@X2_Y2,			@PX_PY
	ps_add		@Z5_nn,			@Z2_nn,			@PZ_nn

	// Tweak points 0, 1 & 2.
	ps_add		@X0_Y0,			@X0_Y0,			@TX_TY
	ps_add		@Z0_nn,			@Z0_nn,			@TZ_nn
	ps_add		@X1_Y1,			@X1_Y1,			@TX_TY
	ps_add		@Z1_nn,			@Z1_nn,			@TZ_nn
	ps_add		@X2_Y2,			@X2_Y2,			@TX_TY
	ps_add		@Z2_nn,			@Z2_nn,			@TZ_nn

	// Set blend mode.
	stb			@BP,			(@FIFO)
	stw			@BLEND_ADD,		(@FIFO)

	// Check for all shapes.
	cmpi		%cr0,			@shape,			0
	bne			@not0

	// Write mesh.
	stb			@TRISTRIP,		(@FIFO)
	sth			@NUMVERTS3,		(@FIFO)

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	stb			@TRISTRIP,		(@FIFO)
	sth			@NUMVERTS3,		(@FIFO)

	psq_st		@X3_Y3,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X3_Y3,			@X3_Y3
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z3_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X4_Y4,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X4_Y4,			@X4_Y4
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z4_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X5_Y5,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X5_Y5,			@X5_Y5
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z5_nn,			0(@FIFO),       1,          SKN_GQR_UV

	// Set blend mode.
	stb			@BP,			(@FIFO)
	stw			@BLEND_SUB,		(@FIFO)

	// Write mesh.
	stb			@TRISTRIP,		(@FIFO)
	sth			@NUMVERTS3,		(@FIFO)

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	stb			@TRISTRIP,		(@FIFO)
	sth			@NUMVERTS3,		(@FIFO)

	psq_st		@X5_Y5,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X5_Y5,			@X5_Y5
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z5_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X4_Y4,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X4_Y4,			@X4_Y4
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z4_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X3_Y3,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X3_Y3,			@X3_Y3
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z3_nn,			0(@FIFO),       1,          SKN_GQR_UV

	b			@overall
@not0:

	cmpi		%cr0,           @shape,			1
	bne			@not1

	// Write mesh.
	stb			@TRISTRIP,		(@FIFO)
	sth			@NUMVERTS6,		(@FIFO)

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X4_Y4,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X4_Y4,			@X4_Y4
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z4_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X3_Y3,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X3_Y3,			@X3_Y3
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z3_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X5_Y5,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X5_Y5,			@X5_Y5
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z5_nn,			0(@FIFO),       1,          SKN_GQR_UV

	// Set blend mode.
	stb			@BP,			(@FIFO)
	stw			@BLEND_SUB,		(@FIFO)

	// Write mesh.
	stb			@TRISTRIP,		(@FIFO)
	sth			@NUMVERTS6,		(@FIFO)

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X3_Y3,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X3_Y3,			@X3_Y3
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z3_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X4_Y4,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X4_Y4,			@X4_Y4
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z4_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X5_Y5,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X5_Y5,			@X5_Y5
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z5_nn,			0(@FIFO),       1,          SKN_GQR_UV

	b			@overall
@not1:

	cmpi		%cr0,           @shape,			2
	bne			@not2

	// Write mesh.
	stb			@TRISTRIP,		(@FIFO)
	sth			@NUMVERTS6,		(@FIFO)

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X5_Y5,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X5_Y5,			@X5_Y5
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z5_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X4_Y4,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X4_Y4,			@X4_Y4
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z4_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X3_Y3,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X3_Y3,			@X3_Y3
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z3_nn,			0(@FIFO),       1,          SKN_GQR_UV

	// Set blend mode.
	stb			@BP,			(@FIFO)
	stw			@BLEND_SUB,		(@FIFO)

	// Write mesh.
	stb			@TRISTRIP,		(@FIFO)
	sth			@NUMVERTS6,		(@FIFO)

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X4_Y4,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X4_Y4,			@X4_Y4
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z4_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X5_Y5,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X5_Y5,			@X5_Y5
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z5_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X3_Y3,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X3_Y3,			@X3_Y3
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z3_nn,			0(@FIFO),       1,          SKN_GQR_UV

	b			@overall
@not2:

	cmpi		%cr0,           @shape,			3
	bne			@not3
	
	// Write mesh.
	stb			@TRISTRIP,		(@FIFO)
	sth			@NUMVERTS8,		(@FIFO)

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X5_Y5,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X5_Y5,			@X5_Y5
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z5_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X4_Y4,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X4_Y4,			@X4_Y4
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z4_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X3_Y3,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X3_Y3,			@X3_Y3
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z3_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	// Set blend mode.
	stb			@BP,			(@FIFO)
	stw			@BLEND_SUB,		(@FIFO)

	// Write mesh.
	stb			@TRISTRIP,		(@FIFO)
	sth			@NUMVERTS8,		(@FIFO)

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X5_Y5,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X5_Y5,			@X5_Y5
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z5_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X4_Y4,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X4_Y4,			@X4_Y4
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z4_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X3_Y3,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X3_Y3,			@X3_Y3
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z3_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	b			@overall
@not3:

	cmpi		%cr0,           @shape,			4
	bne			@not4

	// Write mesh.
	stb			@TRISTRIP,		(@FIFO)
	sth			@NUMVERTS6,		(@FIFO)

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X3_Y3,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X3_Y3,			@X3_Y3
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z3_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X5_Y5,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X5_Y5,			@X5_Y5
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z5_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X4_Y4,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X4_Y4,			@X4_Y4
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z4_nn,			0(@FIFO),       1,          SKN_GQR_UV

	// Set blend mode.
	stb			@BP,			(@FIFO)
	stw			@BLEND_SUB,		(@FIFO)

	// Write mesh.
	stb			@TRISTRIP,		(@FIFO)
	sth			@NUMVERTS6,		(@FIFO)

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X5_Y5,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X5_Y5,			@X5_Y5
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z5_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X3_Y3,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X3_Y3,			@X3_Y3
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z3_nn,			0(@FIFO),       1,          SKN_GQR_UV
		
	psq_st		@X4_Y4,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X4_Y4,			@X4_Y4
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z4_nn,			0(@FIFO),       1,          SKN_GQR_UV

	b			@overall
@not4:
		
	cmpi		%cr0,           @shape,			5
	bne			@not5

	// Write mesh.
	stb			@TRISTRIP,		(@FIFO)
	sth			@NUMVERTS8,		(@FIFO)

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X4_Y4,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X4_Y4,			@X4_Y4
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z4_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X3_Y3,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X3_Y3,			@X3_Y3
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z3_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X5_Y5,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X5_Y5,			@X5_Y5
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z5_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	// Set blend mode.
	stb			@BP,			(@FIFO)
	stw			@BLEND_SUB,		(@FIFO)

	// Write mesh.
	stb			@TRISTRIP,		(@FIFO)
	sth			@NUMVERTS8,		(@FIFO)

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X4_Y4,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X4_Y4,			@X4_Y4
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z4_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X3_Y3,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X3_Y3,			@X3_Y3
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z3_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X5_Y5,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X5_Y5,			@X5_Y5
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z5_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	b			@overall
@not5:
	
	cmpi		%cr0,           @shape,			6
	bne			@not6

	// Write mesh.
	stb			@TRISTRIP,		(@FIFO)
	sth			@NUMVERTS8,		(@FIFO)

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X3_Y3,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X3_Y3,			@X3_Y3
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z3_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X5_Y5,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X5_Y5,			@X5_Y5
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z5_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X4_Y4,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X4_Y4,			@X4_Y4
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z4_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	// Set blend mode.
	stb			@BP,			(@FIFO)
	stw			@BLEND_SUB,		(@FIFO)

	// Write mesh.
	stb			@TRISTRIP,		(@FIFO)
	sth			@NUMVERTS8,		(@FIFO)

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X3_Y3,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X3_Y3,			@X3_Y3
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z3_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X5_Y5,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X5_Y5,			@X5_Y5
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z5_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X4_Y4,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X4_Y4,			@X4_Y4
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z4_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	b			@overall
@not6:
		
	cmpi		%cr0,           @shape,			7
	bne			@not7

	// Write mesh.
	stb			@TRISTRIP,		(@FIFO)
	sth			@NUMVERTS10,	(@FIFO)

	psq_st		@X5_Y5,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X5_Y5,			@X5_Y5
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z5_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X4_Y4,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X4_Y4,			@X4_Y4
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z4_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X4_Y4,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X4_Y4,			@X4_Y4
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z4_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X3_Y3,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X3_Y3,			@X3_Y3
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z3_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X5_Y5,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X5_Y5,			@X5_Y5
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z5_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	// Set blend mode.
	stb			@BP,			(@FIFO)
	stw			@BLEND_SUB,		(@FIFO)

	// Write mesh.
	stb			@TRISTRIP,		(@FIFO)
	sth			@NUMVERTS10,	(@FIFO)

	psq_st		@X2_Y2,			0(@FIFO),		1,			SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X5_Y5,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X5_Y5,			@X5_Y5
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z5_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X4_Y4,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X4_Y4,			@X4_Y4
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z4_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X3_Y3,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X3_Y3,			@X3_Y3
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z3_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X1_Y1,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X1_Y1,			@X1_Y1
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z1_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X0_Y0,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X0_Y0,			@X0_Y0
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z0_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X2_Y2,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X2_Y2,			@X2_Y2
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z2_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X3_Y3,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X3_Y3,			@X3_Y3
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z3_nn,			0(@FIFO),       1,          SKN_GQR_UV

	psq_st		@X5_Y5,			0(@FIFO),       1,          SKN_GQR_UV
	ps_merge11	@FTEMP,			@X5_Y5,			@X5_Y5
	psq_st		@FTEMP,			0(@FIFO),       1,          SKN_GQR_UV
	psq_st		@Z5_nn,			0(@FIFO),       1,          SKN_GQR_UV

@not7:
@overall:

	bdnz+		@mloopadd
	b			@overskip

@skipadd:
	addi		@idx,			@idx,			6
	addi		@edge,			@edge,			6
	bdnz+		@mloopadd
@overskip:
		
	lfd    %f14, 8(%r1)
	lfd    %f15, 16(%r1)
	lfd    %f16, 24(%r1)
	lfd    %f17, 32(%r1)
	lfd    %f18, 40(%r1)
	lwz	   %r14, 48(%r1)
	lwz	   %r15, 52(%r1)
	lwz	   %r16, 56(%r1)
	lwz	   %r17, 60(%r1)
	lwz	   %r18, 64(%r1)
	lwz	   %r19, 68(%r1)
	lwz	   %r20, 72(%r1)
	lwz	   %r21, 76(%r1)
	lwz	   %r22, 76(%r1)
	lwz	   %r23, 80(%r1)
	lwz	   %r24, 84(%r1)
	lwz	   %r25, 88(%r1)
	lwz	   %r25, 92(%r1)
	lwz	   %r26, 96(%r1)
	lwz	   %r27, 100(%r1)
	addi    %r1, %r1, 160
	blr
	.endscope

//#---------------------------------------------------------------------------
//#  Name:         SKN1Vecs16Norms16
//#
//#  Description:  Transforms array of vertex-normal pairs.
//#
//#                Differs from a standard matrix-vector multiply in that the
//#                normals are not translated.  Before you ask, no, there is 
//#                no cycle gain from applying a 3x3 matrix vs. applying a 4x3
//#                matrix.  Also, because we are always doing vertex-normal
//#                pairs, no need to check for odd # of transforms, etc.
//#
//#  Arguments:    m           column-major matrix to apply
//#                srcBase     ptr to source vert/norm pairs
//#                dstBase     ptr to destination, can be same as source
//#                count       number of vert/norm pairs.
//#
//#  Returns:      None.
//#---------------------------------------------------------------------------
//
//	.scope
//	.global	FakeEnvMap
//FakeEnvMap:
//	# Input parameters.
//	.equr		@m,				%r3	#    ROMtx   m
//	.equr		@srcPos,		%r4	#    s16  *srcPos
//	.equr		@srcNorm,		%r5	#    s16  *srcNorm
//	.equr		@dstBase,		%r6	#    s16  *dstBase
//	.equr		@count,			%r7	#    u32     count
//
//	.equr		@M00_M10,		%f0
//	.equr		@M20_nnn,		%f1
//	.equr		@M01_M11,		%f2
//	.equr		@M21_nnn,		%f3
//	.equr		@M02_M12,		%f4
//	.equr		@M22_nnn,		%f5
//	.equr		@M03_M13,		%f6
//	.equr		@M23_nnn,		%f7
//
//# source vectors - 2 3D vectors in 3 PS registers
//	.equr		@SX0_SY0,		%f8
//	.equr		@SZ0_nnn,		%f9
//	.equr		@nnn_SX1,		%f10
//	.equr		@SY1_SZ1,		%f11
//
//# Destination registers - 2 3d vectors in 4 PS registers
//	.equr		@DX0_DY0,		%f12
//	.equr		@DZ0_nnn,		%f13
//	.equr		@DX1_DY1,		%f14
//	.equr		@DZ1_nnn,		%f15
//
//# temp registers for writing back values.  These registers store the final
//# results from the PREVIOUS loop
//	.equr		@WX0_WY0,		%f16
//	.equr		@WZ0_nnn,		%f17
//	.equr		@WX1_WY1,		%f18
//	.equr		@WZ1_nnn,		%f19
//
//	.equr		@HALF,			%f20
//	.equr		@ONE,			%f21
//	.equr		@ONEZ,			%f22
//
//    stwu    %r1, -96(%r1)
//    stfd    %f14, 8(%r1)
//    stfd    %f15, 16(%r1)
//    addi    @count, @count, -1 // unrolled
//    stfd    %f16, 24(%r1)
//    stfd    %f17, 32(%r1)
//    stfd    %f18, 40(%r1)
//    stfd    %f19, 48(%r1)
//    stfd    %f20, 56(%r1)
//    stfd    %f21, 64(%r1)
//    stfd    %f22, 72(%r1)
//    mtctr   @count
//    
//	lis     %r9, @half_data@h
//	ori     %r9, %r9, @half_data@l
//	psq_l   @HALF, 0(%r9),0,SKN_GQR_UV
//
//	lis     %r9, @one_data@h
//	ori     %r9, %r9, @one_data@l
//	psq_l   @ONE, 0(%r9),0,SKN_GQR_UV
//
//	// load matrix
//    psq_l   @M00_M10, 0(@m),0,0  
//	addi    @srcPos, @srcPos, -2
//    addi    @srcNorm, @srcNorm, -4
//    psq_l   @M20_nnn, 8(@m),1,0  
//    addi    @dstBase, @dstBase, -8
//    psq_l   @M03_M13, 36(@m),0,0 
//    psq_lu  @SX0_SY0, 2(@srcPos), 0, SKN_GQR_POS
//    psq_l   @M23_nnn, 44(@m),1,0 
//    psq_lu  @SZ0_nnn, 4(@srcPos), 0, SKN_GQR_POS
//	psq_lu  @nnn_SX1, 2(@srcNorm), 0, SKN_GQR_NORM
//
//    // ------------------------------UNROLLED
//
//    // Apply first column and translation term
//    ps_madds0    @DX0_DY0, @M00_M10, @SX0_SY0, @M03_M13
//      psq_l   @M01_M11, 12(@m),0,0
//    ps_madds0    @DZ0_nnn, @M20_nnn, @SX0_SY0, @M23_nnn
//      psq_l   @M21_nnn, 20(@m),1,0   
//    ps_muls1     @DX1_DY1, @M00_M10, @nnn_SX1  // no trans for norms
//      psq_lu  @SY1_SZ1,4(@srcNorm), 0, SKN_GQR_NORM
//    ps_muls1     @DZ1_nnn, @M20_nnn, @nnn_SX1  // no trans for norms
//      psq_l   @M22_nnn, 32(@m),1,0 
//
//    // Apply second column
//     ps_madds1    @DX0_DY0, @M01_M11, @SX0_SY0, @DX0_DY0
//    ps_madds1    @DZ0_nnn, @M21_nnn, @SX0_SY0, @DZ0_nnn
//      psq_l   @M02_M12, 24(@m),0,0 
//    ps_madds0    @DX1_DY1, @M01_M11, @SY1_SZ1, @DX1_DY1
//      psq_lu @SX0_SY0, 2(@srcPos), 0, SKN_GQR_POS
//    ps_madds0    @DZ1_nnn, @M21_nnn, @SY1_SZ1, @DZ1_nnn
//
//    // Apply third column and Write final values to temp W registers
//    ps_madds0    @WX0_WY0, @M02_M12, @SZ0_nnn, @DX0_DY0
//    ps_madds0    @WZ0_nnn, @M22_nnn, @SZ0_nnn, @DZ0_nnn
//      psq_lu @SZ0_nnn, 4(@srcPos), 0, SKN_GQR_POS
//    ps_madds1    @WX1_WY1, @M02_M12, @SY1_SZ1, @DX1_DY1
//	  psq_lu @nnn_SX1,2(@srcNorm), 0, SKN_GQR_NORM
//    ps_madds1    @WZ1_nnn, @M22_nnn, @SY1_SZ1, @DZ1_nnn
//      psq_lu @SY1_SZ1,4(@srcNorm), 0, SKN_GQR_NORM
//
//    // -------------------------- LOOP START
//@mloop:
//    ps_madds0    @DX0_DY0, @M00_M10, @SX0_SY0, @M03_M13
//
//;//	ps_neg		 @WX1_WY1, @WX1_WY1
//		ps_div		@ONEZ, @ONE, @WZ0_nnn				// Calculate 1/z to 0
//    ps_madds0    @DZ0_nnn, @M20_nnn, @SX0_SY0, @M23_nnn
//		ps_madds0   @WX1_WY1, @WX0_WY0, @ONEZ, @WX1_WY1		// x,y * 1/z + nx,ny.
//    ps_muls1     @DX1_DY1, @M00_M10, @nnn_SX1
//		ps_madd		 @WX1_WY1, @WX1_WY1, @HALF, @HALF		// (x.y*.5)+.5 instead of (x.y+1)*.5
//    ps_muls1     @DZ1_nnn, @M20_nnn, @nnn_SX1
//		psq_stu     @WX1_WY1, 8(@dstBase), 0, SKN_GQR_UV
//    ps_madds1    @DX0_DY0, @M01_M11, @SX0_SY0, @DX0_DY0
//    ps_madds1    @DZ0_nnn, @M21_nnn, @SX0_SY0, @DZ0_nnn
//      psq_lu @SX0_SY0, 2(@srcPos), 0, SKN_GQR_POS // NEXT SX0 SY0
//    ps_madds0    @DX1_DY1, @M01_M11, @SY1_SZ1, @DX1_DY1
//    ps_madds0    @DZ1_nnn, @M21_nnn, @SY1_SZ1, @DZ1_nnn
//
//    // Write final values to temp registers
//    ps_madds0    @WX0_WY0, @M02_M12, @SZ0_nnn, @DX0_DY0
//    ps_madds0    @WZ0_nnn, @M22_nnn, @SZ0_nnn, @DZ0_nnn
//      psq_lu @SZ0_nnn, 4(@srcPos), 0, SKN_GQR_POS // NEXT SZ0 SX1
//    ps_madds1    @WX1_WY1, @M02_M12, @SY1_SZ1, @DX1_DY1
//		psq_lu @nnn_SX1,2(@srcNorm), 0, SKN_GQR_NORM // NEXT SY1 SZ1
//    ps_madds1    @WZ1_nnn, @M22_nnn, @SY1_SZ1, @DZ1_nnn
//      psq_lu @SY1_SZ1,4(@srcNorm), 0, SKN_GQR_NORM // NEXT SY1 SZ1
//
//    bdnz+ @mloop    // -------------------------- LOOP END
//
//;//	ps_neg		 @WX1_WY1, @WX1_WY1
//	ps_div		@ONEZ, @ONE, @WZ0_nnn				// Calculate 1/z to 0
//	ps_madds0   @WX1_WY1, @WX0_WY0, @ONEZ, @WX1_WY1		// x,y * 1/z + nx,ny.
//	ps_madd	 @WX1_WY1, @WX1_WY1, @HALF, @HALF		// (x,y*.5)+.5 instead of (x,y+1)*.5
//
//    psq_stu     @WX1_WY1, 8(@dstBase), 0, SKN_GQR_UV
//
//
//@return:    
//    lfd     %f14, 8(%r1)
//    lfd     %f15, 16(%r1)
//    lfd     %f16, 24(%r1)
//    lfd     %f17, 32(%r1)
//    lfd     %f18, 40(%r1)
//    lfd     %f19, 48(%r1)
//    lfd     %f20, 56(%r1)
//    lfd     %f21, 64(%r1)
//    lfd     %f22, 72(%r1)
//    addi    %r1, %r1, 96
//    blr
//
//@half_data:
//	.float	(0.5)
//	.float	(0.5)
//	.float	(0.5)
//	.float	(0.5)
//
//@one_data:
//	.float	(1.0)
//	.float	(1.0)
//	.float	(1.0)
//	.float	(1.0)
//
//	.endscope
//
//;//---------------------------------------------------------------------
//;//---------------------------------------------------------------------
//;//---------------------------------------------------------------------
//
//	.scope
//	.global	FakeEnvMapFloat
//FakeEnvMapFloat:
//	# Input parameters.
//	.equr		@m,				%r3	#    ROMtx   m
//	.equr		@srcPos,		%r4	#    float  *srcPos
//	.equr		@srcNorm,		%r5	#    s16  *srcNorm
//	.equr		@dstBase,		%r6	#    s16  *dstBase
//	.equr		@count,			%r7	#    u32     count
//
//	.equr		@M00_M10,		%f0
//	.equr		@M20_nnn,		%f1
//	.equr		@M01_M11,		%f2
//	.equr		@M21_nnn,		%f3
//	.equr		@M02_M12,		%f4
//	.equr		@M22_nnn,		%f5
//	.equr		@M03_M13,		%f6
//	.equr		@M23_nnn,		%f7
//
//# source vectors - 2 3D vectors in 3 PS registers
//	.equr		@SX0_SY0,		%f8
//	.equr		@SZ0_nnn,		%f9
//	.equr		@nnn_SX1,		%f10
//	.equr		@SY1_SZ1,		%f11
//
//# Destination registers - 2 3d vectors in 4 PS registers
//	.equr		@DX0_DY0,		%f12
//	.equr		@DZ0_nnn,		%f13
//	.equr		@DX1_DY1,		%f14
//	.equr		@DZ1_nnn,		%f15
//
//# temp registers for writing back values.  These registers store the final
//# results from the PREVIOUS loop
//	.equr		@WX0_WY0,		%f16
//	.equr		@WZ0_nnn,		%f17
//	.equr		@WX1_WY1,		%f18
//	.equr		@WZ1_nnn,		%f19
//
//	.equr		@HALF,			%f20
//	.equr		@ONE,			%f21
//	.equr		@ONEZ,			%f22
//
//	stwu    %r1, -96(%r1)
//	stfd    %f14, 8(%r1)
//	stfd    %f15, 16(%r1)
//	addi    @count, @count, -1 // unrolled
//	stfd    %f16, 24(%r1)
//	stfd    %f17, 32(%r1)
//	stfd    %f18, 40(%r1)
//	stfd    %f19, 48(%r1)
//	stfd    %f20, 56(%r1)
//	stfd    %f21, 64(%r1)
//	stfd    %f22, 72(%r1)
//	mtctr   @count
//
//	lis     %r9, @half_data@h
//	ori     %r9, %r9, @half_data@l
//	psq_l   @HALF, 0(%r9),0,SKN_GQR_UV
//
//	lis     %r9, @one_data@h
//	ori     %r9, %r9, @one_data@l
//	psq_l   @ONE, 0(%r9),0,SKN_GQR_UV
//
//	// load matrix
//	psq_l   @M00_M10, 0(@m),0,0  
//	addi    @srcPos, @srcPos, -4
//	addi    @srcNorm, @srcNorm, -4
//	psq_l   @M20_nnn, 8(@m),1,0  
//	addi    @dstBase, @dstBase, -8
//	psq_l   @M03_M13, 36(@m),0,0 
//	psq_lu  @SX0_SY0, 4(@srcPos), 0, SKN_GQR_UV
//	psq_l   @M23_nnn, 44(@m),1,0 
//	psq_lu  @SZ0_nnn, 8(@srcPos), 0, SKN_GQR_UV
//	psq_lu  @nnn_SX1, 2(@srcNorm), 0, SKN_GQR_NORM
//
//	// ------------------------------UNROLLED
//
//	// Apply first column and translation term
//	ps_madds0    @DX0_DY0, @M00_M10, @SX0_SY0, @M03_M13
//	  psq_l   @M01_M11, 12(@m),0,0
//	ps_madds0    @DZ0_nnn, @M20_nnn, @SX0_SY0, @M23_nnn
//	  psq_l   @M21_nnn, 20(@m),1,0   
//	ps_muls1     @DX1_DY1, @M00_M10, @nnn_SX1  // no trans for norms
//	  psq_lu  @SY1_SZ1,4(@srcNorm), 0, SKN_GQR_NORM
//	ps_muls1     @DZ1_nnn, @M20_nnn, @nnn_SX1  // no trans for norms
//	  psq_l   @M22_nnn, 32(@m),1,0 
//
//	// Apply second column
//	 ps_madds1    @DX0_DY0, @M01_M11, @SX0_SY0, @DX0_DY0
//	ps_madds1    @DZ0_nnn, @M21_nnn, @SX0_SY0, @DZ0_nnn
//	  psq_l   @M02_M12, 24(@m),0,0 
//	ps_madds0    @DX1_DY1, @M01_M11, @SY1_SZ1, @DX1_DY1
//	  psq_lu @SX0_SY0, 4(@srcPos), 0, SKN_GQR_UV
//	ps_madds0    @DZ1_nnn, @M21_nnn, @SY1_SZ1, @DZ1_nnn
//
//	// Apply third column and Write final values to temp W registers
//	ps_madds0    @WX0_WY0, @M02_M12, @SZ0_nnn, @DX0_DY0
//	ps_madds0    @WZ0_nnn, @M22_nnn, @SZ0_nnn, @DZ0_nnn
//	  psq_lu @SZ0_nnn, 8(@srcPos), 0, SKN_GQR_UV
//	ps_madds1    @WX1_WY1, @M02_M12, @SY1_SZ1, @DX1_DY1
//	  psq_lu @nnn_SX1,2(@srcNorm), 0, SKN_GQR_NORM
//	ps_madds1    @WZ1_nnn, @M22_nnn, @SY1_SZ1, @DZ1_nnn
//	  psq_lu @SY1_SZ1,4(@srcNorm), 0, SKN_GQR_NORM
//
//	// -------------------------- LOOP START
//@mloop:
//	ps_madds0    @DX0_DY0, @M00_M10, @SX0_SY0, @M03_M13
//
//;//	ps_neg		 @WX1_WY1, @WX1_WY1
//		ps_div		@ONEZ, @ONE, @WZ0_nnn				// Calculate 1/z to 0
//	ps_madds0    @DZ0_nnn, @M20_nnn, @SX0_SY0, @M23_nnn
//		ps_madds0   @WX1_WY1, @WX0_WY0, @ONEZ, @WX1_WY1		// x,y * 1/z + nx,ny.
//	ps_muls1     @DX1_DY1, @M00_M10, @nnn_SX1
//		ps_madd		 @WX1_WY1, @WX1_WY1, @HALF, @HALF		// (x.y*.5)+.5 instead of (x.y+1)*.5
//	ps_muls1     @DZ1_nnn, @M20_nnn, @nnn_SX1
//		psq_stu     @WX1_WY1, 8(@dstBase), 0, SKN_GQR_UV
//	ps_madds1    @DX0_DY0, @M01_M11, @SX0_SY0, @DX0_DY0
//	ps_madds1    @DZ0_nnn, @M21_nnn, @SX0_SY0, @DZ0_nnn
//	  psq_lu @SX0_SY0, 4(@srcPos), 0, SKN_GQR_UV // NEXT SX0 SY0
//	ps_madds0    @DX1_DY1, @M01_M11, @SY1_SZ1, @DX1_DY1
//	ps_madds0    @DZ1_nnn, @M21_nnn, @SY1_SZ1, @DZ1_nnn
//
//	// Write final values to temp registers
//	ps_madds0    @WX0_WY0, @M02_M12, @SZ0_nnn, @DX0_DY0
//	ps_madds0    @WZ0_nnn, @M22_nnn, @SZ0_nnn, @DZ0_nnn
//	  psq_lu @SZ0_nnn, 8(@srcPos), 0, SKN_GQR_UV // NEXT SZ0 SX1
//	ps_madds1    @WX1_WY1, @M02_M12, @SY1_SZ1, @DX1_DY1
//		psq_lu @nnn_SX1,2(@srcNorm), 0, SKN_GQR_NORM // NEXT SY1 SZ1
//	ps_madds1    @WZ1_nnn, @M22_nnn, @SY1_SZ1, @DZ1_nnn
//	  psq_lu @SY1_SZ1,4(@srcNorm), 0, SKN_GQR_NORM // NEXT SY1 SZ1
//
//	bdnz+ @mloop    // -------------------------- LOOP END
//
//;//	ps_neg		 @WX1_WY1, @WX1_WY1
//	ps_div		@ONEZ, @ONE, @WZ0_nnn				// Calculate 1/z to 0
//	ps_madds0   @WX1_WY1, @WX0_WY0, @ONEZ, @WX1_WY1		// x,y * 1/z + nx,ny.
//	ps_madd	 @WX1_WY1, @WX1_WY1, @HALF, @HALF		// (x,y*.5)+.5 instead of (x,y+1)*.5
//
//	psq_stu     @WX1_WY1, 8(@dstBase), 0, SKN_GQR_UV
//
//
//@return:    
//	lfd     %f14, 8(%r1)
//	lfd     %f15, 16(%r1)
//	lfd     %f16, 24(%r1)
//	lfd     %f17, 32(%r1)
//	lfd     %f18, 40(%r1)
//	lfd     %f19, 48(%r1)
//	lfd     %f20, 56(%r1)
//	lfd     %f21, 64(%r1)
//	lfd     %f22, 72(%r1)
//	addi    %r1, %r1, 96
//	blr
//
//@half_data:
//	.float	(0.5)
//	.float	(0.5)
//	.float	(0.5)
//	.float	(0.5)
//
//@one_data:
//	.float	(1.0)
//	.float	(1.0)
//	.float	(1.0)
//	.float	(1.0)
//
//	.endscope
//
//;//---------------------------------------------------------------------
//;//---------------------------------------------------------------------
//;//---------------------------------------------------------------------
//
//	.scope
//	.global	FakeEnvMapSkin
//FakeEnvMapSkin:
//	# Input parameters.
//	.equr		@m,				%r3	#    ROMtx   m
//	.equr		@srcPosNorm,	%r4	#    s16  *srcPosNorm
//	.equr		@dstBase,		%r5	#    s16  *dstBase
//	.equr		@count,			%r6	#    u32     count
//
//	.equr		@M00_M10,		%f0
//	.equr		@M20_nnn,		%f1
//	.equr		@M01_M11,		%f2
//	.equr		@M21_nnn,		%f3
//	.equr		@M02_M12,		%f4
//	.equr		@M22_nnn,		%f5
//	.equr		@M03_M13,		%f6
//	.equr		@M23_nnn,		%f7
//
//# source vectors - 2 3D vectors in 3 PS registers
//	.equr		@SX0_SY0,		%f8
//	.equr		@SZ0_SX1,		%f9
//	.equr		@SY1_SZ1,		%f10
//
//# Destination registers - 2 3d vectors in 4 PS registers
//	.equr		@DX0_DY0,		%f11
//	.equr		@DZ0_nnn,		%f12
//	.equr		@DX1_DY1,		%f13
//	.equr		@DZ1_nnn,		%f14
//
//# temp registers for writing back values.  These registers store the final
//# results from the PREVIOUS loop
//	.equr		@WX0_WY0,		%f15
//	.equr		@WZ0_nnn,		%f16
//	.equr		@WX1_WY1,		%f17
//	.equr		@WZ1_nnn,		%f18
//
//	.equr		@HALF,			%f19
//	.equr		@ONE,			%f20
//	.equr		@ONEZ,			%f21
//
//	stwu    %r1, -96(%r1)
//	stfd    %f14, 8(%r1)
//	stfd    %f15, 16(%r1)
//	addi    @count, @count, -1 // unrolled
//	stfd    %f16, 24(%r1)
//	stfd    %f17, 32(%r1)
//	stfd    %f18, 40(%r1)
//	stfd    %f19, 48(%r1)
//	stfd    %f20, 56(%r1)
//	stfd    %f21, 64(%r1)
//	mtctr   @count
//	
//	lis     %r9, @half_data@h
//	ori     %r9, %r9, @half_data@l
//	psq_l   @HALF, 0(%r9),0,SKN_GQR_UV
//
//	lis     %r9, @one_data@h
//	ori     %r9, %r9, @one_data@l
//	psq_l   @ONE, 0(%r9),0,SKN_GQR_UV
//
//	// load matrix
//	psq_l   @M00_M10, 0(@m),0,0  
//	addi    @srcPosNorm, @srcPosNorm, -4
//	psq_l   @M20_nnn, 8(@m),1,0  
//	addi    @dstBase, @dstBase, -8
//	psq_l   @M03_M13, 36(@m),0,0 
//	psq_lu  @SX0_SY0, 4(@srcPosNorm), 0, SKN_GQR_POS
//	psq_l   @M23_nnn, 44(@m),1,0 
//	psq_lu  @SZ0_SX1, 4(@srcPosNorm), 0, SKN_GQR_POS
//
//	// ------------------------------UNROLLED
//
//	// Apply first column and translation term
//	ps_madds0    @DX0_DY0, @M00_M10, @SX0_SY0, @M03_M13
//	  psq_l   @M01_M11, 12(@m),0,0
//	ps_madds0    @DZ0_nnn, @M20_nnn, @SX0_SY0, @M23_nnn
//	  psq_l   @M21_nnn, 20(@m),1,0   
//	ps_muls1     @DX1_DY1, @M00_M10, @SZ0_SX1  // no trans for norms
//	  psq_lu  @SY1_SZ1,4(@srcPosNorm), 0, SKN_GQR_POS
//	ps_muls1     @DZ1_nnn, @M20_nnn, @SZ0_SX1  // no trans for norms
//	  psq_l   @M22_nnn, 32(@m),1,0 
//
//	// Apply second column
//	 ps_madds1    @DX0_DY0, @M01_M11, @SX0_SY0, @DX0_DY0
//	ps_madds1    @DZ0_nnn, @M21_nnn, @SX0_SY0, @DZ0_nnn
//	  psq_l   @M02_M12, 24(@m),0,0 
//	ps_madds0    @DX1_DY1, @M01_M11, @SY1_SZ1, @DX1_DY1
//	  psq_lu @SX0_SY0, 4(@srcPosNorm), 0, SKN_GQR_POS
//	ps_madds0    @DZ1_nnn, @M21_nnn, @SY1_SZ1, @DZ1_nnn
//
//	// Apply third column and Write final values to temp W registers
//	ps_madds0    @WX0_WY0, @M02_M12, @SZ0_SX1, @DX0_DY0
//	ps_madds0    @WZ0_nnn, @M22_nnn, @SZ0_SX1, @DZ0_nnn
//	  psq_lu @SZ0_SX1, 4(@srcPosNorm), 0, SKN_GQR_POS
//	ps_madds1    @WX1_WY1, @M02_M12, @SY1_SZ1, @DX1_DY1
//	ps_madds1    @WZ1_nnn, @M22_nnn, @SY1_SZ1, @DZ1_nnn
//	  psq_lu @SY1_SZ1,4(@srcPosNorm), 0, SKN_GQR_POS
//
//	// -------------------------- LOOP START
//@mloop:
//	ps_madds0    @DX0_DY0, @M00_M10, @SX0_SY0, @M03_M13
//
//;//	ps_neg		 @WX1_WY1, @WX1_WY1
//		ps_div		@ONEZ, @ONE, @WZ0_nnn				// Calculate 1/z to 0
//	ps_madds0    @DZ0_nnn, @M20_nnn, @SX0_SY0, @M23_nnn
//		ps_madds0   @WX1_WY1, @WX0_WY0, @ONEZ, @WX1_WY1		// x,y * 1/z + nx,ny.
//	ps_muls1     @DX1_DY1, @M00_M10, @SZ0_SX1
//		ps_madd		 @WX1_WY1, @WX1_WY1, @HALF, @HALF		// (x.y*.5)+.5 instead of (x.y+1)*.5
//	ps_muls1     @DZ1_nnn, @M20_nnn, @SZ0_SX1
//		psq_stu     @WX1_WY1, 8(@dstBase), 0, SKN_GQR_UV
//	ps_madds1    @DX0_DY0, @M01_M11, @SX0_SY0, @DX0_DY0
//	ps_madds1    @DZ0_nnn, @M21_nnn, @SX0_SY0, @DZ0_nnn
//	  psq_lu @SX0_SY0, 4(@srcPosNorm), 0, SKN_GQR_POS // NEXT SX0 SY0
//	ps_madds0    @DX1_DY1, @M01_M11, @SY1_SZ1, @DX1_DY1
//	ps_madds0    @DZ1_nnn, @M21_nnn, @SY1_SZ1, @DZ1_nnn
//
//	// Write final values to temp registers
//	ps_madds0    @WX0_WY0, @M02_M12, @SZ0_SX1, @DX0_DY0
//	ps_madds0    @WZ0_nnn, @M22_nnn, @SZ0_SX1, @DZ0_nnn
//	  psq_lu @SZ0_SX1, 4(@srcPosNorm), 0, SKN_GQR_POS // NEXT SZ0 SX1
//	ps_madds1    @WX1_WY1, @M02_M12, @SY1_SZ1, @DX1_DY1
//	ps_madds1    @WZ1_nnn, @M22_nnn, @SY1_SZ1, @DZ1_nnn
//	  psq_lu @SY1_SZ1,4(@srcPosNorm), 0, SKN_GQR_POS // NEXT SY1 SZ1
//
//	bdnz+ @mloop    // -------------------------- LOOP END
//
//;//	ps_neg		 @WX1_WY1, @WX1_WY1
//	ps_div		@ONEZ, @ONE, @WZ0_nnn				// Calculate 1/z to 0
//	ps_madds0   @WX1_WY1, @WX0_WY0, @ONEZ, @WX1_WY1		// x,y * 1/z + nx,ny.
//	ps_madd	 @WX1_WY1, @WX1_WY1, @HALF, @HALF		// (x,y*.5)+.5 instead of (x,y+1)*.5
//
//	psq_stu     @WX1_WY1, 8(@dstBase), 0, SKN_GQR_UV
//
//
//@return:    
//	lfd     %f14, 8(%r1)
//	lfd     %f15, 16(%r1)
//	lfd     %f16, 24(%r1)
//	lfd     %f17, 32(%r1)
//	lfd     %f18, 40(%r1)
//	lfd     %f19, 48(%r1)
//	lfd     %f20, 56(%r1)
//	lfd     %f21, 64(%r1)
//	addi    %r1, %r1, 96
//	blr
//
//@half_data:
//	.float	(0.5)
//	.float	(0.5)
//	.float	(0.5)
//	.float	(0.5)
//
//@one_data:
//	.float	(1.0)
//	.float	(1.0)
//	.float	(1.0)
//	.float	(1.0)
//
//	.endscope
//
//;//---------------------------------------------------------------------
//;//---------------------------------------------------------------------
//;//---------------------------------------------------------------------
//
//
//
//










;//	# Input parameters.
;//	.equr		@m,				%r3	#    ROMtx   m
;//	.equr		@srcBase,		%r4	#    float  *srcBase
;//	.equr		@dstBase,		%r5	#    float  *dstBase
;//	.equr		@count,			%r6	#    u32     count
;//
;//	.equr		@M00_M10,		%f0
;//	.equr		@M20_nnn,		%f1
;//	.equr		@M01_M11,		%f2
;//	.equr		@M21_nnn,		%f3
;//	.equr		@M02_M12,		%f4
;//	.equr		@M22_nnn,		%f5
;//	.equr		@M03_M13,		%f6
;//	.equr		@M23_nnn,		%f7
;//
;//# source vectors - 2 3D vectors in 3 PS registers
;//	.equr		@SX0_SY0,		%f8
;//	.equr		@SZ0_SX1,		%f9
;//	.equr		@SY1_SZ1,		%f10
;//
;//# Destination registers - 2 3d vectors in 4 PS registers
;//	.equr		@DX0_DY0,		%f11
;//	.equr		@DZ0_nnn,		%f12
;//	.equr		@DX1_DY1,		%f13
;//	.equr		@DZ1_nnn,		%f14
;//
;//# temp registers for writing back values.  These registers store the final
;//# results from the PREVIOUS loop
;//	.equr		@WX0_WY0,		%f15
;//	.equr		@WZ0_nnn,		%f16
;//	.equr		@WX1_WY1,		%f17
;//	.equr		@WZ1_nnn,		%f18
;//
;//	stwu		%r1,			-64(%r1)
;//	stfd		%f14,			8(%r1)
;//	stfd		%f15,			16(%r1)
;//	addi		@count,			@count,			-1								# unrolled
;//	stfd		%f16,			24(%r1)
;//	stfd		%f17,			32(%r1)
;//	stfd		%f18,			40(%r1)
;//	mtctr		@count
;//	# load matrix
;//	psq_l   	@M00_M10,		0(@m),			0,				0  
;//	addi    	@srcBase,		@srcBase,		-8
;//	psq_l   	@M20_nnn,		8(@m),			1,				0  
;//	addi    	@dstBase,		@dstBase,		-4
;//	psq_l   	@M03_M13,		36(@m),			0,				0 
;//	psq_lu  	@SX0_SY0,		8(@srcBase),	0,				SKN_GQR_VERT
;//	psq_l   	@M23_nnn,		44(@m),			1,				0 
;//	psq_lu  	@SZ0_SX1,		8(@srcBase),	0,				SKN_GQR_VERT
;//
;//    # ------------------------------UNROLLED
;//
;//    # Apply first column and translation term
;//	ps_madds0	@DX0_DY0,		@M00_M10,		@SX0_SY0,		@M03_M13
;//	psq_l		@M01_M11,		12(@m),			0,				0
;//	ps_madds0	@DZ0_nnn,		@M20_nnn,		@SX0_SY0,		@M23_nnn
;//	psq_l		@M21_nnn,		20(@m),			1,				0   
;//	ps_muls1	@DX1_DY1,		@M00_M10,		@SZ0_SX1						# no trans for norms
;//	psq_lu		@SY1_SZ1,		8(@srcBase),	0,				SKN_GQR_VERT
;//	ps_muls1	@DZ1_nnn,		@M20_nnn,		@SZ0_SX1						# no trans for norms
;//	psq_l		@M22_nnn,		32(@m),			1,				0 
;//
;//    # Apply second column
;//	ps_madds1	@DX0_DY0,		@M01_M11,		@SX0_SY0,		@DX0_DY0
;//	ps_madds1	@DZ0_nnn,		@M21_nnn,		@SX0_SY0,		@DZ0_nnn
;//	psq_l		@M02_M12,		24(@m),			0,				0 
;//	ps_madds0	@DX1_DY1,		@M01_M11,		@SY1_SZ1,		@DX1_DY1
;//	psq_lu		@SX0_SY0,		8(@srcBase),	0,				SKN_GQR_VERT
;//	ps_madds0	@DZ1_nnn,		@M21_nnn,		@SY1_SZ1,		@DZ1_nnn
;//
;//	# Apply third column and Write final values to temp W registers
;//	ps_madds0	@WX0_WY0,		@M02_M12,		@SZ0_SX1,		@DX0_DY0
;//	ps_madds0	@WZ0_nnn,		@M22_nnn,		@SZ0_SX1,		@DZ0_nnn
;//	psq_lu		@SZ0_SX1,		8(@srcBase),	0,				SKN_GQR_VERT
;//	ps_madds1	@WX1_WY1,		@M02_M12,		@SY1_SZ1,		@DX1_DY1
;//	ps_madds1	@WZ1_nnn,		@M22_nnn,		@SY1_SZ1,		@DZ1_nnn
;//	psq_lu		@SY1_SZ1,		8(@srcBase),	0,				SKN_GQR_VERT
;//
;//	# -------------------------- LOOP START
;//@mloop:
;//	ps_madds0	@DX0_DY0,		@M00_M10,		@SX0_SY0,		@M03_M13
;//	psq_stu		@WX0_WY0,		4(@dstBase),	0,				SKN_GQR_VERT
;//	ps_madds0	@DZ0_nnn,		@M20_nnn,		@SX0_SY0,		@M23_nnn
;//	psq_stu		@WZ0_nnn,		8(@dstBase),	1,				SKN_GQR_VERT
;//	ps_muls1	@DX1_DY1,		@M00_M10,		@SZ0_SX1
;//	psq_stu		@WX1_WY1,		4(@dstBase),	0,				SKN_GQR_VERT
;//	ps_muls1	@DZ1_nnn,		@M20_nnn,		@SZ0_SX1
;//	psq_stu		@WZ1_nnn,		8(@dstBase),	1,				SKN_GQR_VERT
;//	ps_madds1	@DX0_DY0,		@M01_M11,		@SX0_SY0,		@DX0_DY0
;//	ps_madds1	@DZ0_nnn,		@M21_nnn,		@SX0_SY0,		@DZ0_nnn
;//	psq_lu		@SX0_SY0,		8(@srcBase),	0,				SKN_GQR_VERT	# NEXT SX0 SY0
;//	ps_madds0	@DX1_DY1,		@M01_M11,		@SY1_SZ1,		@DX1_DY1
;//	ps_madds0	@DZ1_nnn,		@M21_nnn,		@SY1_SZ1,		@DZ1_nnn
;//
;//	# Write final values to temp registers
;//	ps_madds0	@WX0_WY0,		@M02_M12,		@SZ0_SX1,		@DX0_DY0
;//	ps_madds0	@WZ0_nnn,		@M22_nnn,		@SZ0_SX1,		@DZ0_nnn
;//	psq_lu		@SZ0_SX1,		8(@srcBase),	0,				SKN_GQR_VERT	# NEXT SZ0 SX1
;//	ps_madds1	@WX1_WY1,		@M02_M12,		@SY1_SZ1,		@DX1_DY1
;//	ps_madds1	@WZ1_nnn,		@M22_nnn,		@SY1_SZ1,		@DZ1_nnn
;//	psq_lu		@SY1_SZ1,		8(@srcBase),	0,				SKN_GQR_VERT	# NEXT SY1 SZ1
;//
;//	bdnz+		@mloop
;//	# -------------------------- LOOP END
;//
;//	psq_stu		@WX0_WY0,		4(@dstBase),	0,				SKN_GQR_VERT
;//	psq_stu		@WZ0_nnn,		8(@dstBase),	1,				SKN_GQR_VERT
;//	psq_stu		@WX1_WY1,		4(@dstBase),	0,				SKN_GQR_VERT
;//	psq_stu		@WZ1_nnn,		8(@dstBase),	1,				SKN_GQR_VERT
;//
;//
;//@return:    
;//    lfd			%f14,			8(%r1)
;//    lfd			%f15,			16(%r1)
;//    lfd			%f16,			24(%r1)
;//    lfd			%f17,			32(%r1)
;//    lfd			%f18,			40(%r1)
;//    addi		%r1,			%r1,			64
;//    blr

#---------------------------------------------------------------------------
#  Name:         SKN1Vecs16Norms16
#
#  Description:  Transforms array of vertex-normal pairs.
#
#                Differs from a standard matrix-vector multiply in that the
#                normals are not translated.  Before you ask, no, there is 
#                no cycle gain from applying a 3x3 matrix vs. applying a 4x3
#                matrix.  Also, because we are always doing vertex-normal
#                pairs, no need to check for odd # of transforms, etc.
#
#  Arguments:    m           column-major matrix to apply
#                srcBase     ptr to source vert/norm pairs
#                dstBase     ptr to destination, can be same as source
#                count       number of vert/norm pairs.
#
#  Returns:      None.
#---------------------------------------------------------------------------

	.scope
	.global	TransformSingle
TransformSingle:

	.equr		@m,				%r3	#    ROMtx   m
	.equr		@srcBase,		%r4	#    s16*    srcBase
	.equr		@dstBase,		%r5	#    s16*  	 dstBase
	.equr		@count,			%r6	#    u32     count

	.equr		@M00_M10,		%f0
	.equr		@M20_nnn,		%f1
	.equr		@M01_M11,		%f2
	.equr		@M21_nnn,		%f3
	.equr		@M02_M12,		%f4
	.equr		@M22_nnn,		%f5
	.equr		@M03_M13,		%f6
	.equr		@M23_nnn,		%f7

# source vectors - 2 3D vectors in 3 PS registers
	.equr		@SX0_SY0,		%f8
	.equr		@SZ0_SX1,		%f9
	.equr		@SY1_SZ1,		%f10

# Destination registers - 2 3d vectors in 4 PS registers
	.equr		@DX0_DY0,		%f11
	.equr		@DZ0_nnn,		%f12
	.equr		@DX1_DY1,		%f13
	.equr		@DZ1_nnn,		%f14

# temp registers for writing back values.  These registers store the final
# results from the PREVIOUS loop
	.equr		@WX0_WY0,		%f15
	.equr		@WZ0_nnn,		%f16
	.equr		@WX1_WY1,		%f17
	.equr		@WZ1_nnn,		%f18

	stwu		%r1,			-64(%r1)
	stfd		%f14,			8(%r1)
	stfd		%f15,			16(%r1)
	addi		@count,			@count,			-1								# unrolled
	stfd		%f16,			24(%r1)
	stfd		%f17,			32(%r1)
	stfd		%f18,			40(%r1)
	mtctr		@count

	# load matrix
	psq_l		@M00_M10,		0(@m),			0,				0  
	addi		@srcBase,		@srcBase,		-4
	psq_l		@M20_nnn,		8(@m),			1,				0  
	addi		@dstBase,		@dstBase,		-2          	
	psq_l		@M03_M13,		36(@m),			0,				0 
	psq_lu		@SX0_SY0,		4(@srcBase),	0,				SKN_GQR_POS
	psq_l		@M23_nnn,		44(@m),			1,				0 
	psq_lu		@SZ0_SX1,		4(@srcBase),	0,				SKN_GQR_POS
	
	# ------------------------------UNROLLED                	
	
	# Apply first column and translation term               	
	ps_madds0	@DX0_DY0,		@M00_M10,		@SX0_SY0,		@M03_M13
	psq_l		@M01_M11,		12(@m),			0,				0
	ps_madds0	@DZ0_nnn,		@M20_nnn,		@SX0_SY0,		@M23_nnn
	psq_l		@M21_nnn,		20(@m),			1,				0   
	ps_muls1	@DX1_DY1,		@M00_M10,		@SZ0_SX1						# no trans for norms
	psq_lu		@SY1_SZ1,		4(@srcBase),	0,				SKN_GQR_POS
	ps_muls1	@DZ1_nnn,		@M20_nnn,		@SZ0_SX1						# no trans for norms
	psq_l		@M22_nnn,		32(@m),			1,				0 
	
	# Apply second column                                   	
	ps_madds1	@DX0_DY0,		@M01_M11,		@SX0_SY0,		@DX0_DY0
	ps_madds1	@DZ0_nnn,		@M21_nnn,		@SX0_SY0,		@DZ0_nnn
	psq_l		@M02_M12,		24(@m),			0,				0 
	ps_madds0	@DX1_DY1,		@M01_M11,		@SY1_SZ1,		@DX1_DY1
	psq_lu		@SX0_SY0,		4(@srcBase),	0,				SKN_GQR_POS
	ps_madds0	@DZ1_nnn,		@M21_nnn,		@SY1_SZ1,		@DZ1_nnn

	# Apply third column and Write final values to temp W registers
	ps_madds0	@WX0_WY0,		@M02_M12,		@SZ0_SX1,		@DX0_DY0
	ps_madds0	@WZ0_nnn,		@M22_nnn,		@SZ0_SX1,		@DZ0_nnn
	psq_lu		@SZ0_SX1,		4(@srcBase),	0,				SKN_GQR_POS
	ps_madds1	@WX1_WY1,		@M02_M12,		@SY1_SZ1,		@DX1_DY1
	ps_madds1	@WZ1_nnn,		@M22_nnn,		@SY1_SZ1,		@DZ1_nnn
	psq_lu		@SY1_SZ1,		4(@srcBase),	0,				SKN_GQR_POS

	# -------------------------- LOOP START
@_mloop:
	ps_madds0	@DX0_DY0,		@M00_M10,		@SX0_SY0,		@M03_M13
	psq_stu		@WX0_WY0,		2(@dstBase),	0,				SKN_GQR_POS
	ps_madds0	@DZ0_nnn,		@M20_nnn,		@SX0_SY0,		@M23_nnn
	psq_stu		@WZ0_nnn,		4(@dstBase),	1,				SKN_GQR_POS
	ps_muls1	@DX1_DY1,		@M00_M10,		@SZ0_SX1
	psq_stu		@WX1_WY1,		2(@dstBase),	0,				SKN_GQR_POS	//NORM
	ps_muls1	@DZ1_nnn,		@M20_nnn,		@SZ0_SX1
	psq_stu		@WZ1_nnn,		4(@dstBase),	1,				SKN_GQR_POS	//NORM
	ps_madds1	@DX0_DY0,		@M01_M11,		@SX0_SY0,		@DX0_DY0
	ps_madds1	@DZ0_nnn,		@M21_nnn,		@SX0_SY0,		@DZ0_nnn
	psq_lu		@SX0_SY0,		4(@srcBase),	0,				SKN_GQR_POS		# NEXT SX0 SY0
	ps_madds0	@DX1_DY1,		@M01_M11,		@SY1_SZ1,		@DX1_DY1
	ps_madds0	@DZ1_nnn,		@M21_nnn,		@SY1_SZ1,		@DZ1_nnn

	# Write final values to temp registers
	ps_madds0	@WX0_WY0,		@M02_M12,		@SZ0_SX1,		@DX0_DY0
	ps_madds0	@WZ0_nnn,		@M22_nnn,		@SZ0_SX1,		@DZ0_nnn
	psq_lu		@SZ0_SX1,		4(@srcBase),	0,				SKN_GQR_POS		# NEXT SZ0 SX1
	ps_madds1	@WX1_WY1,		@M02_M12,		@SY1_SZ1,		@DX1_DY1
	ps_madds1	@WZ1_nnn,		@M22_nnn,		@SY1_SZ1,		@DZ1_nnn
	psq_lu		@SY1_SZ1,		4(@srcBase),	0,				SKN_GQR_POS		# NEXT SY1 SZ1

	bdnz+		@_mloop    # -------------------------- LOOP END

	psq_stu		@WX0_WY0,		2(@dstBase),	0,				SKN_GQR_POS
	psq_stu		@WZ0_nnn,		4(@dstBase),	1,				SKN_GQR_POS
	psq_stu		@WX1_WY1,		2(@dstBase),	0,				SKN_GQR_POS	//NORM
	psq_stu		@WZ1_nnn,		4(@dstBase),	1,				SKN_GQR_POS //NORM


@_return:    
	lfd			%f14,			8(%r1)
	lfd			%f15,			16(%r1)
	lfd			%f16,			24(%r1)
	lfd			%f17,			32(%r1)
	lfd			%f18,			40(%r1)
	addi		%r1,			%r1,			64
	blr

	.endscope

#---------------------------------------------------------------------------
#  Name:         SKNVecs16Norms16NoTouch
#
#  Description:  Blends array of vertex-normal pairs between 2 matrices.
#                Note that normals are not translated
#
#                NOTE: no cache touches are done.  Should use locked cache
#                      to avoid cache pollution and misses.
#
#  Arguments:    m0          matrix 1
#                m1          matrix 2
#                wtBase      ptr to array of weights, 2 per vert/norm pair
#                srcBase     ptr to source vert/norm pairs
#                dstBase     ptr to destination, can be same as source
#                count       number of vert/norm pairs.
#
#  Returns:      None.
#---------------------------------------------------------------------------

	.scope
	.global	TransformDouble
TransformDouble:

	.equr		@m0,			%r3	#    ROMtx   m0
	.equr		@m1,			%r4	#    ROMtx   m1
	.equr		@wtBase,		%r5	#    s16*  wtBase
	.equr		@srcBase,		%r6	#    s16*  srcBase
	.equr		@dstBase,		%r7	#    s16*  dstBase
	.equr		@count,			%r8	#    u32     count

	.equr		@COUNTTEMP,		%r9
	.equr		@READAHEAD,		%r10

	.equr		@M00_M10,		%f0
	.equr		@M20_nnn,		%f1
	.equr		@M01_M11,		%f2
	.equr		@M21_nnn,		%f3
	.equr		@M02_M12,		%f4
	.equr		@M22_nnn,		%f5
	.equr		@M03_M13,		%f6
	.equr		@M23_nnn,		%f7

# source vectors - 2 3D vectors in 3 PS registers
	.equr		@SX0_SY0,		%f8
	.equr		@SZ0_SX1,		%f9
	.equr		@SY1_SZ1,		%f10

	.equr		@TX0_TY0,		%f11
	.equr		@TZ0_nnn,		%f12
	.equr		@TX1_TY1,		%f13
	.equr		@TZ1_nnn,		%f14 

# temp registers for writing back values.  These registers store the final
# results from the PREVIOUS loop
	.equr		@WX0_WY0,		%f15
	.equr		@WZ0_nnn,		%f16
	.equr		@WX1_WY1,		%f17
	.equr		@WZ1_nnn,		%f18

# second matrix
	.equr		@N00_N10,		%f19
	.equr		@N20_nnn,		%f20
	.equr		@N01_N11,		%f21
	.equr		@N21_nnn,		%f22
	.equr		@N02_N12,		%f23
	.equr		@N22_nnn,		%f24
	.equr		@N03_N13,		%f25
	.equr		@N23_nnn,		%f26

# current weight for 1 vert
	.equr		@WT0,			%f27

    stwu		%r1,			-160(%r1)
    stfd		%f14,			8(%r1)
	# unrolled once
    addi		@COUNTTEMP,		@count,			-1
	stfd		%f15,			16(%r1)
	stfd		%f16,			24(%r1)
	stfd		%f17,			32(%r1)
	stfd		%f18,			40(%r1)
	stfd		%f19,			48(%r1)
	stfd		%f20,			56(%r1)
	stfd		%f21,			64(%r1)
    addi		@READAHEAD,		%r0,			32
	stfd		%f22,			72(%r1)
	stfd		%f23,			80(%r1)
	stfd		%f24,			88(%r1)
	stfd		%f25,			96(%r1)
	stfd		%f26,			104(%r1)
	stfd		%f27,			112(%r1)

    mtctr		@COUNTTEMP
	# load matrix
	psq_l		@M00_M10,		0(@m0),			0,				0  
	addi		@srcBase,		@srcBase,		-4
	psq_l		@M20_nnn,		8(@m0),			1,				0  
	addi		@dstBase,		@dstBase,		-2
	psq_l		@M03_M13,		36(@m0),		0,				0 
	addi		@wtBase,		@wtBase,		-4
	psq_lu		@SX0_SY0,		4(@srcBase),	0,				SKN_GQR_POS
	psq_l		@M23_nnn,		44(@m0),		1,				0 
	psq_lu		@SZ0_SX1,		4(@srcBase),	0,				SKN_GQR_POS

    # ------------------------------UNROLLED
	psq_lu		@WT0,			4(@wtBase),		0,				SKN_GQR_NORM
	
	# Apply first column and translation term
	ps_madds0	@WX0_WY0,		@M00_M10,		@SX0_SY0,		@M03_M13
	psq_l		@M01_M11,		12(@m0),		0,				0 
	ps_madds0	@WZ0_nnn,		@M20_nnn,		@SX0_SY0,		@M23_nnn
	psq_l		@M21_nnn,		20(@m0),		1,				0   
	ps_muls1	@WX1_WY1,		@M00_M10,		@SZ0_SX1						# no translation for normals
	psq_lu		@SY1_SZ1,		4(@srcBase),	0,				SKN_GQR_POS
	ps_muls1	@WZ1_nnn,		@M20_nnn,		@SZ0_SX1						# no translation for normals
	psq_l		@M22_nnn,		32(@m0),		1,				0 
	
	# Apply second column
	ps_madds1	@WX0_WY0,		@M01_M11,		@SX0_SY0,		@WX0_WY0
	psq_l		@N00_N10,		0(@m1),			0,				0  
	ps_madds1	@WZ0_nnn,		@M21_nnn,		@SX0_SY0,		@WZ0_nnn
	psq_l		@M02_M12,		24(@m0),		0,				0 
	ps_madds0	@WX1_WY1,		@M01_M11,		@SY1_SZ1,		@WX1_WY1
	psq_l		@N20_nnn,		8(@m1),			0,				0  
	ps_madds0	@WZ1_nnn,		@M21_nnn,		@SY1_SZ1,		@WZ1_nnn
	psq_l		@N01_N11,		12(@m1),		0,				0  
	
	# Apply third column and Write final values to temp W registers
	ps_madds0	@WX0_WY0,		@M02_M12,		@SZ0_SX1,		@WX0_WY0
	psq_l		@N21_nnn,		20(@m1),		0,				0  
	ps_madds0	@WZ0_nnn,		@M22_nnn,		@SZ0_SX1,		@WZ0_nnn
	psq_l		@N02_N12,		24(@m1),		0,				0  
	ps_madds1	@WX1_WY1,		@M02_M12,		@SY1_SZ1,		@WX1_WY1
	psq_l		@N22_nnn,		32(@m1),		0,				0  
	ps_madds1	@WZ1_nnn,		@M22_nnn,		@SY1_SZ1,		@WZ1_nnn
	psq_l		@N03_N13,		36(@m1),		0,				0  
	
	# now apply weights to W vectors
	ps_muls0	@WX0_WY0,		@WX0_WY0,		@WT0
	psq_l		@N23_nnn,		44(@m1),		0,				0  
	ps_muls0	@WZ0_nnn,		@WZ0_nnn,		@WT0
	ps_muls0	@WX1_WY1,		@WX1_WY1,		@WT0
	ps_muls0	@WZ1_nnn,		@WZ1_nnn,		@WT0
	
	
	# now transform by second matrix into TX* registers
	ps_madds0	@TX0_TY0,		@N00_N10,		@SX0_SY0,		@N03_N13
	ps_madds0	@TZ0_nnn,		@N20_nnn,		@SX0_SY0,		@N23_nnn
	ps_muls1	@TX1_TY1,		@N00_N10,		@SZ0_SX1						# no translation for normals
	ps_muls1	@TZ1_nnn,		@N20_nnn,		@SZ0_SX1						# no translation for normals
	ps_madds1	@TX0_TY0,		@N01_N11,		@SX0_SY0,		@TX0_TY0
	ps_madds1	@TZ0_nnn,		@N21_nnn,		@SX0_SY0,		@TZ0_nnn
	ps_madds0	@TX1_TY1,		@N01_N11,		@SY1_SZ1,		@TX1_TY1
	psq_lu		@SX0_SY0,		4(@srcBase),	0,				SKN_GQR_POS	# XXX LOAD NEW
	ps_madds0	@TZ1_nnn,		@N21_nnn,		@SY1_SZ1,		@TZ1_nnn
	
	# Write final values to temp registers
	ps_madds0	@TX0_TY0,		@N02_N12,		@SZ0_SX1,		@TX0_TY0
	ps_madds0	@TZ0_nnn,		@N22_nnn,		@SZ0_SX1,		@TZ0_nnn
	ps_madds1	@TX1_TY1,		@N02_N12,		@SY1_SZ1,		@TX1_TY1
	ps_madds1	@TZ1_nnn,		@N22_nnn,		@SY1_SZ1,		@TZ1_nnn
	psq_lu		@SZ0_SX1,		4(@srcBase),	0,				SKN_GQR_POS	# XXX LOAD NEW
	
	# madd the second xformed verts with temps, and store them out
	ps_madds1	@TX0_TY0,		@TX0_TY0,		@WT0,			@WX0_WY0
	ps_madds1	@TZ0_nnn,		@TZ0_nnn,		@WT0,			@WZ0_nnn
	ps_madds1	@TX1_TY1,		@TX1_TY1,		@WT0,			@WX1_WY1
	psq_lu		@SY1_SZ1,		4(@srcBase),	0,				SKN_GQR_POS	# XXX LOAD NEW
	ps_madds1	@TZ1_nnn,		@TZ1_nnn,		@WT0,			@WZ1_nnn
	
	# -------------------------- LOOP START
@_mloop:



	ps_madds0	@WX0_WY0,		@M00_M10,		@SX0_SY0,		@M03_M13
	psq_stu		@TX0_TY0,		2(@dstBase),	0,				SKN_GQR_POS
	ps_madds0	@WZ0_nnn,		@M20_nnn,		@SX0_SY0,		@M23_nnn
	psq_stu		@TZ0_nnn,		4(@dstBase),	1,				SKN_GQR_POS
	ps_muls1	@WX1_WY1,		@M00_M10,		@SZ0_SX1						# no translation for normals
	psq_stu		@TX1_TY1,		2(@dstBase),	0,				SKN_GQR_POS //NORM
	ps_muls1	@WZ1_nnn,		@M20_nnn,		@SZ0_SX1						# no translation for normals
	psq_stu		@TZ1_nnn,		4(@dstBase),	1,				SKN_GQR_POS //NORM
	
	#  DX0=M01*SY0+DX0, DY0=M11*SY0+DY0
	#  DZ0=M21*SY0+DZ0
	#  DX1=M01*SY1+DX1, DY1=M11*SY1+DY1
	#  DZ1=M21*SY1+DZ1
	
	ps_madds1	@WX0_WY0,		@M01_M11,		@SX0_SY0,		@WX0_WY0
	ps_madds1	@WZ0_nnn,		@M21_nnn,		@SX0_SY0,		@WZ0_nnn
	dcbt		@READAHEAD,		@srcBase 		         		
	ps_madds0	@WX1_WY1,		@M01_M11,		@SY1_SZ1,		@WX1_WY1
	ps_madds0	@WZ1_nnn,		@M21_nnn,		@SY1_SZ1,		@WZ1_nnn
	
	#  DX0=M02*SZ0+DX0, DY0=M12*SZ0+DY0
	#  DZ0=M22*SZ0+DZ0
	#  DX1=M02*SZ1+DX1, DY1=M12*SZ1+DY1
	#  DZ1=M22*SZ1+DZ1
	
	# Write final values to temp registers
	ps_madds0	@WX0_WY0,		@M02_M12,		@SZ0_SX1,		@WX0_WY0
	ps_madds0	@WZ0_nnn,		@M22_nnn,		@SZ0_SX1,		@WZ0_nnn
	psq_lu		@WT0,			4(@wtBase),		0,				SKN_GQR_NORM
	ps_madds1	@WX1_WY1,		@M02_M12,		@SY1_SZ1,		@WX1_WY1
	ps_madds1	@WZ1_nnn,		@M22_nnn,		@SY1_SZ1,		@WZ1_nnn
	
	
	# now apply weights to w vectors
	ps_muls0	@WX0_WY0,		@WX0_WY0,		@WT0
	ps_muls0	@WZ0_nnn,		@WZ0_nnn,		@WT0
	ps_muls0	@WX1_WY1,		@WX1_WY1,		@WT0
	ps_muls0	@WZ1_nnn,		@WZ1_nnn,		@WT0
	
	
	# now transform by second matrix into TX* registers
	ps_madds0	@TX0_TY0,		@N00_N10,		@SX0_SY0,		@N03_N13
	ps_madds0	@TZ0_nnn,		@N20_nnn,		@SX0_SY0,		@N23_nnn
	dcbt		@READAHEAD,		@dstBase
	ps_muls1	@TX1_TY1,		@N00_N10,		@SZ0_SX1						# no translation for normals
	ps_muls1	@TZ1_nnn,		@N20_nnn,		@SZ0_SX1						# no translation for normals
	ps_madds1	@TX0_TY0,		@N01_N11,		@SX0_SY0,		@TX0_TY0
	ps_madds1	@TZ0_nnn,		@N21_nnn,		@SX0_SY0,		@TZ0_nnn
	ps_madds0	@TX1_TY1,		@N01_N11,		@SY1_SZ1,		@TX1_TY1
	psq_lu		@SX0_SY0,		4(@srcBase),	0,				SKN_GQR_POS	# XXX LOAD NEW
	ps_madds0	@TZ1_nnn,		@N21_nnn,		@SY1_SZ1,		@TZ1_nnn
	
	# Write final values to temp registers
	ps_madds0	@TX0_TY0,		@N02_N12,		@SZ0_SX1,		@TX0_TY0
	ps_madds0	@TZ0_nnn,		@N22_nnn,		@SZ0_SX1,		@TZ0_nnn
	ps_madds1	@TX1_TY1,		@N02_N12,		@SY1_SZ1,		@TX1_TY1
	ps_madds1	@TZ1_nnn,		@N22_nnn,		@SY1_SZ1,		@TZ1_nnn
	psq_lu		@SZ0_SX1,		4(@srcBase),	0,				SKN_GQR_POS	# XXX LOAD NEW
	
	# madd the second xformed verts with temps, and store them out
	ps_madds1	@TX0_TY0,		@TX0_TY0,		@WT0,			@WX0_WY0
	ps_madds1	@TZ0_nnn,		@TZ0_nnn,		@WT0,			@WZ0_nnn
	ps_madds1	@TX1_TY1,		@TX1_TY1,		@WT0,			@WX1_WY1
	psq_lu		@SY1_SZ1,		4(@srcBase),	0,				SKN_GQR_POS	# XXX LOAD NEW
	ps_madds1	@TZ1_nnn,		@TZ1_nnn,		@WT0,			@WZ1_nnn
	
	
	
	bdnz+		@_mloop    # -------------------------- LOOP END
	
	psq_stu		@TX0_TY0,		2(@dstBase),	0,				SKN_GQR_POS
	psq_stu		@TZ0_nnn,		4(@dstBase),	1,				SKN_GQR_POS
	psq_stu		@TX1_TY1,		2(@dstBase),	0,				SKN_GQR_POS	//NORM
	psq_stu		@TZ1_nnn,		4(@dstBase),	1,				SKN_GQR_POS //NORM
	
	
@_return:    
	lfd			%f14,			8(%r1)
	lfd			%f15,			16(%r1)
	lfd			%f16,			24(%r1)
	lfd			%f17,			32(%r1)
	lfd			%f18,			40(%r1)
	lfd			%f19,			48(%r1)
	lfd			%f20,			56(%r1)
	lfd			%f21,			64(%r1)
	lfd			%f22,			72(%r1)
	lfd			%f23,			80(%r1)
	lfd			%f24,			88(%r1)
	lfd			%f25,			96(%r1)
	lfd			%f26,			104(%r1)
	lfd			%f27,			112(%r1)
	addi		%r1,			%r1,			160
	blr

	.endscope

#---------------------------------------------------------------------------
#  Name:         SKNAccVecs16Norms16Iu16
#
#  Description:  General accumulation code
#                Assumes source data is duplicated in linear fashion
#                Assumes u16 indices
#
#                Loop cannot be unrolled since we must handle 1 length lists
#                as well
#
#  Arguments:    m       current animation matrix
#                count   number of vertex-normal pairs
#                vnlist  source data for vertex-normal pairs, pre-blended
#                dest    destination array
#                indices indices into the destination array
#                weights list of weights, use GQ%r6
#
#  Returns:      None.
#---------------------------------------------------------------------------
	.scope
	.global	TransformAcc
TransformAcc:

	.equr		@m,				%r3	#	ROMtx      m
	.equr		@count,			%r4	#	u16        count
	.equr		@srcBase,		%r5	#	float     *srcBase
	.equr		@dstBase,		%r6	#	s16     *dstBase
	.equr		@indices,		%r7	#	u16       *indices
	.equr		@weights,		%r8	#	s16     *weights

	.equr		@M00_M10,		%f0
	.equr		@M20_nnn,		%f1
	.equr		@M01_M11,		%f2
	.equr		@M21_nnn,		%f3
	.equr		@M02_M12,		%f4
	.equr		@M22_nnn,		%f5
	.equr		@M03_M13,		%f6
	.equr		@M23_nnn,		%f7

# source vectors - 2 3D vectors in 3 PS registers
	.equr		@SX0_SY0,		%f8
	.equr		@SZ0_SX1,		%f9
	.equr		@SY1_SZ1,		%f10

# Destination registers - 2 3d vectors in 4 PS registers
	.equr		@DX0_DY0,		%f11
	.equr		@DZ0_nnn,		%f12
	.equr		@DX1_DY1,		%f13
	.equr		@DZ1_nnn,		%f14

# accumulation registers
	.equr		@AX0_AY0,		%f19
	.equr		@AZ0_nnn,		%f20
	.equr		@AX1_AY1,		%f21
	.equr		@AZ1_nnn,		%f22

# temp registers for writing back values.  These registers store the final
# results from the PREVIOUS loop
	.equr		@WX0_WY0,		%f15
	.equr		@WZ0_nnn,		%f16
	.equr		@WX1_WY1,		%f17
	.equr		@WZ1_nnn,		%f18

	.equr		@WEIGHT,		%f23

	.equr		@INDEX,			%r9
	.equr		@DESTADDR,		%r10
	.equr		@INDEXTMP,		%r11

	stwu		%r1,			-160(%r1)
	addi		@indices,		@indices,		-2								# for load-update
	stfd		%f14,			8(%r1)
	lhzu		@INDEX,			2(@indices)										# index for first destination
	stfd		%f15,			16(%r1)
	stfd		%f16,			24(%r1)
	add			@INDEXTMP,		@INDEX,			@INDEX							# addr for dest = dest + (index*3*sizeof (s16))
	stfd		%f17,			32(%r1)
	add			@INDEX,			@INDEX,			@INDEXTMP						# addr for dest = dest + (index*3*sizeof (s16))
	stfd		%f18,			40(%r1)
	add			@INDEX,			@INDEX,			@INDEX							# x 2 for s16
	stfd		%f19,			48(%r1)
	add			@INDEX,			@INDEX,			@INDEX							# x 2 for vert/norm
	stfd		%f20,			56(%r1)
	add			@DESTADDR,		@INDEX,			@dstBase
	stfd		%f21,			64(%r1)
	addi		@weights,		@weights,		-2
	stfd		%f22,			72(%r1)
	stfd		%f23,			80(%r1)

	mtctr		@count

	psq_l		@M00_M10,		0(@m),			0,				0
	addi		@srcBase,		@srcBase,		-4
	psq_l		@M20_nnn,		8(@m),			1,				0
	psq_l		@M03_M13,		36(@m),			0,				0
	psq_l		@M23_nnn,		44(@m),			1,				0
	psq_lu		@SX0_SY0,		4(@srcBase),	0,				SKN_GQR_POS
	psq_lu		@SZ0_SX1,		4(@srcBase),	0,				SKN_GQR_POS


	psq_l		@M01_M11,		12(@m),			0,				0
	psq_l		@M21_nnn,		20(@m),			1,				0
	psq_lu		@SY1_SZ1,		4(@srcBase),	0,				SKN_GQR_POS
	psq_l		@M22_nnn,		32(@m),			1,				0
	psq_l		@M02_M12,		24(@m),			0,				0

    # -------------------------- LOOP START
@mloop:
	ps_madds0	@DX0_DY0,		@M00_M10,		@SX0_SY0,		@M03_M13
	psq_lu		@WEIGHT,		2(@weights),	1,				SKN_GQR_NORM
	ps_madds0	@DZ0_nnn,		@M20_nnn,		@SX0_SY0,		@M23_nnn
	ps_muls1	@DX1_DY1,		@M00_M10,		@SZ0_SX1
	psq_l		@AX0_AY0,		0(@DESTADDR),	0,				SKN_GQR_POS
	ps_muls1	@DZ1_nnn,		@M20_nnn,		@SZ0_SX1

	ps_madds1	@DX0_DY0,		@M01_M11,		@SX0_SY0,		@DX0_DY0
	ps_madds1	@DZ0_nnn,		@M21_nnn,		@SX0_SY0,		@DZ0_nnn
	psq_l		@AZ0_nnn,		4(@DESTADDR),	1,				SKN_GQR_POS
	ps_madds0	@DX1_DY1,		@M01_M11,		@SY1_SZ1,		@DX1_DY1
	ps_madds0	@DZ1_nnn,		@M21_nnn,		@SY1_SZ1,		@DZ1_nnn
	psq_l		@AX1_AY1,		6(@DESTADDR),	0,				SKN_GQR_POS
	ps_madds0	@WX0_WY0,		@M02_M12,		@SZ0_SX1,		@DX0_DY0
	ps_madds0	@WZ0_nnn,		@M22_nnn,		@SZ0_SX1,		@DZ0_nnn
	lhzu		@INDEX,			2(@indices)										# index for destination
	ps_madds1	@WX1_WY1,		@M02_M12,		@SY1_SZ1,		@DX1_DY1
	ps_madds1	@WZ1_nnn,		@M22_nnn,		@SY1_SZ1,		@DZ1_nnn
	psq_l		@AZ1_nnn,		10(@DESTADDR),	1,				SKN_GQR_POS
	# accumulate
	ps_madds0	@AX0_AY0,		@WX0_WY0,		@WEIGHT,		@AX0_AY0
	psq_lu		@SX0_SY0,		4(@srcBase),	0,				SKN_GQR_POS	# NEXT data
	ps_madds0	@AZ0_nnn,		@WZ0_nnn,		@WEIGHT,		@AZ0_nnn
	psq_lu		@SZ0_SX1,		4(@srcBase),	0,				SKN_GQR_POS	# NEXT data
	ps_madds0	@AX1_AY1,		@WX1_WY1,		@WEIGHT,		@AX1_AY1
	psq_lu		@SY1_SZ1,		4(@srcBase),	0,				SKN_GQR_POS	# NEXT data
	ps_madds0	@AZ1_nnn,		@WZ1_nnn,		@WEIGHT,		@AZ1_nnn
	add			@INDEXTMP,		@INDEX,			@INDEX							# NEXT DESTADDR
	psq_st		@AX0_AY0,		0(@DESTADDR),	0,				SKN_GQR_POS
	add			@INDEX,			@INDEX,			@INDEXTMP						# NEXT DESTADDR
	psq_st		@AZ0_nnn,		4(@DESTADDR),	1,				SKN_GQR_POS
	add			@INDEX,			@INDEX,			@INDEX							# NEXT DESTADDR
	psq_st		@AX1_AY1,		6(@DESTADDR),	0,				SKN_GQR_POS	//NORM
	add			@INDEX,			@INDEX,			@INDEX							# NEXT DESTADDR
	psq_st		@AZ1_nnn,		10(@DESTADDR),	1,				SKN_GQR_POS	//NORM
	dcbst		0,				@DESTADDR										# FLUSH data to memory
	add			@DESTADDR,		@INDEX,			@dstBase						# NEXT DESTADDR

	bdnz+		@mloop
	# -------------------------- LOOP END


@return:    
    lfd			%f14,			8(%r1)
    lfd			%f15,			16(%r1)
    lfd			%f16,			24(%r1)
    lfd			%f17,			32(%r1)
    lfd			%f18,			40(%r1)
    lfd			%f19,			48(%r1)
    lfd			%f20,			56(%r1)
    lfd			%f21,			64(%r1)
    lfd			%f22,			72(%r1)
    lfd			%f23,			80(%r1)
    addi		%r1,			%r1,			160
    blr





