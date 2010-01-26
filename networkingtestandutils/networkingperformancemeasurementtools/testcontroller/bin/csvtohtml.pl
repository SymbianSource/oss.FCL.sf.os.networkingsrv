# Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
# All rights reserved.
# This component and the accompanying materials are made available
# under the terms of "Eclipse Public License v1.0"
# which accompanies this distribution, and is available
# at the URL "http://www.eclipse.org/legal/epl-v10.html".
#
# Initial Contributors:
# Nokia Corporation - initial contribution.
#
# Contributors:
#
# Description:
# Given a .csv file, outputs a displayable table for the build results page.
# 
#

#!perl

print qq{<html>
 <HEAD>
    <TITLE>Results</TITLE>
  </HEAD>

<body>
<table width="100%" border ="1" align="center">
};

while(<>)
	{
	print qq{<tr align="right">\n\t<td>};

	chomp;

	if($.==1)
		{
		 # first line only.. clean up headings
		s{_}{<br>}g if($.==1);
		s{usec}{\&#181;s}g;
		s{percent}{\%}g;
		}

	s{,}{</td>\n\t<td>}g;
	s{<td></td>}{<td>&nbsp;</td>}g; # missing cells are ugly
	print;

	print "</td>\n</tr>\n";
	}
	
print "</table>
</body></html>
";
