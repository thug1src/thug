//--- Highest bit indicates presence of return value
#define EzBGM_INIT           0x8000
#define EzBGM_QUIT           0x0010
#define EzBGM_OPEN           0x8020
#define EzBGM_CLOSE          0x0030
#define EzBGM_PRELOAD        0x0040
#define EzBGM_START          0x0050
#define EzBGM_STOP           0x0060
#define EzBGM_SEEK           0x0070
#define EzBGM_SETVOL         0x8080
#define EzBGM_SETVOLDIRECT   0x8090
#define EzBGM_CLOSE_NO_WAIT  0x0100
//#define EzBGM_SETMASTERVOL 0x00a0
#define EzBGM_GETMODE        0x80b0
#define EzBGM_SETMODE        0x80c0
#define EzBGM_SDINIT         0x00d0


//-- SET AVAILABLE
#define BGM_MODE_REPEAT_OFF      0x0000
#define BGM_MODE_REPEAT_DEFAULT  0x0001
#define BGM_MODE_REPEAT_FORCED   0x0002

#define BGM_MODE_STEREO          0x0000
#define BGM_MODE_MONO            0x0010

//-- GET ONLY
#define BGM_MODE_IDLE            0x0000
#define BGM_MODE_RUNNING         0x1000
#define BGM_MODE_PAUSE           0x2000
//#define BGM_MODE_KICKSTART       0x4000
#define BGM_MODE_TERMINATE       0x8000


#define BGM_MASK_STATUS          0x0FFF
#define BGM_MASK_REPEAT          0xFFF0
#define BGM_MASK_STEREO          0xFF0F

#define WAV_STEREO_BIT           ( 1 << 1 )


/* ----------------------------------------------
   module ID number
  ----------------------------------------------- */
#define EZBGM_DEV   0x000666


#define PRINTF(x) printf x
//#define PRINTF(x) 

#define ERROR(x) printf x
//#define ERROR(x) 

#define BASE_priority  42

#define OLDLIB 0
#define TRANS_CH  0

