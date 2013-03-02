#ifndef psmHighlighter_H
#define psmHighlighter_H

#include <QSyntaxHighlighter>

class PsmHighlighter : public QSyntaxHighlighter { Q_OBJECT
public:
    PsmHighlighter( QTextDocument *parent = 0 ) ;

protected:
    void highlightBlock(const QString &text) ;

private:
    enum _STATE {
        stINIT, stWHITE, stIDENT, stREG, stSINGLE, stDOUBLE,
        stNUM, stHEX, stBIN, stOCT, stDEC, stCHAR,
        stSTRING, stCOMMENT1, stCOMMENT2, stSKIP
    } ;

    struct HighlightingRule {
        QRegExp pattern ;
        QTextCharFormat format ;
    } ;
    QVector<HighlightingRule> highlightingRules ;

    QTextCharFormat keywordFormat ;
    QStringList keywords ;

    QTextCharFormat directiveFormat ;
    QStringList directives ;

    QTextCharFormat registerFormat ;
    QTextCharFormat operatorFormat ;
    QTextCharFormat numberFormat ;
    QTextCharFormat stringFormat ;
    QTextCharFormat commentFormat ;
    QTextCharFormat errorFormat ;
} ;

#endif // psmHighlighter_H
