/*
www.sourceforge.net/projects/tinyxml
Original file by Yves Berquin.

This software is provided 'as-is', without any express or implied 
warranty. In no event will the authors be held liable for any 
damages arising from the use of this software.

Permission is granted to anyone to use this software for any 
purpose, including commercial applications, and to alter it and 
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must 
not claim that you wrote the original software. If you use this 
software in a product, an acknowledgment in the product documentation 
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source 
distribution.
*/

#include "cssysdef.h"
#include "tinyxml.h"

#ifndef TIXML_USE_STL


#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "tinystr.h"

// TiXmlString constructor, based on a C string
TiXmlString::TiXmlString (const char* instring)
{
    unsigned newlen;
    char * newstring;

    if (!instring)
    {
        allocated = 0;
        cstring = NULL;
	clength = 0;
        return;
    }
    newlen = strlen (instring) + 1;
    newstring = new char [newlen];
    strcpy (newstring, instring);
    allocated = newlen;
    cstring = newstring;
    clength = newlen - 1; // length of string not including null terminator.
}

// TiXmlString copy constructor
TiXmlString::TiXmlString (const TiXmlString& copy)
{
    unsigned newlen;
    char * newstring;

    if (! copy . allocated)
    {
        allocated = 0;
        cstring = NULL;
	clength = 0;
        return;
    }
    newlen = copy . length() + 1;
    newstring = new char [newlen];
    strcpy (newstring, copy . cstring);
    allocated = newlen;
    cstring = newstring;
    clength = newlen - 1; // length of string not including null terminator.
}

// TiXmlString = operator. Safe when assign own content
void TiXmlString ::operator = (const char * content)
{
    unsigned newlen;
    char * newstring;

    if (! content)
    {
        empty_it ();
        return;
    }
    newlen = strlen (content) + 1;
    newstring = new char [newlen];
    strcpy (newstring, content);
    empty_it ();
    allocated = newlen;
    cstring = newstring;
    clength = newlen - 1; // length of string not including null terminator.
}

// = operator. Safe when assign own content
void TiXmlString ::operator = (const TiXmlString & copy)
{
    unsigned newlen;
    char * newstring;

    if (! copy . length ())
    {
        empty_it ();
        return;
    }
    newlen = copy . length () + 1;
    newstring = new char [newlen];
    strcpy (newstring, copy . c_str ());
    empty_it ();
    allocated = newlen;
    cstring = newstring;
    clength = newlen - 1; // length of string not including null terminator.
}


//// Checks if a TiXmlString contains only whitespace (same rules as isspace)
//bool TiXmlString::isblank () const
//{
//    char * lookup;
//    for (lookup = cstring; * lookup; lookup++)
//        if (! isspace (* lookup))
//            return false;
//    return true;
//}

// append a const char * to an existing TiXmlString
void TiXmlString::append( const char* str, int len )
{
    char * new_string;
    unsigned new_alloc, new_size;

    new_size = length () + len + 1;
    // check if we need to expand
    if (new_size > allocated)
    {
        // compute new size
        new_alloc = assign_new_size (new_size);

        // allocate new buffer
        new_string = new char [new_alloc];        

        // copy the previous allocated buffer into this one
        if (allocated && cstring)
            strcpy (new_string, cstring);

        // append the suffix. It does exist, otherwize we wouldn't be expanding 
	strncpy (new_string + length(), str, len);
	new_string [new_size - 1] = 0;

        // return previsously allocated buffer if any
        if (allocated && cstring)
            delete [] cstring;

        // update member variables
        cstring = new_string;
	clength = new_size - 1;
        allocated = new_alloc;
    }
    else
    {
        // we know we can safely append the new string
	strncpy (cstring + length(), str, len);
	clength += len;
	cstring [clength] = 0;
     }
}


// Check for TiXmlString equuivalence
//bool TiXmlString::operator == (const TiXmlString & compare) const
//{
//    return (! strcmp (c_str (), compare . c_str ()));
//}

unsigned TiXmlString::find (char tofind, unsigned offset) const
{
    char * lookup;

    if (offset >= length ())
        return (unsigned) notfound;
    for (lookup = cstring + offset; * lookup; lookup++)
        if (* lookup == tofind)
            return lookup - cstring;
    return (unsigned) notfound;
}


bool TiXmlString::operator == (const TiXmlString & compare) const
{
	if ( &compare == this )
		return true;
	if ( length() == 0 && compare.length() == 0 )
		return true;
	if ( allocated && compare.allocated )
	{
		assert( cstring );
		assert( compare.cstring );
		return ( strcmp( cstring, compare.cstring ) == 0 );
 	}
	return false;
}


bool TiXmlString::operator < (const TiXmlString & compare) const
{
	if ( &compare == this )
		return false;
	if ( allocated && compare.allocated )
	{
		assert( cstring );
		assert( compare.cstring );
		return ( strcmp( cstring, compare.cstring ) > 0 );
 	}
	if ( length() == 0 && compare.length() != 0 )
		return true;
	return false;
}


bool TiXmlString::operator > (const TiXmlString & compare) const
{
	if ( &compare == this )
		return false;
	if ( allocated && compare.allocated )
	{
		assert( cstring );
		assert( compare.cstring );
		return ( strcmp( cstring, compare.cstring ) < 0 );
 	}
	if ( length() != 0 )
		return true;
	return false;
}


#endif	// TIXML_USE_STL
