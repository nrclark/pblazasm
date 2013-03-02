#include "loghighlighter.h"


LogHighlighter::LogHighlighter( QTextDocument * parent ) : QSyntaxHighlighter(parent) {
    HighlightingRule rule ;

    keywordFormat.setForeground( Qt::darkBlue ) ;
    keywordFormat.setFontWeight( QFont::Bold ) ;
    QStringList keywordPatterns ;
    keywordPatterns << "\\bpBlazASM\\b" << "\\bpBlazMRG\\b" << "\\bpBlazBIT\\b" << "\\bPicoblaze\\b" ;
    foreach ( const QString &pattern, keywordPatterns ) {
        rule.pattern = QRegExp( pattern ) ;
        rule.format = keywordFormat ;
        highlightingRules.append( rule ) ;
    }

    pathFormat.setFontUnderline( true ) ;
    pathFormat.setForeground( Qt::blue ) ;
    rule.pattern = QRegExp("^([a-z]:[^:]+):", Qt::CaseInsensitive ) ;
    rule.format = pathFormat ;
    highlightingRules.append( rule ) ;

    commentFormat.setForeground( Qt::darkGreen ) ;
    rule.pattern = QRegExp("! [^\n]*") ;
    rule.format = commentFormat ;
    highlightingRules.append( rule ) ;

    errorFormat.setForeground( Qt::darkRed ) ;
    rule.pattern = QRegExp("? [^\n]*") ;
    rule.format = errorFormat ;
    highlightingRules.append( rule ) ;
}

void LogHighlighter::highlightBlock(const QString &text) {
     foreach (const HighlightingRule &rule, highlightingRules) {
         QRegExp expression( rule.pattern ) ;
         int index = expression.indexIn(text) ;
         while ( index >= 0 ) {
             int length = expression.matchedLength() ;
             setFormat(index, length, rule.format) ;
             index = expression.indexIn(text, index + length) ;
         }
     }
}

