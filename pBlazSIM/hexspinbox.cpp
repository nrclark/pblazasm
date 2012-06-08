
#include "hexspinbox.h"

HexSpinBox::HexSpinBox(QWidget *parent) : QSpinBox(parent) {
    setRange(0, 255);
    validator = new QRegExpValidator(QRegExp("[0-9A-Fa-f]{1,8}"), this);
}

void HexSpinBox::setText( const QString &text ) {
    bool ok ;
    setValue( text.toInt(&ok, 16) ) ;
}

QValidator::State HexSpinBox::validate(QString &text, int &pos) const {
    return validator->validate(text, pos);
}

QString HexSpinBox::textFromValue(int value) const {
    return QString("%L1").arg(value, 2, 16, QChar('0') ).toUpper();
}

int HexSpinBox::valueFromText(const QString &text) const {
    bool ok ;
    return text.toInt(&ok, 16);
}
