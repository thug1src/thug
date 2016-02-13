# register numbers for mfspr/mtspr

	.equ	spr_xer,1
	.equ	spr_lr,8
	.equ	spr_ctr,9

	.equ	spr_upmc1,937
	.equ	spr_upmc2,938
	.equ	spr_upmc3,941
	.equ	spr_upmc4,942

	.equ	spr_usia,939

	.equ	spr_ummcr0,936
	.equ	spr_ummcr1,940

	.equ	spr_hid0,1008
	.equ	spr_hid1,1009

	.equ	spr_pvr,287

	.equ	spr_ibat0u,528
	.equ	spr_ibat0l,529
	.equ	spr_ibat1u,530
	.equ	spr_ibat1l,531
	.equ	spr_ibat2u,532
	.equ	spr_ibat2l,533
	.equ	spr_ibat3u,534
	.equ	spr_ibat3l,535

	.equ	spr_dbat0u,536
	.equ	spr_dbat0l,537
	.equ	spr_dbat1u,538
	.equ	spr_dbat1l,539
	.equ	spr_dbat2u,540
	.equ	spr_dbat2l,541
	.equ	spr_dbat3u,542
	.equ	spr_dbat3l,543

	.equ	spr_sdr1,25

	.equ	spr_sprg0,272
	.equ	spr_sprg1,273
	.equ	spr_sprg2,274
	.equ	spr_sprg3,275

	.equ	spr_dar,19
	.equ	spr_dsisr,18

	.equ	spr_srr0,26
	.equ	spr_srr1,27

	.equ	spr_ear,282

	.equ	spr_dabr,1013

	.equ	spr_tbl,284
	.equ	spr_tbu,285

	.equ	spr_l2cr,1017

	.equ	spr_dec,22

	.equ	spr_iabr,1010

	.equ	spr_pmc1,953
	.equ	spr_pmc2,954
	.equ	spr_pmc3,957
	.equ	spr_pmc4,958

	.equ	spr_sia,955

	.equ	spr_mmcr0,952
	.equ	spr_mmcr1,956

	.equ	spr_thrm1,1020
	.equ	spr_thrm2,1021
	.equ	spr_thrm3,1022

	.equ	spr_ictc,1019

# gekko registers
	.equ	spr_gqr0,912
	.equ	spr_gqr1,913
	.equ	spr_gqr2,914
	.equ	spr_gqr3,915
	.equ	spr_gqr4,916
	.equ	spr_gqr5,917
	.equ	spr_gqr6,918
	.equ	spr_gqr7,919

	.equ	spr_hid2,920

	.equ	spr_wpar,921

	.equ	spr_dmau,922
	.equ	spr_dmal,923
# end of gekko registers

	.equ	MSR_POW,0x00040000	# Power Management
	.equ	MSR_ILE,0x00010000	# Interrupt Little Endian
	.equ	MSR_EE,0x00008000	# external interrupt
	.equ	MSR_PR,0x00004000	# privilege level(should be 0)
	.equ	MSR_FP,0x00002000	# floating point available
	.equ	MSR_ME,0x00001000	# machine check enable
	.equ	MSR_FE0,0x00000800	# floating point exception enable
	.equ	MSR_SE,0x00000400	# single step trace enable
	.equ	MSR_BE,0x00000200	# branch trace enable
	.equ	MSR_FE1,0x00000100	# floating point exception enable
	.equ	MSR_IP,0x00000040	# Exception prefix
	.equ	MSR_IR,0x00000020	# instruction relocate
	.equ	MSR_DR,0x00000010	# data relocate
	.equ	MSR_PM,0x00000004	# Performance monitor marked mode
	.equ	MSR_RI,0x00000002	# Recoverable interrupt
	.equ	MSR_LE,0x00000001	# Little Endian

	.equ	MSR_POW_BIT,13		# Power Management
	.equ	MSR_ILE_BIT,15		# Interrupt Little Endian
	.equ	MSR_EE_BIT,16		# external interrupt
	.equ	MSR_PR_BIT,17		# privilege level (should be 0)
	.equ	MSR_FP_BIT,18		# floating point available
	.equ	MSR_ME_BIT,19		# machine check enable
	.equ	MSR_FE0_BIT,20		# floating point exception enable
	.equ	MSR_SE_BIT,21		# single step trace enable
	.equ	MSR_BE_BIT,22		# branch trace enable
	.equ	MSR_FE1_BIT,23		# floating point exception enable
	.equ	MSR_IP_BIT,25		# Exception prefix
	.equ	MSR_IR_BIT,26		# instruction relocate
	.equ	MSR_DR_BIT,27		# data relocate
	.equ	MSR_PM_BIT,29		# Performance monitor marked mode
	.equ	MSR_RI_BIT,30		# Recoverable interrupt
	.equ	MSR_LE_BIT,31		# Little Endian


	.equ	HID2_LSQE,0x80000000	# L/S quantize enable
	.equ	HID2_WPE,0x40000000	# Write pipe enable
	.equ	HID2_PSE,0x20000000	# Paired single enable
	.equ	HID2_LCE,0x10000000	# Locked cache enable

	.equ	HID2_DCHERR,0x00800000	# ERROR: dcbz_l cache hit
	.equ	HID2_DNCERR,0x00400000	# ERROR: DMA access to normal cache
	.equ	HID2_DCMERR,0x00200000	# ERROR: DMA cache miss error
	.equ	HID2_DQOERR,0x00100000	# ERROR: DMA queue overflow
	.equ	HID2_DCHEE,0x00080000	# dcbz_l cache hit error enable
	.equ	HID2_DNCEE,0x00040000	# DMA access to normal cache error enable
	.equ	HID2_DCMEE,0x00020000	# DMA cache miss error error enable
	.equ	HID2_DQOEE,0x00010000	# DMA queue overflow error enable

	.equ	HID2_DMAQL_MASK,0x0F000000	# DMA queue length mask
	.equ	HID2_DMAQL_SHIFT,24		# DMA queue shift

	.equ	HID2_LSQE_BIT,0
	.equ	HID2_WPE_BIT,1
	.equ	HID2_PSE_BIT,2
	.equ	HID2_LCE_BIT,3

	.equ	HID2_DCHERR_BIT,8   
	.equ	HID2_DNCERR_BIT,9
	.equ	HID2_DCMERR_BIT,10
	.equ	HID2_DQOERR_BIT,11
	.equ	HID2_DCHEE_BIT,12
	.equ	HID2_DNCEE_BIT,13
	.equ	HID2_DCMEE_BIT,14
	.equ	HID2_DQOEE_BIT,15

#------------------------------------------

	.equ	EX_SYSTEM_RESET,0
	.equ	EX_MACHINE_CHECK,1
	.equ	EX_DSI,2
	.equ	EX_ISI,3
	.equ	EX_EXTERNAL_INTERRUPT,4
	.equ	EX_ALIGNMENT,5
	.equ	EX_PROGRAM,6
	.equ	EX_FLOATING_POINT,7
	.equ	EX_DECREMENTER,8
	.equ	EX_SYSTEM_CALL,9
	.equ	EX_TRACE,10
	.equ	EX_PERFORMANCE_MONITOR,11
	.equ	EX_BREAKPOINT,12
	.equ	EX_SYSTEM_INTERRUPT,13
	.equ	EX_THERMAL_INTERRUPT,14
	.equ	EX_MAX,15

	.equ	REAL_EX_SYSTEM_RESET,0
	.equ	REAL_EX_MACHINE_CHECK,1
	.equ	REAL_EX_DSI,2
	.equ	REAL_EX_ISI,3
	.equ	REAL_EX_EXTERNAL_INTERRUPT,4
	.equ	REAL_EX_ALIGNMENT,5
	.equ	REAL_EX_PROGRAM,6
	.equ	REAL_EX_FLOATING_POINT,7
	.equ	REAL_EX_DECREMENTER,8
	.equ	REAL_EX_SYSTEM_CALL,11
	.equ	REAL_EX_TRACE,12
	.equ	REAL_EX_PERFORMANCE_MONITOR,14
	.equ	REAL_EX_BREAKPOINT,18
	.equ	REAL_EX_SYSTEM_INTERRUPT,21
	.equ	REAL_EX_THERMAL_INTERRUPT,22
	.equ	REAL_EX_MAX,15

#------------------------------------------

	.equ	CMD_OK,0
	.equ	CMD_PK_2SMALL,1
	.equ	CMD_COMMSERR,2

#-----------------------------------------
# Boot Info2 is a 8K byte structure that is loaded to himem
# (lower than FST) by apploader.

	.equ	BOOTINFO2_ADDR,0x800000F4

	.equ	OS_BI2_SIZE,0x2000      			# 8K 
	.equ	OS_BI2_DEBUGMONSIZE_OFFSET,0x0000
	.equ	OS_BI2_SIMMEMSIZE_OFFSET,0x0004
	.equ	OS_BI2_ARGOFFSET_OFFSET,0x0008
	.equ	OS_BI2_DEBUGFLAG_OFFSET,0x000c
	.equ	OS_BI2_TRKLOCATION_OFFSET,0x0010
	.equ	OS_BI2_TRKSIZE_OFFSET,0x0014
	.equ	OS_BI2_COUNTRYCODE_OFFSET,0x0018
	.equ	OS_BI2_ARGSIZE_MAX,0x1000      		# 4K
	.equ	OS_BI2_COUNTRYCODE_JP,0
	.equ	OS_BI2_COUNTRYCODE_US,1

	.equ	ARENAHI_ADDR,0x80000034
