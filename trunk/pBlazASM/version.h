#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "28";
	static const char MONTH[] = "12";
	static const char YEAR[] = "2012";
	static const char UBUNTU_VERSION_STYLE[] = "12.12";
	
	//Software Status
	static const char STATUS[] = "Beta";
	static const char STATUS_SHORT[] = "b";
	
	//Standard Version Type
	static const long MAJOR = 2;
	static const long MINOR = 6;
	static const long BUILD = 3;
	static const long REVISION = 13;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT = 128;
	#define RC_FILEVERSION 2,6,3,13
	#define RC_FILEVERSION_STRING "2, 6, 3, 13\0"
	static const char FULLVERSION_STRING[] = "2.6.3.13";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY = 7;
	

#endif //VERSION_H
