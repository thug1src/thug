#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <eekernel.h>
#include <sifdev.h>
#include "gel/movies/ngps/defs.h"
#include "gel/movies/ngps/strfile.h"
#include <sys/config/config.h>

namespace Flx
{



int isStrFileInit = 0;

// ////////////////////////////////////////////////////////////////
//
//  Open a file to read and check its size
//
//	< file name conversion >
//	   = from HD =
//	    dir/file.pss           -> host0:dir/file.pss
//	    host0:dir/file.pss     -> host0:dir/file.pss
//
//	   = from CD/DVD =
//	    cdrom0:\dir\file.pss;1 -> cdrom0:\DIR\FILE.PSS;1
//	    cdrom0:/dir/file.pss;1 -> cdrom0:\DIR\FILE.PSS;1
//	    cdrom0:/dir/file.pss   -> cdrom0:\DIR\FILE.PSS;1
//
int strFileOpen(StrFile *file, char *filename, int pIopBuff )
{
    int ret;
    char *body = NULL;
    char fn[256];
    char devname[64];

    body = index(filename, ':');

	if (body)
	{
		int dlen;

		// copy device name
		dlen = body - filename;
		strncpy(devname, filename, dlen);
		devname[dlen] = 0;

		body += 1;

		if (!strcmp(devname, "cdrom0"))
		{ // CD/DVD
			int i;
			int len = strlen(body);
			const char *tail;

			file->isOnCD = 1;

			for (i = 0; i < len; i++)
			{
				if (body[i] == '/')
				{
					body[i] = '\\';
				}
				body[i] = toupper(body[i]);
			}

			tail = (index(filename, ';'))? "": ";1";
			sprintf(fn, "%s%s", body, tail);

		}
		else
		{			 // HD
			file->isOnCD = 0;
			sprintf(fn, "%s:%s", devname, body+1);  // Mick:  The +1 is to skip the initial \, so file names are the same off HD aas CD 
		}
	}
	else
	{				 // HD (default)
		body = filename;            
		strcpy(devname, "host0");
		file->isOnCD = 0;
		sprintf(fn, "%s:%s", devname, body);
	}

    printf("file: %s\n", fn);

    if (file->isOnCD)
	{
		sceCdRMode mode;
		if (!Config::CD())
		{
			if (!isStrFileInit)
			{
				sceCdInit(SCECdINIT);
				sceCdMmode(SCECdDVD);
				sceCdDiskReady(0);
		
				isStrFileInit = 1;
			}
		}	
		file->iopBuf = ( u_char * )pIopBuff; //( u_char * )sceSifAllocIopHeap((2048 * 80) + 16);
		sceCdStInit(80, 5, bound((u_int)file->iopBuf, 16));
	
		if(!sceCdSearchFile(&file->fp, fn)){
	
			printf("Cannot open '%s'(sceCdSearchFile)\n", fn);
			return 0;
		}
	
		file->size = file->fp.size;
		mode.trycount = 0;
		mode.spindlctrl = SCECdSpinStm;
		mode.datapattern = SCECdSecS2048;
		sceCdStStart(file->fp.lsn, &mode);
    }
	else
	{

		file->fd = sceOpen(fn, SCE_RDONLY);
		if (file->fd < 0)
		{
			printf("Cannot open '%s'(sceOpen)\n", fn);
			return 0;
		}
		file->size = sceLseek(file->fd, 0, SCE_SEEK_END);
		if (file->size < 0)
		{
			printf("sceLseek() fails (%s): %d\n", fn, file->size);
			sceClose(file->fd);
			return 0;
		}
	
		ret = sceLseek(file->fd, 0, SCE_SEEK_SET);
		if (ret < 0)
		{
			printf("sceLseek() fails (%s)\n", fn);
			sceClose(file->fd);
			return 0;
		}
    }

    return 1;
}

// ////////////////////////////////////////////////////////////////
//
//  Close a file
//
int strFileClose(StrFile *file)
{
    if (file->isOnCD) {
    	sceCdStStop();
//        sceSifFreeIopHeap((void *)file->iopBuf);
    } else {
	sceClose(file->fd);
    }
    return 1;
}

// ////////////////////////////////////////////////////////////////
//
//  Read data
//
int strFileRead(StrFile *file, void *buff, int size)
{
    int count;
    if (file->isOnCD) {
	u_int err;

        count= sceCdStRead(size >> 11, (u_int *)buff, STMBLK, &err);
	count <<= 11;

    } else {
	count = sceRead(file->fd, buff, size);
    }
    return count;
}

}
