#include "mainwindow.h"
#include "ui_mainwindow.h"

//class Highlighter : public QSyntaxHighlighter { Q_OBJECT
//public:
//    Highlighter(QTextDocument *parent = 0);

//protected:
//    void highlightBlock(const QString &text);

//private:
//    struct HighlightingRule {
//        QRegExp pattern;
//        QTextCharFormat format;
//    } ;
//    QVector<HighlightingRule> highlightingRules;

//    QRegExp commentStartExpression;
//    QRegExp commentEndExpression;

//    QTextCharFormat keywordFormat;
//    QTextCharFormat classFormat;
//    QTextCharFormat singleLineCommentFormat;
//    QTextCharFormat multiLineCommentFormat;
//    QTextCharFormat quotationFormat;
//    QTextCharFormat functionFormat;
//};

//Highlighter::Highlighter( QTextDocument *parent ) : QSyntaxHighlighter(parent) {
//    HighlightingRule rule;

//    keywordFormat.setForeground(Qt::darkBlue);
//    keywordFormat.setFontWeight(QFont::Bold);
//    QStringList keywordPatterns;
//    keywordPatterns << "\\bchar\\b" << "\\bclass\\b" << "\\bconst\\b"
//                    << "\\bdouble\\b" << "\\benum\\b" << "\\bexplicit\\b"
//                    << "\\bfriend\\b" << "\\binline\\b" << "\\bint\\b"
//                    << "\\blong\\b" << "\\bnamespace\\b" << "\\boperator\\b"
//                    << "\\bprivate\\b" << "\\bprotected\\b" << "\\bpublic\\b"
//                    << "\\bshort\\b" << "\\bsignals\\b" << "\\bsigned\\b"
//                    << "\\bslots\\b" << "\\bstatic\\b" << "\\bstruct\\b"
//                    << "\\btemplate\\b" << "\\btypedef\\b" << "\\btypename\\b"
//                    << "\\bunion\\b" << "\\bunsigned\\b" << "\\bvirtual\\b"
//                    << "\\bvoid\\b" << "\\bvolatile\\b";
//    foreach (const QString &pattern, keywordPatterns) {
//        rule.pattern = QRegExp(pattern);
//        rule.format = keywordFormat;
//        highlightingRules.append(rule);
//    }

//    classFormat.setFontWeight(QFont::Bold);
//    classFormat.setForeground(Qt::darkMagenta);
//    rule.pattern = QRegExp("\\bQ[A-Za-z]+\\b");
//    rule.format = classFormat;
//    highlightingRules.append(rule);

//    quotationFormat.setForeground(Qt::darkGreen);
//    rule.pattern = QRegExp("\".*\"");
//    rule.format = quotationFormat;
//    highlightingRules.append(rule);

//    functionFormat.setFontItalic(true);
//    functionFormat.setForeground(Qt::blue);
//    rule.pattern = QRegExp("\\b[A-Za-z0-9_]+(?=\\()");
//    rule.format = functionFormat;
//    highlightingRules.append(rule);

//    singleLineCommentFormat.setForeground(Qt::red);
//    rule.pattern = QRegExp("//[^\n]*");
//    rule.format = singleLineCommentFormat;
//    highlightingRules.append(rule);

//    multiLineCommentFormat.setForeground(Qt::red);

//    commentStartExpression = QRegExp("/\\*");
//    commentEndExpression = QRegExp("\\*/");
//}

//void Highlighter::highlightBlock(const QString &text) {
//     foreach (const HighlightingRule &rule, highlightingRules) {
//         QRegExp expression(rule.pattern);
//         int index = expression.indexIn(text);
//         while (index >= 0) {
//             int length = expression.matchedLength();
//             setFormat(index, length, rule.format);
//             index = expression.indexIn(text, index + length);
//         }
//     }
//    setCurrentBlockState(0);

//    int startIndex = 0;
//    if (previousBlockState() != 1)
//        startIndex = commentStartExpression.indexIn(text);

//    while (startIndex >= 0) {
//        int endIndex = commentEndExpression.indexIn(text, startIndex);
//        int commentLength;
//        if (endIndex == -1) {
//            setCurrentBlockState(1);
//            commentLength = text.length() - startIndex;
//        } else {
//            commentLength = endIndex - startIndex + commentEndExpression.matchedLength();
//        }
//        setFormat(startIndex, commentLength, multiLineCommentFormat);
//        startIndex = commentStartExpression.indexIn(text, startIndex + commentLength);
//    }
//}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    QIcon icon ;
    icon.addFile( ":/files/worker.ico" ) ;
    this->setWindowTitle( "pBlazBLD V0.1 (" + QString(QT_VERSION_STR) + ") - http://www.mediatronix.com" ) ;
    this->setWindowIcon( icon ) ;
    qApp->setApplicationName("pBlazBLD V0.1 (" + QString(QT_VERSION_STR) + ")");
    qApp->setWindowIcon( icon ) ;

    // main splitter layout
    QSplitter * hSplitter = new QSplitter( this ) ;
    setCentralWidget( hSplitter ) ;

    lbMode = new QLabel( tr( "insert" ) ) ;
    ui->statusBar->addWidget( lbMode, 50 ) ;
    lbModified = new QLabel( tr("N") ) ;
    ui->statusBar->addWidget( lbModified, 50 ) ;
    lbPosition = new QLabel( tr("00:00") ) ;
    ui->statusBar->addWidget( lbPosition, 50 ) ;

    popup = new QMenu() ;
    popup->addAction( ui->actionAdd_source_file ) ;
    popup->addAction( ui->actionRemove_source_file ) ;
    popup->addSeparator();
    popup->addAction( ui->actionCancel ) ;

    // create MRU list
    for ( int i = 0; i < MAXRECENTFILES; ++i ) {
        recentFileActs[i] = new QAction(this);
        recentFileActs[i]->setVisible(false);
        connect(recentFileActs[i], SIGNAL(triggered()),
                this, SLOT(openRecentFile()));
    }

    // add to the file menu
    separatorAct = ui->menuFile->addSeparator();
    for ( int i = 0; i < MAXRECENTFILES; ++i )
        ui->menuFile->addAction( recentFileActs[i] ) ;

    // create a project manager
    projectHandler = new QmtxProjectHandler( this ) ;

    hSplitter->addWidget( projectHandler->getVariantEditor() ) ;
    QSplitter * vSplitter = new QSplitter( this ) ;
    vSplitter->setOrientation( Qt::Vertical ) ;
    hSplitter->addWidget( vSplitter ) ;

    // our source editor
    textEdit = new QsciScintilla( vSplitter ) ;

    // and its lexer for Picoblaze Assembler source
    lexer = new QsciLexerPsm() ;
    textEdit->setLexer( lexer ) ;
    lexer->setFont( projectHandler->getFont() ) ;

    // log
    logBox = new QPlainTextEdit( vSplitter ) ;
    logBox->setReadOnly( true );
    logBox->setFont( projectHandler->getFont() ) ;
    connect( logBox, SIGNAL(cursorPositionChanged()), this, SLOT(highlightLogBox())) ;
//    highlighter = new Highlighter( logBox->document() ) ;


    // editor settings
    textEdit->setMarginWidth( 0, QString("00000") ) ;
    textEdit->setMarginType( 0, QsciScintilla::NumberMargin ) ;
    textEdit->setMarginWidth( 1, QString("0") ) ;
    textEdit->setMarginType( 1, QsciScintilla::SymbolMargin ) ;
    textEdit->setMarginWidth( 2, QString("0") ) ;
    textEdit->setMarginType( 2, QsciScintilla::SymbolMargin ) ;
    textEdit->setMarginWidth( 3, QString("0") ) ;
    textEdit->setMarginType( 3, QsciScintilla::SymbolMargin ) ;

    textEdit->markerDefine( 'M', Qt::ControlModifier ) ;
    textEdit->setMarginMarkerMask( 1, 0xFFFFFFFF ) ;
    textEdit->setMarginMarkerMask( 2, 0xFFFFFFFF ) ;
    textEdit->setMarginMarkerMask( 3, 0xFFFFFFFF ) ;

    // our file object
    currentFile = new QFile() ;

    // standard
    connect( ui->actionAbout_Qt, SIGNAL( triggered() ), qApp, SLOT( aboutQt() ) ) ;

    // cut and paste support
    ui->actionCut->setEnabled( false ) ;
    ui->actionCopy->setEnabled( false ) ;
    ui->actionDelete->setEnabled( false ) ;
    connect( ui->actionCut, SIGNAL( triggered() ), textEdit, SLOT( cut() ) ) ;
    connect( ui->actionCopy, SIGNAL( triggered() ), textEdit, SLOT( copy() ) ) ;
    connect( ui->actionDelete, SIGNAL (triggered() ), textEdit, SLOT( delete_selection() ) ) ;
    connect( textEdit, SIGNAL( copyAvailable(bool) ), ui->actionCut, SLOT( setEnabled(bool) ) ) ;
    connect( textEdit, SIGNAL( copyAvailable(bool) ), ui->actionCopy, SLOT( setEnabled(bool) ) ) ;
    connect( textEdit, SIGNAL( copyAvailable(bool) ), ui->actionDelete, SLOT( setEnabled(bool) ) ) ;

    connect( ui->actionPaste, SIGNAL( triggered() ), textEdit, SLOT( paste() ) ) ;

    // undo and redo support
    ui->actionUndo->setEnabled( false ) ;
    ui->actionRedo->setEnabled( false ) ;
    connect( ui->actionUndo, SIGNAL( triggered() ), textEdit, SLOT( undo() ) ) ;
    connect( ui->actionRedo, SIGNAL( triggered() ), textEdit, SLOT( redo() ) ) ;

    // markers
    connect( textEdit, SIGNAL( marginClicked(int, int, Qt::KeyboardModifiers) ),
        this, SLOT( onMarginClicked(int, int, Qt::KeyboardModifiers) ) ) ;

    // editor status
    connect( textEdit, SIGNAL( cursorPositionChanged(int, int) ), this, SLOT( onCursorpositionchanged(int, int) ) ) ;
    connect( textEdit, SIGNAL( modificationChanged(bool) ), this, SLOT( onModificationchanged(bool) ) ) ;
    connect( textEdit, SIGNAL( textChanged() ), this, SLOT( onTextchanged() ) ) ;

    ui->actionSave->setEnabled( false ) ;
    connect( textEdit, SIGNAL( modificationChanged(bool) ), ui->actionSave, SLOT( setEnabled(bool) ) ) ;

    // exit
    connect( ui->actionExit, SIGNAL( triggered() ), this, SLOT( close() ) ) ;

    // builders
    ui->actionAssemble->setEnabled( QFile::exists( "./pBlazASM.exe" ) ) ;
    ui->actionMerge->setEnabled( QFile::exists( "./pBlazMRG.exe" ) ) ;
    ui->actionBitfile->setEnabled( QFile::exists( "./pBlazBIT.exe" ) ) ;

    hSplitter->setStretchFactor( 0, 20 ) ;
    hSplitter->setStretchFactor( 1, 80 ) ;
    vSplitter->setStretchFactor( 0, 80 ) ;
    vSplitter->setStretchFactor( 1, 20 ) ;

    readSettings() ;
}

MainWindow::~MainWindow() {
    delete ui ;
}

void MainWindow::highlightLogBox() {
    QTextEdit::ExtraSelection highlight ;
    highlight.cursor = logBox->textCursor() ;
    highlight.format.setProperty(QTextFormat::FullWidthSelection, true ) ;
    highlight.format.setBackground( Qt::magenta );
    QList<QTextEdit::ExtraSelection> extras ;
    extras << highlight ;
    logBox->setExtraSelections( extras ) ;

    QTextCursor cursor = logBox->textCursor() ;
    cursor.select( QTextCursor::LineUnderCursor ) ;
    QString text = cursor.selectedText() ;
//    qDebug() << text ;

    QRegExp regexp( QString("^([a-z]:[^:]+):([0-9]+):"), Qt::CaseInsensitive, QRegExp::RegExp ) ;
    regexp.indexIn( text ) ;
    QStringList list ;
    list << regexp.capturedTexts() ;
//    qDebug() << list ;

    if ( list[ 0 ].isEmpty() )
         return ;

    loadFile( list[ 1 ] ) ;
    textEdit->setCursorPosition( list[ 2 ].toInt(), 1 ) ;
    textEdit->setFocus();
}


void MainWindow::readSettings() {
    QSettings settings( "Mediatronix", "pBlazBLD" ) ;
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();

    updateRecentFileActions() ;

    resize( size ) ;
    move( pos ) ;
}

void MainWindow::writeSettings() {
    QSettings settings( "Mediatronix", "pBlazBLD" ) ;
    settings.setValue( "pos", pos() ) ;
    settings.setValue( "size", size() ) ;
}

void MainWindow::closeEvent( QCloseEvent *event ) {
    if ( projectHandler->maybeSave() && maybeSaveFile() ) {
        writeSettings() ;
        event->accept() ;
    } else
        event->ignore() ;
 }

void MainWindow::on_actionAbout_triggered() {
    QMessageBox::about( this, "pBlazBLD", windowTitle() +
        "\n\nThis program comes with ABSOLUTELY NO WARRANTY." +
        "\nThis is free software, and you are welcome to redistribute it" +
        "\nunder certain conditions. See <http://www.gnu.org/licenses/>"
    ) ;
}

void MainWindow::on_actionNew_triggered() {
    if ( maybeSaveFile() ) {
        textEdit->clear() ;
        textEdit->setModified( false ) ;
        setCurrentFile( "" ) ;
    }
}
void MainWindow::onCursorpositionchanged(int line, int index) {
    lbPosition->setText( QString( "%1 : %2" ).arg( line + 1 ).arg( index + 1 ) ) ;
}

void MainWindow::onTextchanged() {
    ui->actionUndo->setEnabled(textEdit->isUndoAvailable());
    ui->actionRedo->setEnabled(textEdit->isRedoAvailable());
}

void MainWindow::onMarginClicked( int margin, int line, Qt::KeyboardModifiers state ) {
    Q_UNUSED(line ) ;
    textEdit->markerAdd( margin, state ) ;
}

void MainWindow::onModificationchanged( bool m ) {
    lbModified->setText( m ? "Y" : "N" ) ;
}

void MainWindow::openRecentFile() {
    QAction * action = qobject_cast<QAction *>(sender() ) ;
    if ( action )
        loadFile(action->data().toString());
}

void MainWindow::loadFile( const QString filename ) {
    if ( filename.isNull() )
        return ;
    if ( filename == currentFile->fileName() )
        return ;
    if ( maybeSaveFile() ) {
        setCurrentFile( filename ) ;

        if ( ! currentFile->open(QIODevice::ReadOnly | QIODevice::Text) ) {
            QMessageBox mb ;
            mb.setStandardButtons( QMessageBox::Ok ) ;
            mb.setInformativeText( filename );
            mb.setText( "Could not open file:" ) ;
            mb.exec() ;
            return ;
        }
        textEdit->read( currentFile ) ;
        textEdit->setModified( false ) ;
        currentFile->close() ;
    }
}

void MainWindow::on_actionOpen_triggered() {
    QString filename = QFileDialog::getOpenFileName(
        this, tr("Open Source File"), ".", tr( "Picoblaze source files (*.psm *.psh)" ) ) ;
    if ( ! filename.isEmpty() )
        loadFile( filename ) ;
}

void MainWindow::on_actionSave_triggered() {
    save() ;
}

bool MainWindow::save() {
    if ( currentFile->fileName().isEmpty() )
        return saveas() ;
    else
        return saveFile( currentFile->fileName() ) ;
}

void MainWindow::on_actionSaveAs_triggered() {
    saveas() ;
}

bool MainWindow::saveas() {
    QString fileName = QFileDialog::getSaveFileName( this ) ;
    if ( fileName.isEmpty() )
        return false ;
    return saveFile( fileName ) ;
}

bool MainWindow::saveFile(const QString fileName) {
    QFile file( fileName ) ;

    if ( !file.open( QFile::WriteOnly ) ) {
        QMessageBox::warning(this, tr("Application"),
            tr("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString() ) ) ;
        return false ;
    }

    textEdit->write( &file ) ;
    textEdit->setModified( false ) ;
    currentFile->setFileName( fileName ) ;
    statusBar()->showMessage (tr("File saved"), 2000 ) ;
    return true ;
}

bool MainWindow::maybeSaveFile() {
    if ( textEdit->isModified() ) {
        int ret = QMessageBox::warning(this, tr("pBlazBLD"),
             "The document has been modified.\nDo you want to save your changes?",
             QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel
        ) ;
        if ( ret == QMessageBox::Yes )
            return save() ;
        else if ( ret == QMessageBox::Cancel )
            return false ;
    }
    return true ;
}

void MainWindow::on_actionAssemble_triggered() {
    QProcess pBlazASM( this ) ;
    pBlazASM.setProcessChannelMode( QProcess::MergedChannels ) ;

    QString program = "./pBlazASM.exe " ;
    QStringList arguments =  projectHandler->asmArguments() ;
//    qDebug() << program << arguments ;

    pBlazASM.start( program, arguments ) ;
    if ( ! pBlazASM.waitForStarted( 1000 ) )
             return ;
    if ( ! pBlazASM.waitForFinished( 2000 ) )
        logBox->appendPlainText( "pBlazASM failed: " + pBlazASM.errorString() ) ;
    else
        logBox->appendPlainText( "pBlazASM output:" + pBlazASM.readAll() ) ;
}

void MainWindow::setCurrentFile(const QString &filename) {
    currentFile->setFileName ( filename ) ;
    setWindowFilePath( filename ) ;

    QSettings settings( "Mediatronix", "pBlazBLD" ) ;

    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll( filename ) ;
    files.prepend( filename );
    while (files.size() > MAXRECENTFILES)
        files.removeLast();

    settings.setValue("recentFileList", files);
    updateRecentFileActions();
}

void MainWindow::updateRecentFileActions() {
    QSettings settings( "Mediatronix", "pBlazBLD" ) ;

    QStringList files = settings.value("recentFileList").toStringList();

    int numRecentFiles = qMin(files.size(), (int)MAXRECENTFILES);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = QString("&%1 %2").arg(i + 1).arg(strippedName(files[i]) ) ;
        recentFileActs[i]->setText( text ) ;
        recentFileActs[i]->setData( files[i] ) ;
        recentFileActs[i]->setVisible( true ) ;
    }
    for (int j = numRecentFiles; j < MAXRECENTFILES; ++j)
        recentFileActs[j]->setVisible(false);

    separatorAct->setVisible( numRecentFiles > 0 ) ;
}

QString MainWindow::strippedName(const QString &fullFileName) {
    return QFileInfo(fullFileName).fileName() ;
}

void MainWindow::on_actionAdd_source_file_triggered() {
    projectHandler->addSourceFile( currentFile->fileName() ) ;
}

void MainWindow::on_actionRemove_source_file_triggered() {
    projectHandler->removeSourceFile( currentFile->fileName() ) ;
}

void MainWindow::on_actionNew_Project_triggered() {
    projectHandler->New() ;
}

void MainWindow::on_actionOpen_Project_triggered() {
    projectHandler->Load() ;
}

void MainWindow::on_actionSave_Project_triggered() {
    projectHandler->Save() ;
}


void MainWindow::on_actionClose_triggered() {
    if ( maybeSaveFile() )
        textEdit->clear();
}
