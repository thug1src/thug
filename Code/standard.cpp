/******************************************************************
 Standard.cpp - sample file illustrating and documenting the 
 Neversoft coding standards, such as they are.
 This file is initially offered up as a guideline, and is open to change
 It is intended to be short
*****************************************************************/

// Tabs set to four.  Tabs do not expand to spaces.
//  +	+	+	+	+	+	+	+	+	+	+	+
//  +...+...+...+...+...+...+...+...+...+...+...+
// (the above two lines should line up)

// Note, this (interface) section should be in a .H file, it is presented here for clarity

const 		int vMAGIC_NUMBER = 100;	 				// const values in UPPER_CASE, prepended by v (for value)
// OR const int	MAGIC_NUMBER = 100;	 					// do we need the v?  

#define	SKATER_GRAVITY	(2.0f * Script::GetFloat("skt_g"))	//	#define macro is UPPER_CASE 

enum EDialogBoxResult			   					// Enums start with 'E'
{
	DB_YES 	=			0x00000001,			 		// Do we need a v here? or is it clear?
	DB_NO   =			0x00000002,
};

// Note: type, member name and comments all line up in columns
 
class CTallTree										// Classes start with 'C', and are CamelBack
{													// '{' and '}' line up like this
public:												// public members come first, in CamelBack	  	
	int					GetSomething();				// public member function
	void				DoSomething(int numberOfItems, char *p_items = NULL); 		// function parameters
	static int			sGetNumberOfTrees();		// static member function
	int					mSomeNumber; 				// public member variable (not recommended)
	int	*				mpSomethingElse;			// public member pointer
	int	**				mppSomethingElse;			// public member pointer to pointer

private:												// private members come last, in lower_case_underscored
	int					get_another();				// private member function
	static int			s_get_xxx();				// private static function
	int					m_another_number;			// private member variable  	
	char *				mp_letters_to_use;			// private member pointer 
	char **				mpp_thing_to_use;			// private member pointer to pointer
	static int			s_number_of_trees;			// static member variable 
}

/////////////////////////////////////////////////////////////
// the following (implementation) section is the .CPP portion

int	GNumberOfCallsToSomeFunction;					// Global, 'G'Prefix, CamelBack, not recommended
													// usually for temporary debugging hacks   

int	CTallTree::s_number_of_trees;					// instance of static variable													  

////////////////////////////////////////////////////////////////////////	function seperator												  
// Returns the number of trees inexistance		    // function comment
int CTallTree::sGetNumberOfTrees()	 				// instance of static function
{
	return s_number_of_trees;
}

////////////////////////////////////////////////////////////////////////													  
void CTallTree::DoSomething(int numberOfItems, char *p_itemsInList); 		// instance of member function 
{
	int		temp_value;								// local variable in lower_case
	
	if (numberOfItems)
	{												// note lining up of { and }
		p_itemsInList++;			  				// (reccomended) single line statement in curly brackets
	}
	else											// 'else' on seperate line
	{
		printf ("%d\n",numberOfItems);
		printf ("%c\n",*p_itemsInList);
	}
	// TODO:  write more code						// comment telling me what TODO
}



///////////////////////////////////////////////////////////////////////
// Cross platform compiling compatibility rules

/////////////////////////////////////////////////////////////////////////////////////////////																		   
// #1 Do Not initialize non-integral const static data members inside the class declaration.

class CScreenElement : public Obj::CObject
{
//	static const float vJUST_LEFT = -1.0f;			// WRONG!!
	static const float vJUST_LEFT;					// Correct, value is defined below
}

const float CScreenElement::vJUST_LEFT	= -1.0f;

//////////////////////////////////////////////////////////////////////////////
// #2 Do not redefine default parameters in the implementation of a function

// e.g
void  DoSomething(int numberOfItems, char *p_items = NULL); 		// wrong! should only be in .h declaration
{
 // ...
}


//////////////////////////////////////////////////////////////////////////////
// #3 Alignment requires different code on different platforms
//The __attribute__ aligned ((aligned(n))) compiler directive is GNU specific.
//So the following class member (taken from sk\gamenet\gamenet.h) will cause
//VC7 to crap out:

char m_net_thread_stack[ vNET_THREAD_STACK_SIZE ] __attribute__
((aligned(16)));

//The equivalent alignment directive for Xbox would be as follows:

#pragma pack( 16 )
char m_net_thread_stack[ vNET_THREAD_STACK_SIZE ];
#pragma pack()

//So please conditionalise alignment directives for all platforms.

//////////////////////////////////////////////////////////////////////////////
// #4 - "far" is a reserved keyword in VC++ (harkening back 16 bit memory models)
//

// bool far;   //  <<<<<<<<<<< wrong, will crap in VC++

	bool	find_far_collision;		// correct, and more meaningful too!!
																							 
////////////////////////////////////////////////////////////////////////////
// #5 - every #if.... directive must have a matching #endif
// in GNU, you can just reach the end of a file, but VC++
// with throw an error

#ifdef __PLAT_NGPS__
... some code
... end of file     <<<<<<<<<<<<< wrong, needs #endif for VC++, but will compile fine in GNU																							 
																							 








                                              
