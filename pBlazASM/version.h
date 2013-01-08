#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "08";
	static const char MONTH[] = "01";
	static const char YEAR[] = "2013";
	static const char UBUNTU_VERSION_STYLE[] = "13.01";
	
	//Software Status
	static const char STATUS[] = "Beta";
	static const char STATUS_SHORT[] = "b";
	
	//Standard Version Type
	static const long MAJOR = 2;
	static const long MINOR = 7;
	static const long BUILD = 2;
	static const long REVISION = 8;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT = 175;
	#define RC_FILEVERSION 2,7,2,8
	#define RC_FILEVERSION_STRING "2, 7, 2, 8\0"
	static const char FULLVERSION_STRING[] = "2.7.2.8";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY = 11;
	

#endif //VERSION_H
