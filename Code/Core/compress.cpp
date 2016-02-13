#include <core/compress.h>

#define N		 4096	/* size of ring buffer */
#define F		   18	/* upper limit for match_length */
#define THRESHOLD	2   /* encode string into position and length
						   if match_length is greater than this */
#define NIL			N	/* index for root of binary search trees */

unsigned long int
textsize = 0,	/* text size counter */
codesize = 0,	/* code size counter */
printcount = 0;	/* counter for reporting progress every 1K bytes */


//unsigned char text_buf[N + F - 1];	 	//ring buffer of size N, with extra F-1 bytes to facilitate string comparison 
int     match_position, match_length;	// of longest match.  These are 	set by the InsertNode() procedure. 
//int		lson[N + 1], rson[N + 257], dad[N + 1];   // left & right children & parents -- These constitute binary search trees.


unsigned char *text_buf;
int *lson;
int *rson;
int *dad;


#define readc()		*pIn++
#define writec(x)	*pOut++ = x


void InitTree(void)	 /* initialize trees */
{
	int  i;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap()); 
	text_buf = new unsigned char[N + F - 1];
	lson = new int[N+1];
	rson = new int[N+257];
	dad  = new int[N+1];
	Mem::Manager::sHandle().PopContext(); //Mem::Manager::sHandle().TopDownHeap());	



	/* For i = 0 to N - 1, rson[i] and lson[i] will be the right and
	   left children of node i.  These nodes need not be initialized.
	   Also, dad[i] is the parent of node i.  These are initialized to
	   NIL (= N), which stands for 'not used.'
	   For i = 0 to 255, rson[N + i + 1] is the root of the tree
	   for strings that begin with character i.  These are initialized
	   to NIL.  Note there are 256 trees. */

	for ( i = N + 1; i <= N + 256; i++ ) rson[i] = NIL;
	for ( i = 0; i < N; i++ ) dad[i] = NIL;
}

void    DeInitTree(void)  /* free up the memory */
{
	delete [] text_buf;
	delete [] lson;
	delete [] rson;
	delete [] dad;

}

void InsertNode(int r)
/* Inserts string of length F, text_buf[r..r+F-1], into one of the
   trees (text_buf[r]'th tree) and returns the longest-match position
   and length via the global variables match_position and match_length.
   If match_length = F, then removes the old node in favor of the new
   one, because the old one will be deleted sooner.
   Note r plays double role, as tree node and position in buffer. */
{
	int  i, p, cmp;
	unsigned char  *key;

	cmp = 1;  key = &text_buf[r];  p = N + 1 + key[0];
	rson[r] = lson[r] = NIL;  match_length = 0;
	for ( ; ; )
	{
		if ( cmp >= 0 )
		{
			if ( rson[p] != NIL )
			{
				p = rson[p];
			}
			else
			{
				rson[p] = r;  dad[r] = p;  return;
			}
		}
		else
		{
			if ( lson[p] != NIL )
			{
				p = lson[p];
			}
			else
			{
				lson[p] = r;  dad[r] = p;  return;
			}
		}
		
		for ( i = 1; i < F; i++ )
		{
			if ( (cmp = key[i] - text_buf[p + i]) != 0 )  break;
		}
		
		if ( i > match_length )
		{
			match_position = p;
			if ( (match_length = i) >= F )
			{
				break;
			}
		}
	}
	dad[r] = dad[p];  lson[r] = lson[p];  rson[r] = rson[p];
	dad[lson[p]] = r;  dad[rson[p]] = r;
	if ( rson[dad[p]] == p )
	{
		rson[dad[p]] = r;
	}
	else
	{
		lson[dad[p]]	= r;
	}
	dad[p] = NIL;  /* remove p */
}

void DeleteNode(int p)	/* deletes node p from tree */
{
	int  q;

	if ( dad[p] == NIL ) return;  /* not in tree */
	if ( rson[p] == NIL ) q = lson[p];
	else if ( lson[p] == NIL ) q = rson[p];
	else
	{
		q = lson[p];
		if ( rson[q] != NIL )
		{
			do
			{
				q = rson[q];
			} while ( rson[q] != NIL );
			rson[dad[q]] = lson[q];  dad[lson[q]] = dad[q];
			lson[q] = lson[p];  dad[lson[p]] = q;
		}
		rson[q] = rson[p];  dad[rson[p]] = q;
	}
	dad[q] = dad[p];
	if ( rson[dad[p]] == p ) rson[dad[p]] = q;
	else lson[dad[p]] = q;
	dad[p] = NIL;
}

int Encode(char *pIn, char *pOut, int bytes_to_read, bool print_progress)
{
	int  i, c, len, r, s, last_match_length, code_buf_ptr;
	unsigned char  code_buf[17], mask;

	textsize = 0;	/* text size counter */
	codesize = 0;	/* code size counter */
	printcount = 0;	/* counter for reporting progress every 1K bytes */

	InitTree();	 /* initialize trees */
	code_buf[0] = 0;  /* code_buf[1..16] saves eight units of code, and
		code_buf[0] works as eight flags, "1" representing that the unit
		is an unencoded letter (1 byte), "0" a position-and-length pair
		(2 bytes).  Thus, eight units require at most 16 bytes of code. */
	code_buf_ptr = mask = 1;
	s = 0;  r = N - F;
	for ( i = s; i < r; i++ ) text_buf[i] = ' ';  /* Clear the buffer with
		  any character that will appear often. */
	for ( len = 0; len < F && bytes_to_read; len++ )
	{
		c = readc();
		bytes_to_read--;
		text_buf[r + len] = c;	/* Read F bytes into the last F bytes of
			the buffer */
	}
	if ( (textsize = len) == 0 )
	{
		DeInitTree();
		return 0;  /* text of size zero */
	}
	for ( i = 1; i <= F; i++ ) InsertNode(r - i);  /* Insert the F strings,
		  each of which begins with one or more 'space' characters.  Note
		  the order in which these strings are inserted.  This way,
		  degenerate trees will be less likely to occur. */
	InsertNode(r);	/* Finally, insert the whole string just read.  The
		global variables match_length and match_position are set. */
	do
	{
		if ( match_length > len ) match_length = len;  /* match_length
			  may be spuriously long near the end of text. */
		if ( match_length <= THRESHOLD )
		{
			match_length = 1;  /* Not long enough match.  Send one byte. */
			code_buf[0] |= mask;  /* 'send one byte' flag */
			code_buf[code_buf_ptr++] = text_buf[r];	 /* Send uncoded. */
		}
		else
		{
			code_buf[code_buf_ptr++] = (unsigned char) match_position;
			code_buf[code_buf_ptr++] = (unsigned char)
									   (((match_position >> 4) & 0xf0)
										| (match_length - (THRESHOLD + 1)));  /* Send position and
											  length pair. Note match_length > THRESHOLD. */
		}
		if ( (mask <<= 1) == 0 )
		{  /* Shift mask left one bit. */
			for ( i = 0; i < code_buf_ptr; i++ )  /* Send at most 8 units of */
				writec(code_buf[i]);	 /* code together */
			codesize += code_buf_ptr;
			code_buf[0] = 0;  code_buf_ptr = mask = 1;
		}
		last_match_length = match_length;
		for ( i = 0; i < last_match_length &&
			bytes_to_read; i++ )
		{
			c = readc();
			bytes_to_read--;
			DeleteNode(s);		/* Delete old strings and */
			text_buf[s] = c;	/* read new bytes */
			if ( s < F - 1 ) text_buf[s + N] = c;  /* If the position is
				  near the end of buffer, extend the buffer to make
				  string comparison easier. */
			s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
			/* Since this is a ring buffer, increment the position
			   modulo N. */
			InsertNode(r);	/* Register the string in text_buf[r..r+F-1] */
		}
		if ( (textsize += i) > printcount )
		{
//			printf("%12ld\r", textsize);  printcount += 1024;
			/* Reports progress each time the textsize exceeds
			   multiples of 1024. */
		}
		while ( i++ < last_match_length )
		{	/* After the end of text, */
			DeleteNode(s);					/* no need to read, but */
			s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
			if ( --len ) InsertNode(r);		  /* buffer may not be empty. */
		}
	} while ( len > 0 );  /* until length of string to be processed is zero */
	if ( code_buf_ptr > 1 )
	{	  /* Send remaining code. */
		for ( i = 0; i < code_buf_ptr; i++ ) writec(code_buf[i]);
		codesize += code_buf_ptr;
	}

	if ( print_progress )
	{
		printf("     In : %ld bytes\n", textsize);	/* Encoding is done. */
		printf("     Out: %ld bytes\n", codesize);
		printf("     Out/In: %.3f\n", (double)codesize / textsize);
	}

	DeInitTree();
	return codesize;
}


///////////////////////////////////////////////////////////////////////////////////////
//
// Decompression code, cut & pasted from pre.cpp
//
///////////////////////////////////////////////////////////////////////////////////////

#define RINGBUFFERSIZE		 4096	/* N size of ring buffer */
#define MATCHLIMIT		   18	/* F upper limit for match_length */
#define THRESHOLD	2   /* encode string into position and length */

//#define WriteOut(x) 	{Dbg_MsgAssert(pOut<pIn,("Oops! Decompression overlap!\nIncrease IN_PLACE_DECOMPRESSION_MARGIN in sys\\file\\pip.cpp")); *pOut++ = x;}
#define WriteOut(x) 	{*pOut++ = x;}

#define USE_BUFFER	1		 // we don't need no stinking buffer!!!!

#if USE_BUFFER
	#ifdef	__PLAT_NGPS__
// ring buffer is just over 4K 4096+17),
// so fits nicely in the PS2's 8K scratchpad
static unsigned char    sTextBuf[RINGBUFFERSIZE + MATCHLIMIT - 1];
// Note:  if we try to use the scratchpad, like this
// then the code actually runs slower
// if we want to optimize this, then it should
// be hand crafted in assembly, using 128bit registers
//	const unsigned char * sTextBuf = (unsigned char*) 0x70000000;
	#else
static unsigned char
sTextBuf[RINGBUFFERSIZE + MATCHLIMIT - 1];	/* ring buffer of size N,
	with extra F-1 bytes to facilitate string comparison */
	#endif
#endif


#define	ReadInto(x)		if (!Len) break; Len--; x = *pIn++
#define	ReadInto2(x)	Len--; x = *pIn++ 	  // version that knows Len is Ok


// Decode an LZSS encoded stream
// Runs at approx 12MB/s on PS2	 without scratchpad (which slows it down in C)
// a 32x CD would run at 4.8MB/sec, although we seem to get a lot less than this
// with our current file system, more like 600K per seconds.....
// Need to write a fast streaming file system....

// Ken: Made this return the new pOut so I can do some asserts to make sure it has
// written the expected number of bytes.
unsigned char *DecodeLZSS(unsigned char *pIn, unsigned char *pOut, int Len)	/* Just the reverse of Encode(). */
{
	int  i, j, k, r, c;
//	uint64	LongWord;
//	int bytes = 0;
//	unsigned char *pScratch;
	unsigned int  flags;



//	int basetime =  (int) Tmr::ElapsedTime(0);
//	int len = Len;

//	int	OutBytes = 4;
//	int	OutWord = 0;

#if USE_BUFFER
	for ( i = 0; i < RINGBUFFERSIZE - MATCHLIMIT; i++ )
		sTextBuf[i] = ' ';
	r = RINGBUFFERSIZE - MATCHLIMIT;
#else
	r = RINGBUFFERSIZE - MATCHLIMIT;
#endif
	flags = 0;
	for ( ; ; )
	{
		if ( ((flags >>= 1) & 256) == 0 )
		{
			ReadInto(c);
			flags = c | 0xff00;			/* uses higher byte cleverly */
		}										/* to count eight */
		if ( flags & 1 )
		{
			ReadInto(c);
			//			putc(c, outfile);
			WriteOut(c);
#if USE_BUFFER
			sTextBuf[r++] = c;
			r &= (RINGBUFFERSIZE - 1);
#else
			r++;
//			r &= (RINGBUFFERSIZE - 1);	  // don't need to wrap r until it is used
#endif
		}
		else
		{
			ReadInto(i);
			ReadInto2(j);			// note, don't need to check len on this one....

			i |= ((j & 0xf0) << 4);						// i is 12 bit offset

#if !USE_BUFFER
			j = (j & 0x0f) + THRESHOLD+1;				// j is 4 bit length (above the threshold)
			unsigned char *pStream;
			r &= (RINGBUFFERSIZE - 1);					// wrap r around before it is used
			pStream = pOut - r;							// get base of block
			if ( i>=r )										  // if offset > r, then
				pStream -= RINGBUFFERSIZE;				// it's the previous block
			pStream += i;									// add in the offset to the base
			r+=j;												// add size to r
			while ( j-- )									  // copy j bytes
				WriteOut(*pStream++);
#else

			j = (j & 0x0f) + THRESHOLD;				// j is 4 bit length (above the threshold)
			for ( k = 0; k <= j; k++ )					  // just copy the bytes
			{
				c =  sTextBuf[(i+k) & (RINGBUFFERSIZE - 1)];
				WriteOut(c);
				sTextBuf[r++] = c;
				r &= (RINGBUFFERSIZE - 1);
			}
#endif
		}
	}

//	int Time = (int) Tmr::ElapsedTime(basetime);
//	if (Time > 5)
//	{
//		printf("decomp time is %d ms, for %d bytes,  %d bytes/second\n", Time,len, len * 1000 /Time );
//	}
	return pOut;
}

