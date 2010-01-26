// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// Contains the implementation punycode conversion algorithm
//



/**
 @file
*/

#include "punycodeconverter.h"
#include <e32std.h>

// *******************************************************
// This implementation is adapted from the Punycode sample
// implementation in appendix C of the RFC-3492.
// *******************************************************
//
// Bootstring parameters for Punycode
//
#define PUNYCODE_BASE 36
#define PUNYCODE_TMIN 1
#define PUNYCODE_TMAX 26
#define PUNYCODE_SKEW 38
#define PUNYCODE_DAMP 700
#define PUNYCODE_BIAS 72
#define PUNYCODE_INIT 0x80
#define PUNYCODE_DELI 0x2D

_LIT8(KAcePrefix, "xn--");


/* basic(cp) tests whether cp is a basic code point: */
#define basic(cp) ((TUint)(cp) < 0x80)

/* delim(cp) tests whether cp is a delimiter: */
#define delim(cp) ((cp) == PUNYCODE_DELI)


/**
Function to decode each digit and return the character.
	decode_digit(cp) returns the numeric value of a basic code 
	point (for use in representing integers) in the range 0 to 
	base-1, or base if cp is does not represent a value.       
@return - returns the non ASCII character for the input
@param cp the codepoint
*/

static TUint decode_digit(TUint cp)
	{
	return  cp - 48 < 10 ? cp - 22 :
			cp - 65 < 26 ? cp - 65 :
			cp - 97 < 26 ? cp - 97 :
			PUNYCODE_BASE;
	}


/**
Function to encode each digit and return the character.
	encode_digit(d) returns the basic code point whose value      
	(when used for representing integers) is d, which needs to be in   
	the range 0 to base-1.                                           
@return - returns the ASCII character for the input
@param cp the codepoint
*/
static char encode_digit(TUint d)
	{
	return (d + 22 + 75 * (d < 26));
	  /*  0..25 map to ASCII a..z */
	  /* 26..35 map to ASCII 0..9 */
	}



/**
Function to adapt the bias.
Bias adaptation function
@return - returns the ASCII character for the input
@param delta, difference delta
@param numPoints, the number of points
@param firsttime , whether the first bias
*/
static TUint adapt(TUint delta, TUint numpoints, int firsttime)
	{
  	TUint k;

	delta = firsttime ? delta / PUNYCODE_DAMP : delta >> 1;
	delta += delta / numpoints;

	for (k = 0;  delta > ((PUNYCODE_BASE - PUNYCODE_TMIN) * PUNYCODE_TMAX) / 2;  k += PUNYCODE_BASE)
		{
		delta /= PUNYCODE_BASE - PUNYCODE_TMIN;
		}
	return k + (PUNYCODE_BASE - PUNYCODE_TMIN + 1) * delta / (delta + PUNYCODE_SKEW);
	}


/**
Function to convert the IDN to Punycode
@return KErrNone, if conversion is successful
	KErrDndNameTooBig, if the IDN conversion exceeds the limit for a domain Name
	or any other system wide errors
@param aName, the input name in UCS2.0 encoding
*/
EXPORT_C TInt TPunyCodeDndName::IdnToPunycode(const THostName &aName)
	{
	SetLength(0);
			
	for (TInt i = 0; i < aName.Length(); )
		{
		i = Encode(aName, i);
		if (i < 0)
			return i;
		}
	return KErrNone;
	}

/**
Function to encode each label
@return KErrNone, if conversion is successful
	KErrDndNameTooBig, if the IDN conversion exceeds the limit for a domain Name
	or any other system wide errors
@param aName, the input name in UCS2.0 encoding
*/
TInt TPunyCodeDndName::Encode(const THostName &aName, TInt aIndex)
	{
	const TInt output_start = Length();

	TInt j;
	TUint n = PUNYCODE_INIT;
	TUint delta = 0;
	TUint bias = PUNYCODE_BIAS;
	TInt first_time = 1;

	// Copy the basic code points as is, and
	// compute the length of the current label
	// into input_length
	TUint input_length = 0;
	for (j = aIndex;  j < aName.Length();  ++j)
		{
		const TUint c = aName[j];
		if (c == '.')
			break;
	    if (basic(c))
	    	{
	    	if (Length() == MaxLength())
				return KErrDndNameTooBig;
	    	Append(c);
			}
		input_length += 1;
		}

	// h is the number of code points that have been handled
	TUint h = Length() - output_start;

	if (h == input_length)
		// Only basic code points, all done.
		goto done;

	// IDN is required, add prefix!
	if (Length() + KAcePrefix().Length() > MaxLength())
   		return KErrDndNameTooBig;
	Insert(output_start, KAcePrefix);

	if (h > 0)
		{
		// Both basic and non-basic points, need to add a delimiter.
		if (Length() == MaxLength())
			return KErrDndNameTooBig;
		Append(PUNYCODE_DELI);
		}
	
	// Main encoding loop

	while (h < input_length)
		{
		// All non-basic code points < n have been
		// handled already.  Find the next larger one:
		TUint m = KMaxTUint;
	    for (j = aIndex;  j < aIndex + input_length;  ++j)
	    	{
	    	const TUint c = aName[j];
			if (c >= n && c < m)
				m = c;
	    	}

		// Increase delta enough to advance the decoder's
		// <n,i> state to <m,0>, but guard against overflow:
		if (m - n > (KMaxTUint - delta) / (h + 1))
			return KErrOverflow;
		delta += (m - n) * (h + 1);
		n = m;
		
		for (j = aIndex;  j < aIndex + input_length;  ++j)
			{
			const TUint c = aName[j];
			if (c < n)
				{
				if (++delta == 0)
					return KErrOverflow;
				}
			else if (c == n)
				{
				// Represent delta as a generalized variable-length integer:
				TUint q = delta;
		        for (TUint k = PUNYCODE_BASE;  ;  k += PUNYCODE_BASE)
		        	{
		        	if (Length() >= MaxLength())
						return KErrDndNameTooBig;

					const TUint t = k <= bias /* + tmin */ ? PUNYCODE_TMIN :     /* +tmin not needed */
              			k >= bias + PUNYCODE_TMAX ? PUNYCODE_TMAX : k - bias;
              		if (q < t)
              			break;
              		Append(encode_digit(t + (q - t) % (PUNYCODE_BASE - t)));
					q = (q - t) / (PUNYCODE_BASE - t);
		        	}
		        Append(encode_digit(q));
				++h;
        		bias = adapt(delta, h, first_time);
        		delta = 0;
        		first_time = 0;
				}
			}
	    ++delta, ++n;
		}
done:
	aIndex += input_length;
	if (aIndex < aName.Length())
		{
		// Input terminated with '.', copy it to ouput.
		if (Length() == MaxLength())
			return KErrDndNameTooBig;
		Append('.');
		aIndex += 1;
		}
	return aIndex;
	}
	

/**
Function to decode the punycode to IDN
@return KErrNone, if conversion is successful
	KErrDndBadName, if the punycode provided cannot be decoded
	or any other system wide errors
@param aName, the input punycode name in ASCII format
@param aStart, where to start the conversion, defaulted to 0.
*/
EXPORT_C TInt TPunyCodeDndName::PunycodeToIdn(TDes& aBuf, const TInt aStart)
	{
	aBuf.SetLength(0);
	for (TInt i = aStart; i < Length();  )
		{
		i = Decode(i, aBuf);
		if (i < 0)
			{
			// If Punycode fails for any reason, just return
			// the raw name (it probably was not punycode).
			return KErrDndBadName;
			}
		}
	return KErrNone;
	}

/**
Function to decode each label
@return KErrNone, if conversion is successful
	KErrDndBadName, if the punycode provided cannot be decoded
	or any other system wide errors
@param aBuf, the input punycode name in ASCII format for each label
@param aIndex, where to start the conversion, defaulted to 0.
*/
TInt TPunyCodeDndName::Decode(TInt aIndex, TDes &aBuf) const
	{
	if (aIndex + KAcePrefix().Length() > Length() ||
			Mid(aIndex, KAcePrefix().Length()).Compare(KAcePrefix()) != 0)
		{
		// cannot be punycode.
		// copy label as is, while updating aIndex
		while (aIndex < Length())
			{
			const TUint c = (*this)[aIndex++];
			if (aBuf.Length() == aBuf.MaxLength())
				return KErrDndNameTooBig;
			aBuf.Append(c);
			if (c == '.')
				break;
			}
		return aIndex;
		}
		
	aIndex += KAcePrefix().Length();	// Skip KAcePrefix.


	// Handle the basic code points.
	TInt puny_end = aIndex;
	TInt inp = aIndex;
	for ( ; puny_end < Length();  ++puny_end)
		{
		const TUint c = (*this)[puny_end];
		if (c == '.')
			break;
		if (delim(c))
			inp = puny_end;
		}

	if (aBuf.Length() + inp - aIndex > aBuf.MaxLength())
		return KErrDndNameTooBig;

	const TUint out_base = aBuf.Length();
	// Copy the basic code points as is.
	for (TInt j = aIndex;  j < inp;  ++j)
		{
		const TUint c = (*this)[j];
		if (!basic(c))
			return KErrGeneral;
		aBuf.Append(c);
		}
	// Skip inp over delimiter, if present
	if (inp > aIndex)
		inp += 1;

	// Initialize the state:

	TUint n = PUNYCODE_INIT;
	TUint outp = aBuf.Length() - out_base;
	TUint i = 0;
	TUint bias = PUNYCODE_BIAS;

	// Main decoding loop:  Start just after the last delimiter if any 
	// basic code points were copied; start at the beginning otherwise.

	while (inp < puny_end)
		{
		// inp is the index of the next character to be consumed, and
		// outp is the number of code points processed (includes the
		// initial basic points).

		// Decode a generalized variable-length integer into delta,
		// which gets added to i.  The overflow checking is easier
		// if we increase i as we go, then subtract off its starting
		// value at the end to obtain delta.
		const TUint oldi = i;
		TUint w = 1;
		for (TUint k = PUNYCODE_BASE;  ; k += PUNYCODE_BASE)
			{
			if (inp >= puny_end)
				return KErrGeneral;
			
			const TUint digit = decode_digit((*this)[inp++]);
			if (digit >= PUNYCODE_BASE)
				return KErrGeneral;
			if (digit > (KMaxTUint - i) / w)
				return KErrOverflow;
			i += digit * w;
			const TUint t = k <= bias /* + tmin */ ? PUNYCODE_TMIN :     /* +tmin not needed */
				k >= bias + PUNYCODE_TMAX ? PUNYCODE_TMAX : k - bias;
			if (digit < t)
				break;
			if (w > KMaxTUint / (PUNYCODE_BASE - t))
				return KErrOverflow;
			w *= (PUNYCODE_BASE - t);
			}

		outp++;		// Going to add new code, increment count.
		bias = adapt(i - oldi, outp, oldi == 0);
		
		// i was supposed to wrap around from out+1 to 0,
		// incrementing n each time, so we'll fix that now:

	    if (i / outp > KMaxTUint - n)
			return KErrOverflow;
	    n += i / outp;
	    i %= outp;

		// Insert n at position i of the output:

		if (aBuf.Length() == aBuf.MaxLength())
			return KErrDndNameTooBig;
		TBuf<1> tmp;
		tmp.Append(n);
		aBuf.Insert(out_base + i, tmp);
		i++;
		}
	if (puny_end < Length())
		{
		// Input terminated with '.', copy it to ouput.
		if (aBuf.Length() == aBuf.MaxLength())
			return KErrDndNameTooBig;
		aBuf.Append('.');
		puny_end++;
		}
	return puny_end;
	}
