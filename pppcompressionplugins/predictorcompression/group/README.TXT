File      :     README.TXT
Author    :     Robert Cripps . Copyright Symbian (c) 1999
Abstract  :     Overview of Predictor Compression Project

1) Introduction
This project contains all the files required to build a DLL for Predictor
Compression type 1 as defined in RFC 1978. The design is based on the MS
Compressor project.

2) Files

2.1)Predcomp.h
Contains class declarations for Compressor and Decompressor classes

2.2) Comp.cpp
Definition of the compressor class
Borrowed code from MS Compressor plus RFC Compression Algorithn

2.2) Decomp.cpp
Definition of the Decompressor class
Borrowed code from MS Compressor plus RFC Decompression Algorithn

3) Class Overview

3.1) CPredCompFactory : public CPppCompFactory
Required virtual, contains the methods for creating the Compressor and
Decompressor classes.

3.2) class CPredCompressor : public CPppCompressor
Compressor class contains main Compressor virtual plus support methods

3.3) class CPredDeCompressor : public CPppDeCompressor
Compressor class contains main DeCompressor virtual plus support methods

3.4) class TRFC1978Table
Main RFC 1978 class that contains the guess table and the associated
RFC compress and decompress functions.
