#include "psmhighlighter.h"

#include <QDebug>

PsmHighlighter::PsmHighlighter( QTextDocument * parent ) : QSyntaxHighlighter( parent ) {
    keywordFormat.setForeground( Qt::darkBlue ) ;
    keywordFormat.setFontWeight( QFont::Bold ) ;
    keywords
        << "MOVE" << "ADD" << "OR" << "XOR"
        << "ADDC" << "AND" << "SUB" << "SUBC"
        << "COMP" << "CMPC" << "TEST" << "TSTC"
        << "JUMP" << "CALL" << "RET" << "RETI"
        << "LD" << "ST" << "IN" << "OUT"
        << "SL0" << "SL1" << "SLA" << "SLX" << "RL"
        << "SR0" << "SR1" << "SRA" << "SRX" << "RR"
        << "DINT" << "EINT" << "BANK" << "CORE"
        << "C" << "NC" << "Z" << "NZ"
    ;
    keywords.sort();

    directiveFormat.setForeground( Qt::darkRed ) ;
    directiveFormat.setFontWeight( QFont::Bold ) ;
    directives
        << ".BYT" << ".TXT" << ".BUF" << ".EQU" << ".SET"
        << ".ORG" << ".SCR" << ".DSG" << ".ESG" << ".ALN"
        << ".WLE" << ".WBE" << ".LLE" << ".LBE"
        << ".IF"  << ".FI"
    ;
    directives.sort();

    registerFormat.setForeground( Qt::darkMagenta ) ;
    registerFormat.setFontWeight( QFont::Bold ) ;

    stringFormat.setForeground( Qt::darkYellow ) ;
    stringFormat.setFontItalic( true ) ;

    numberFormat.setForeground( Qt::darkGreen ) ;
    numberFormat.setFontWeight( QFont::Bold ) ;

    commentFormat.setForeground( Qt::darkGray ) ;
    commentFormat.setFontItalic( true ) ;

    operatorFormat.setForeground( Qt::blue ) ;

    errorFormat.setForeground( Qt::red ) ;
    errorFormat.setFontItalic( true ) ;
}

void PsmHighlighter::highlightBlock( const QString &text ) {
    if ( text.isEmpty() )
        return ;

    QByteArray b = text.toAscii().toLower() + '\n' ;
    enum _STATE state = stINIT ;

    for ( int s = 0, p = 0 ; p < b.size() ; ) {
        switch ( state ) {
        case stINIT :
            if ( b[ p ] == ' ' || iscntrl( b[ p ] ) ) {
                p++ ;
            } else if ( b[ p ] == ',' || b[ p ] == ':' || b[ p ] == '(' || b[ p ] == ')' ) {
                p++ ;
            } else if (
                b[ p ] == '*' || b[ p ] == '%' || b[ p ] == '+' || b[ p ] == '-' ||
                b[ p ] == '|' || b[ p ] == '&' || b[ p ] == '^' || b[ p ] == '~' ||
                b[ p ] == '%' || b[ p ] == '@'
            ) {
                s = p++ ;
                state = stSINGLE ;
            } else if ( b[ p ] == '<' || b[ p ] == '>' ) {
                // double char operators
                s = p++ ;
                state = stDOUBLE ;
            } else if ( b[ p ] == '/' ) {
                s = p++ ;
                state = stCOMMENT1 ;
            } else if ( b[ p ] == ';' ) {
                s = p++ ;
                state = stCOMMENT2 ;
            } else if ( b[ p ] == '"' ) {
                s = p++ ;
                state = stSTRING ;
            } else if ( b[ p ] == '\'' ) {
                s = p++ ;
                state = stCHAR ;
            } else if ( b[ p ] == 's' ) {
                s = p++ ;
                state = stREG ;
            } else if ( isalpha( b[ p ] ) || b[ p ] == '_' || b[ p ] == '.' ) {
                s = p++ ;
                state = stIDENT ;
            } else if ( b[ p ] == '0' ) {
                s = p++ ;
                state = stNUM ;
            } else if ( isdigit( b[ p ] ) ) {
                s = p++ ;
                state = stDEC ;
            } else {
                s = p++ ;
                state = stSKIP ;
            }
            break ;

        case stWHITE :
            if ( b[ p ] == ',' || b[ p ] == ':' ) {
                p++ ;
                state = stINIT ;
            } else if ( b[ p ] == ' ' || iscntrl( b[ p ] ) ) {
                p++ ;
                state = stINIT ;
            } else
                state = stSKIP ;
            break ;

        case stIDENT :
            if ( isalnum( b[ p ] ) )
                p++ ;
            else if ( b[ p ] == '_' )
                p++ ;
            else {
                QString t( b.mid( s, p - s ) ) ;
                if ( keywords.contains( t.toUpper(), Qt::CaseInsensitive ) )
                    setFormat( s, p - s, keywordFormat ) ;
                 else if ( directives.contains( t.toUpper(), Qt::CaseInsensitive ) )
                    setFormat( s, p - s, directiveFormat ) ;
                state = stWHITE ;
            }
            break ;

        case stREG :
            if ( isxdigit( b[ p ] ) ) {
                p++ ;
                if ( isalnum( b[ p ] ) || b[ p ] == '_' )
                    state = stIDENT ;
                else {
                    setFormat( s, p - s, registerFormat ) ;
                    state = stWHITE ;
                }
            } else
                state = stIDENT ;
            break ;

        case stSINGLE :
            setFormat( s, p - s, operatorFormat ) ;
            state = stWHITE ;
            break ;

        case stDOUBLE :
            if ( b[ p ] == b[ s ] ) {
                p++ ;
                setFormat( s, p - s, operatorFormat ) ;
                state = stWHITE ;
            } else
                state = stSKIP ;
            break ;

        case stNUM :
            if ( b[ p ] == 'x' ) {
                p++ ;
                state = stHEX ;
            } else if ( b[ p ] == 'o' ) {
                p++ ;
                state = stOCT ;
            } else if ( b[ p ] == 'b' ) {
                p++ ;
                state = stBIN ;
            } else
                state = stDEC ;
            break ;

        case stHEX :
            if ( isxdigit( b[ p ] ) || b[ p ] == '_'  )
                p++ ;
            else {
                setFormat( s, p - s, numberFormat ) ;
                state = stWHITE ;
            }
            break ;

        case stBIN :
            if ( b[ p ] == '0' || b[ p ] == '1' || b[ p ] == '_' )
                p++ ;
            else {
                setFormat( s, p - s, numberFormat ) ;
                state = stWHITE ;
            }
            break ;

        case stOCT :
            if ( ( b[ p ] >= '0' && b[ p ] <= '7' ) || b[ p ] == '_' )
                p++ ;
            else {
                setFormat( s, p - s, numberFormat ) ;
                state = stWHITE ;
            }
            break ;

        case stDEC :
            if ( isdigit( b[ p ] ) || b[ p ] == '_' )
                p++ ;
            else {
                setFormat( s, p - s, numberFormat ) ;
                state = stWHITE ;
            }
            break ;

        case stCHAR :
            if ( b[ p ] != '\'' )
                p++ ;
            else {
                setFormat( s, ++p - s, stringFormat ) ;
                state = stWHITE ;
            }
            break ;

        case stSTRING :
            if ( b[ p ] != '"' )
                p++ ;
            else {
                setFormat( s, ++p - s, stringFormat ) ;
                state = stWHITE ;
            }
            break ;

        case stCOMMENT1 :
            if ( b[ p ] == '/' )
                state = stCOMMENT2 ;
            else
                state = stWHITE ;
            break ;

        case stCOMMENT2 :
            while ( p < b.size() )
                p++ ;
            setFormat( s, p - s, commentFormat ) ;
            state = stINIT ;
            break ;

        case stSKIP :
            while ( p < b.size() ) {
                if ( b[ p ] == '/' || b[ p ] == ';' || b[ p ] == '"' )
                    break ;
                else
                    p++ ;
            }
            setFormat( s, p - s, errorFormat ) ;
            state = stINIT ;
            break ;

        default :
            p++ ;
        }
    }
}

