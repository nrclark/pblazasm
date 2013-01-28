/****************************************************************************
 **
 ** Copyright (C) 2005-2007 Sergey A. Sukiyazov. All rights reserved.
 **
 ** This file may be used under the terms of the GNU General Public
 ** License version 2.0 as published by the Free Software Foundation
 ** and appearing in the file LICENSE.GPL included in the packaging of
 ** this file.  Please review the following information to ensure GNU
 ** General Public Licensing requirements will be met:
 ** http://www.trolltech.com/products/qt/opensource.html
 **
 ** If you are unsure which license is appropriate for your use, please
 ** review the following information:
 ** http://www.trolltech.com/products/qt/licensing.html or contact the
 ** sales department at sales@trolltech.com.
 **
 ** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 ** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** AUTHOR: Sergey A. Sukiyazov <sergey.sukiyazov@gmail.com>
 **
 ****************************************************************************/
//! -*- coding: UTF-8 -*-
#define SOURCE_CODING "UTF-8"

#include <QtCore>
#include <QtXml>

bool readXmlFile(QIODevice &aDevice, QSettings::SettingsMap &aMap);
bool writeXmlFile(QIODevice &aDevice, const QSettings::SettingsMap &aMap);

//! function
static bool nodeToVariant(const QDomNode &aNode,QVariant &aValue) {
    bool vRetval = false;
    QString vType, vValue;

    aValue = QVariant();
    if(!vRetval && aNode.isCDATASection()) {
        vValue = aNode.toCDATASection().data();
        vRetval = true;
    }

    if(!vRetval && aNode.isText()) {
        vValue = aNode.toText().data();
        vRetval = true;
    }

    if(!vRetval) return vRetval;
    if(vValue.isEmpty()) return false; // ????

    const QDomNode vParent = aNode.parentNode();
    if(vParent.isElement()) {
        vType = vParent.toElement().attribute(QString::fromLatin1("type"));
    }

    if(vType == QString::fromLatin1("bytearray")) {
        aValue = QVariant(vValue.toLatin1());
    }
    else if(vType == QString::fromLatin1("variant")) {
        QByteArray vArray(vValue.toLatin1());
        QDataStream vStream(&vArray, QIODevice::ReadOnly);
        vStream >> aValue;
    }
    else if(vType == QString::fromLatin1("rect")) {
        QStringList vArgs = vValue.split(QLatin1Char(' '));
        if(vArgs.size() == 4) {
            aValue = QVariant(QRect(vArgs[0].toInt(), vArgs[1].toInt(), vArgs[2].toInt(), vArgs[3].toInt()));
        }
        else {
            vRetval = false;
        }
    }
    else if(vType == QString::fromLatin1("size")) {
        QStringList vArgs = vValue.split(QLatin1Char(' '));
        if(vArgs.size() == 2) {
            aValue = QVariant(QSize(vArgs[0].toInt(), vArgs[1].toInt()));
        }
        else {
            vRetval = false;
        }
    }
    else if(vType == QString::fromLatin1("point")) {
        QStringList vArgs = vValue.split(QLatin1Char(' '));
        if(vArgs.size() == 2) {
            aValue = QVariant(QPoint(vArgs[0].toInt(), vArgs[1].toInt()));
        }
        else {
            vRetval = false;
        }
    }
    else if(vType == QString::fromLatin1("integer")) {
        aValue = vValue.toInt(&vRetval);
    }
    else if(vType == QString::fromLatin1("bool")) {
        aValue = QVariant(vValue).toBool();
    }
    else if(vType == QString::fromLatin1("double")) {
        aValue =  vValue.toDouble(&vRetval);
    }
    else if(vType == QString::fromLatin1("invalid")) {
        vRetval = false;
    }
    else {
        aValue = vValue;
    }
    return vRetval;
}

//! function
static bool variantToNode(const QVariant &aValue, QDomElement &aElement) {
    QDomDocument vDocument = aElement.ownerDocument();
    QString vValue, vType;
    QDomNode vValueNode;

    switch (aValue.type()) {
        case QVariant::Invalid: {
            vType  = QString::fromLatin1("invalid");
            vValue = QString::null;
            break;
        }
        case QVariant::ByteArray: {
            QByteArray a = aValue.toByteArray();
            vType  = QString::fromLatin1("bytearray");
            vValue = QString::fromLatin1(a.constData(), a.size());
            break;
        }
        case QVariant::String: {
            vType  = QString::null;
            vValue = aValue.toString();
            break;
        }
        case QVariant::LongLong:
        case QVariant::ULongLong:
        case QVariant::Int:
        case QVariant::UInt: {
            vType  = QString::fromLatin1("integer");
            vValue = aValue.toString();
            break;
        }
        case QVariant::Bool: {
            vType  = QString::fromLatin1("bool");
            vValue = aValue.toString();
            break;
        }
        case QVariant::Double: {
            vType  = QString::fromLatin1("double");
            vValue = aValue.toString();
            break;
        }
        case QVariant::Rect: {
            QRect vRect = qvariant_cast<QRect>(aValue);
            vType   = QString::fromLatin1("rect");
            vValue  = QString::number(vRect.x());
            vValue += QLatin1Char(' ');
            vValue += QString::number(vRect.y());
            vValue += QLatin1Char(' ');
            vValue += QString::number(vRect.width());
            vValue += QLatin1Char(' ');
            vValue += QString::number(vRect.height());
            break;
        }
        case QVariant::Size: {
            QSize vSize = qvariant_cast<QSize>(aValue);
            vType   = QString::fromLatin1("size");
            vValue  = QString::number(vSize.width());
            vValue += QLatin1Char(' ');
            vValue += QString::number(vSize.height());
            break;
        }
        case QVariant::Point: {
            QPoint vPoint = qvariant_cast<QPoint>(aValue);
            vType   = QString::fromLatin1("point");
            vValue  = QString::number(vPoint.x());
            vValue += QLatin1Char(' ');
            vValue += QString::number(vPoint.y());
            break;
        }
        default: {
            QByteArray vArray;
            {
                QDataStream vStream(&vArray, QIODevice::WriteOnly);
                vStream << aValue;
            }

            vType   = QString::fromLatin1("variant");
            vValue += QString::fromLatin1(vArray.constData(), vArray.size());
            break;
        }
    }

    if(!vType.isEmpty()) {
        QDomAttr vTypeAttr = vDocument.createAttribute(QString::fromLatin1("type"));

        vTypeAttr.setValue(vType);
        aElement.setAttributeNode(vTypeAttr);
    }

    // проверяем возможность добаления текста
    bool vTextOk = false;

    if(!vValue.contains(QLatin1Char('>')) && !vValue.contains(QLatin1Char('<'))) {
        QDomDocument vDocumentTest;
        if(vDocumentTest.setContent(QString::fromLatin1("<T>%1</T>").arg(vValue))) {
            vTextOk = true;
        }
    }

    if(vTextOk) {
        vValueNode = vDocument.createTextNode(vValue);
    }
    else {
        vValueNode = vDocument.createCDATASection(vValue);
    }

    aElement.appendChild(vValueNode);
    return true;
}

//! function
static bool readXmlFileElement(const QString &aPath, const QDomElement &aElement, QSettings::SettingsMap &map) {
    bool vRetval = true;
    QDomNodeList vTmpList0 = aElement.childNodes();
    for(int i=0; i < vTmpList0.count(); i++) {
        QDomNode vTmpNode = vTmpList0.at(i);

        if((vTmpNode.isCDATASection() || vTmpNode.isText()) && vTmpNode.childNodes().count()==0) {
            QVariant vValue;

            if(nodeToVariant(vTmpNode,vValue)) {
                map.insert(aPath,vValue);
                return vRetval;
            }
        }

        if(vTmpNode.isElement()) {
            QDomElement vTmpElement = vTmpNode.toElement();
            QString vPath = vTmpElement.tagName();
            if(!aPath.isEmpty()) {
                vPath = QString::fromLatin1("%1/%2").arg(aPath).arg(vTmpElement.tagName());
            }
            vRetval = readXmlFileElement(vPath,vTmpElement,map);
        }
    }
    return vRetval;
}

//! function
bool readXmlFile(QIODevice &aDevice, QSettings::SettingsMap &aMap) {
    QString vErrorMsg;
    QDomDocument vDocument;
    int vErrorLine = 0, vErrorColumn = 0;

    if(!vDocument.setContent(&aDevice,&vErrorMsg,&vErrorLine,&vErrorColumn)) {
        qDebug() << QString::fromLatin1("Cant read config: %1 at line %2 column %3")
                        .arg(vErrorMsg).arg(vErrorLine).arg(vErrorColumn);
        return false;
    }

    QDomElement vRoot = vDocument.documentElement();
    if(vRoot.tagName() != QString::fromLatin1("config")) return false;

    return readXmlFileElement(QString::null,vRoot,aMap);
//    bool vRetval = readXmlFileElement(QString::null,vRoot,map);
//    return vRetval;
}

//! function
bool writeXmlFile(QIODevice &aDevice, const QSettings::SettingsMap &aMap) {
    QDomDocument vDocument;
//    qWarning("\n\nwriteXmlFile\n\n");
    QTextCodec *vpCodecForLocale = QTextCodec::codecForLocale();
    if(!vpCodecForLocale) {
        vpCodecForLocale = QTextCodec::codecForMib(106); // UTF-8
    }

    QString vInstructData = QString::fromLatin1("version=\"1.0\" encoding=\"%1\"").arg(QString::fromLatin1(vpCodecForLocale->name()));
    QDomProcessingInstruction vInstruct = vDocument.createProcessingInstruction(QString::fromLatin1("xml"),vInstructData);
    vDocument.appendChild(vInstruct);

    QDomElement vRoot = vDocument.createElement(QString::fromLatin1("config"));
    vDocument.appendChild(vRoot);

    QMap<QString,QVariant>::const_iterator vIter = aMap.begin();
    for (; vIter != aMap.end(); ++vIter) {
        QString vKey = vIter.key();
        QVariant vValue = vIter.value();

        // Ключ пустой
        if(vKey.isEmpty()) continue;

        QStringList vPath = vKey.split(QLatin1Char('/'));
        //QString vLastKey = vPath.takeLast();

        // Находим елемент соответсвующий пути ключа
        QStringListIterator i(vPath);
        QDomElement vElement = vRoot;
        while (i.hasNext()) {
            QString vTagName = i.next();
            QDomElement vTmp = vElement.firstChildElement(vTagName);

            if(vTmp.isNull()) {
                vTmp = vDocument.createElement(vTagName);
                vElement.appendChild(vTmp);
            }

            vElement = vTmp;
            vElement.removeAttribute(QString::fromLatin1("type"));

            QDomNodeList vTmpList = vElement.childNodes();
            for(int i=0; i < vTmpList.count(); i++) {
                if(vTmpList.item(i).nodeType() != QDomNode::ElementNode) {
                    vElement.removeChild(vTmpList.item(i));
                }
            }
        }


        vElement.removeAttribute(QString::fromLatin1("type"));

        QDomNodeList vTmpList = vElement.childNodes();
        for(int i=0; i < vTmpList.count(); i++) {
            if(vTmpList.item(i).nodeType() != QDomNode::ElementNode) {
                vElement.removeChild(vTmpList.item(i));
            }
        }
        variantToNode(vValue, vElement);
    }

    QByteArray vRawXml = vpCodecForLocale->fromUnicode(vDocument.toString(4));
    if(aDevice.write(vRawXml)==vRawXml.size()) {
         return true;
    }
    return false;
}
