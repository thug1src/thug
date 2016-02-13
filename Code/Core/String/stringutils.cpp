/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:																**
**																			**
**	Module:			String			 										**
**																			**
**	File name:		stringutils.cpp											**
**																			**
**	Created by:		05/31/01	-	Mick									**
**																			**
**	Description:	useful string (char *) utilities						**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <sys/config/config.h>
/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Str
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

#define	Caseless(letter) (((letter)>='A' && (letter)<='Z')?((letter)+'a'-'A'):letter)			 

// Returns location of Needle in the Haystack			 
const char * StrStr (const char *pHay, const char *pNeedle)
{
	register char First = *pNeedle++;		// get first letter in needle
	if (!First) return 0;					// if end of needle, then return NULL
	First = Caseless(First);

	while (1)
	{
		while (1)							// scan Haystack one letter at a time 
		{
			char FirstHay = *pHay++;		// get first letter of haystack
			FirstHay = Caseless(FirstHay);
			if (FirstHay == First) break;	// first letter matches, so drop into other code
			if (!FirstHay) return 0;		// if end of haystack, then return NULL
		}

		const char *pH = pHay;					// second letter of hay
		const char *pN = pNeedle;					// second letter of needle
		while (1)
		{
			char n = *pN++;					// get a needle letter
			if (!n) return pHay-1;			// end of needle, so return pointer to start of string in haystack
			char h = *pH++;					// get a hay letter
			if (Caseless(n) != Caseless(h)) break;				// difference, so stop looping (could be end of haystack, which is valid)
		}
	}
}

void LowerCase(char *p)
{
	while (*p)
	{
		char letter = *p;
		if (letter>='A' && letter<='Z')
			letter+='a'-'A';
		*p++ = letter;
	}
}

void UpperCase(char *p)
{
	while (*p)
	{
		char letter = *p;
		if (letter>='a' && letter<='z')
			letter-='a'-'A';
		*p++ = letter;
	}
}


int WhiteSpace(char *p)
{
	if (*p == 0x20 || *p== 0x09)
		return 1;
	else
		return 0;
}

char *PrintThousands(int n, char c)
{
	static	char buffer[32];		// enough for any int
	char	normal[32];				// printf in here without commas	
	char *p = buffer;

	if (n<0)
	{
		*p++ = '-';
		n = -n;
	}
	sprintf (normal, "%d", n);
	int digits = strlen(normal);
	char *p_digit = normal;
	
	// Since other countries deliminate their successive kilos differently
	switch (Config::GetLanguage())
	{
	case Config::LANGUAGE_FRENCH:
		c=' ';
		break;
	case Config::LANGUAGE_SPANISH:
		c='.';
		break;
	case Config::LANGUAGE_GERMAN:
		c='.';
		break;
	case Config::LANGUAGE_ITALIAN:
		c='.';
		break;
	default:
		c=',';
		break;
	}

	while (digits)
	{
		*p++ = *p_digit++;
		digits--;
		if ( digits && ( digits % 3) == 0)
		{
			*p++ = c;
		}		
	}
	*p++ = 0;
//	printf ("%s\n",buffer);
	return buffer;	
}


// helper function for SText::Draw(), SFont::QueryString()
uint DehexifyDigit(const char *pLetter)
{
	uint digit = 0;
	if (*pLetter >= '0' && *pLetter <= '9')
		digit = *pLetter - '0';
	else if (*pLetter >= 'a' && *pLetter <= 'v')
		digit = *pLetter - 'a' + 10;
	else if (*pLetter >= 'A' && *pLetter <= 'V')
		digit = *pLetter - 'A' + 10;
	else
	{
		Dbg_MsgAssert(0, ("Non Hex digit"));
	}
	return digit;
}

// Returns
// -1 if a<b
//  0 -f a==b
//  1 if a>b
//
//
// if      
int Compare(const char *p_a, const char *p_b)
{
	for (;;)
	{
		char a = *p_a++;
		char b = *p_b++;
		a = Caseless(a);
		b = Caseless(b);
		if (a < b)
		{
			return -1;
		}
		if (a > b)
		{
			return 1;
		}
		if (!a && !b)
		{
			// strings are identical
			return 0;
		}
		if (!a)
		{
			// a is shorter, so a < b
			return -1;
		}
		if (!b)
		{
			// b is shorter, so b < a
			return 1;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Str




