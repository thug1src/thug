/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Core library											**
**																			**
**	Module:			String      			 								**
**																			**
**	File name:		Core\String\CString.h									**
**																			**
**	Created by:		9/20/2000 - rjm							                **
**																			**
**	Description:	A handy, safe class to represent a string				**
**																			**
*****************************************************************************/

#ifndef __CORE_STRING_STRING_H
#define __CORE_STRING_STRING_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_SUPPORT_CLASS_H
    #include <core/support/class.h>
#endif
        
/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Str
{

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

/****************************************************************************
 *																			
 * Class:			String
 *
 * Description:		A handy, safe class for representing a string.
 *
 *                  Usage:  String x = "try our new diet of pinecones";
 *                          String y = x;                          
 *                          printf("%s\n", y.getString());
 *
 ****************************************************************************/

class String : public Spt::Class		// fix bug delete[] !!!
{

public:
				String();
				String(const char *string);
				String(const String &string);
	~String();

    String& 	operator= (const char * string);
    String& 	operator= (const String & string);
    bool 		operator== (const char * string);
    bool 		operator== (const String & string);
	bool		operator!();

    const char*	getString() const;
	void		copy(const char *pChar, int first_char, int last_char);
    
	int			m_dumbNum;
	static int 	sDumbCount;

private:
    char*		mp_string;
    int			m_length;


    static const int s_max_size;
};
    
/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace String

#endif	// __CORE_STRING_STRING_H


