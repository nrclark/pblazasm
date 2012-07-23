#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "19";
	static const char MONTH[] = "07";
	static const char YEAR[] = "2012";
	static const char UBUNTU_VERSION_STYLE[] = "12.07";
	
	//Software Status
	static const char STATUS[] = "Beta";
	static const char STATUS_SHORT[] = "b";
	
	//Standard Version Type
	static const long MAJOR = 2;
	static const long MINOR = 4;
	static const long BUILD = 2;
	static const long REVISION = 0;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT = 7;
	#define RC_FILEVERSION 2,4,2,0
	#define RC_FILEVERSION_STRING "2, 4, 2, 0\0"
	static const char FULLVERSION_STRING[] = "2.4.2.0";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY = 1;
	

#endif //VERSION_H
