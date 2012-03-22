#ifndef BSPARSE_H_INCLUDED
#define BSPARSE_H_INCLUDED

bool parse_file ( const char * strBitfile, bool bSpartan6, bool bVerbose ) ;
bool merge_code ( uint16_t * code, int len, int nr ) ;
bool write_file ( const char * strBitfile ) ;


#endif // BSPARSE_H_INCLUDED
