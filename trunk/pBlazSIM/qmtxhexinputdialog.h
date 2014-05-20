#ifndef QMTXHEXINPUTDIALOG_H
#define QMTXHEXINPUTDIALOG_H

#include <QDialog>

namespace Ui {
    class QmtxHexInputDialog;
}

class QmtxHexInputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QmtxHexInputDialog(QWidget *parent = 0);
    ~QmtxHexInputDialog();

    void setLabelText( QString s ) ;
    void setValue( quint32 value ) ;
    quint32 value( void ) ;

private:
    Ui::QmtxHexInputDialog *ui;
};

#endif // QMTXHEXINPUTDIALOG_H
