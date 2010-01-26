TLS Test 2 README
-----------------

This is TLS Test 2. All blame should be assigned to its author, Chris Davies.
TLS Test 2 is designed as a complete replacement for the frankly worthless
tests that we were handed with the original TLS Provider. If you are reading 
this, I really, really hope that these tests have been deleted. Actually, I
hope that TLS Provider has been deleted to. *sigh*

This README will tell you how to contruct new test cases for TLS test 2, since
that tended to be the bane of the original TLS tests. Big opaque data files,
and no one willing to own up to constructing them.

To construct new test cases, you need the OpenSSL command line utility. All
the files needed come directly from there.

INI File Options
----------------

MaxInterval - Integer, Maximum interval in seconds between now and when the timestamp in random data.
NumCipherSuites - The count of cipher suites we expect TLS Provider to claim to support
CipherSuite<n> - A cipher suite number, in hex. E.g. 0035 is cipher suite (0x0, 0x35). E.g. CipherSuite1=0035.
		 The <n> indicates expected order.
ServerRandomFile - A string pointing at a file containing 32 bytes of random data for use as the Server Random. This may be
		   Generated using openssl.exe rand 32.
ClientRandomFile - A string pointing at a file containing 32 bytes of random for use as the Client Random. Generated as above.
CipherHighByte - The high byte of the cipher suite to use in the test in decimal. Typically, zero.
CipherLowByte - The low byte of the cipher suite to use in the test in decimal.
ProtocolMajorVersion - The major version of the TLS/SSL protocol to use. Typically, 3. 
ProtocolMinorVersion - The minor version of the TLS/SSL protocol to use. Currently supported, 0 for SSLv3 or 1 for TLSv1.
ServerCert - A file name pointing at a DER encoded X.509 certificate for use as the server certificate.
ServerKey - A file name pointing at a DER encoded PKCS8 private key paired with the server certificate. Must be unencrypted.
DHParamFile - If the cipher suite uses DH key exchange, you must specify the file name of a DER encoded DH parameters file.
	      Generate using openssl.exe -gendh, then convert from PEM with openssl.exe dh -inform PEM -outform DER.
DomainName - The domain name of the "server" used in the test. Doesn't have to be a real domain name.
SequenceNumber - The sequence number of the fake records we feed to TLS Provider.
RecordType - The record type of the fake records we feed to TLS provider.
RecordSize - The size of the fake records we feed to TLS provider.
TamperedSequenceNumber - When simulating attacks, the tampered sequence number to use.
TamperedRecordType - When simulating attacks, the tampered record type to use.
TamperedDigest - Boolean, when simulating attacks, should we tamper with the record mac.
TamperHandshake - Boolean, when simulating attacks, should we tamper with the fake sequence of handshake messages.
ExpectedResult - Various steps allow you to give an expected result. An error number.

Test Steps
----------

Give or take, there is one test step per public method on the TLS Provider interface. The steps are:

GetRandomStep - This test step tests that the method used to generate the client random functions correctly.
	It takes the following parameters:
		+ MaxInterval

CipherSuitesStep - Tests that TLS provider supports all the cipher suites we think it should, and that they are
		   in the order of priority that we think they should be in.
	It takes the following parameters:
		+ NumCipherSuites
		+ CipherSuite<n>

VerifyServerCertStep - Tests that we correctly verify server certificates chaining and domain names.
	It takes the following parameters:
		+ ExpectedResult
		+ ServerCert
		+ DomainName

VerifySignature - Tests that we correctly verify signatures sent to us to prove certificate ownership.
	It takes the following parameters:
		+ ServerCert
		+ DomainName
		+ ServerKey 
		+ CipherHighByte
		+ CipherLowByte

ClientKeyExchangeStep - Tests that we correctly produce the key exchange message.
	It takes the following parameters:
		+ ServerCert
		+ DomainName
		+ ServerKey 
		+ CipherHighByte
		+ CipherLowByte
		+ DHParamFile 
		+ ServerRandomFile 
		+ ClientRandomFile
		+ ProtocolMajorVersion 
		+ ProtocolMinorVersion 

ClientCertificateStep - At the time of writing, this step was not yet complete.

ClientFinishedStep - Tests that we correctly produce the client finished message.
	It takes the following parameters:
		+ ServerCert
		+ DomainName
		+ ServerKey 
		+ CipherHighByte
		+ CipherLowByte
		+ DHParamFile 
		+ ServerRandomFile 
		+ ClientRandomFile
		+ ProtocolMajorVersion 
		+ ProtocolMinorVersion 

ServerFinishedStep - Tests that we correctly verify or reject the server finished message.
	It takes the following parameters:
		+ ServerCert
		+ DomainName
		+ ServerKey 
		+ CipherHighByte
		+ CipherLowByte
		+ DHParamFile 
		+ ServerRandomFile 
		+ ClientRandomFile
		+ ProtocolMajorVersion 
		+ ProtocolMinorVersion 
		+ TamperHandshake

EncryptStep - Tests that we correctly encrypt records.
	It takes the following parameters:
		+ ServerCert
		+ DomainName
		+ ServerKey 
		+ CipherHighByte
		+ CipherLowByte
		+ DHParamFile 
		+ ServerRandomFile 
		+ ClientRandomFile
		+ ProtocolMajorVersion 
		+ ProtocolMinorVersion
		+ RecordSize
		+ RecordType
		+ SequenceNumber

DecryptStep - Tests that we correctly decrypt and verify macs.
	It takes the following parameters:
		+ ServerCert
		+ DomainName
		+ ServerKey 
		+ CipherHighByte
		+ CipherLowByte
		+ DHParamFile 
		+ ServerRandomFile 
		+ ClientRandomFile
		+ ProtocolMajorVersion 
		+ ProtocolMinorVersion
		+ RecordSize
		+ RecordType
		+ SequenceNumber
		+ TamperedRecordType
		+ TamperedSequenceNumber

KeyDerivationStep - Tests that we correctly derive EAP-TLS keys.
		+ ServerCert
		+ DomainName
		+ ServerKey 
		+ CipherHighByte
		+ CipherLowByte
		+ DHParamFile 
		+ ServerRandomFile 
		+ ClientRandomFile
		+ ProtocolMajorVersion 
		+ ProtocolMinorVersion 	
		

!!!TODO!!!

- Standard test battery for export ciphers
- Security testing for ciphers, using the corruption tests in DecryptStep, ServerFinishedStep.
- TLS 1.2 pre-testing. New test padding class has been added for this.
- Security Testing for certs steps
- Port the tlstest2 token tests to the new test script
- Finish the client auth step, Alexey doing this I hope.
- Much later, test steps for session caching. (fake this now with "hardware" token?)
