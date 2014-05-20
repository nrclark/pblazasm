#include "qmtxhexinputdialog.h"
#include "ui_qmtxhexinputdialog.h"

QmtxHexInputDialog::QmtxHexInputDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QmtxHexInputDialog)
{
    ui->setupUi(this);
}

QmtxHexInputDialog::~QmtxHexInputDialog()
{
    delete ui;
}

void QmtxHexInputDialog::setLabelText( QString s ) {
    ui->label->setText( s ) ;
}

void QmtxHexInputDialog::setValue( quint32 value ) {
    ui->spinBox->setValue( value ) ;
}

quint32 QmtxHexInputDialog::value( void ) {
    return ui->spinBox->value() ;
}
