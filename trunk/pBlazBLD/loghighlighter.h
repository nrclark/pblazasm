#ifndef LOGHIGHLIGHTER_H
#define LOGHIGHLIGHTER_H

#include <QSyntaxHighlighter>

class LogHighlighter : public QSyntaxHighlighter { Q_OBJECT
public:
    LogHighlighter( QTextDocument *parent = 0 ) ;

protected:
    void highlightBlock(const QString &text) ;

private:
    struct HighlightingRule {
        QRegExp pattern ;
        QTextCharFormat format ;
    } ;
    QVector<HighlightingRule> highlightingRules ;

    QTextCharFormat keywordFormat ;
    QTextCharFormat errorFormat ;
    QTextCharFormat commentFormat ;
    QTextCharFormat pathFormat ;
} ;

#endif // LOGHIGHLIGHTER_H
