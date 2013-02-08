#include "mainwindow.h"
#include "ui_mainwindow.h"


Highlighter::Highlighter( QTextDocument * parent ) : QSyntaxHighlighter(parent) {
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

void Highlighter::highlightBlock(const QString &text) {
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

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this) ;

    QIcon icon ;
    icon.addFile( ":/files/worker.ico" ) ;
    this->setWindowTitle( "pBlazBLD V0.1 (" + QString(QT_VERSION_STR) + ") - http://www.mediatronix.com" ) ;
    this->setWindowIcon( icon ) ;
    qApp->setApplicationName("pBlazBLD V0.1 (" + QString(QT_VERSION_STR) + ")") ;
    qApp->setWindowIcon( icon ) ;

    // tab widget
    tabWidget = new QTabWidget() ;
    setCentralWidget( tabWidget ) ;
    tabWidget->setTabPosition( QTabWidget::West ) ;

    // splitter layout
    splitter = new QSplitter() ;
    splitter->setOrientation( Qt::Horizontal ) ;
    tabWidget->addTab( splitter, "Project" ) ;

    lbMode = new QLabel( tr( "insert" ) ) ;
    ui->statusBar->addWidget( lbMode, 50 ) ;
    lbModified = new QLabel( tr("N") ) ;
    ui->statusBar->addWidget( lbModified, 50 ) ;
    lbPosition = new QLabel( tr("00:00") ) ;
    ui->statusBar->addWidget( lbPosition, 50 ) ;

    popup = new QMenu() ;
    popup->addAction( ui->actionAdd_source_file ) ;
    popup->addAction( ui->actionRemove_source_file ) ;
    popup->addSeparator() ;
    popup->addAction( ui->actionCancel ) ;


    // create MRU project list
    for ( int i = 0 ; i < MAXRECENTFILES ; ++i ) {
        recentProjectActs[ i ] = new QAction(this) ;
        recentProjectActs[ i ]->setVisible(false) ;
        connect(recentProjectActs[ i ], SIGNAL(triggered()),
                this, SLOT(openRecentProject())) ;
    }

    // create MRU file list
    for ( int i = 0 ; i < MAXRECENTFILES ; ++i ) {
        recentFileActs[ i ] = new QAction(this) ;
        recentFileActs[ i ]->setVisible(false) ;
        connect(recentFileActs[ i ], SIGNAL(triggered()),
                this, SLOT(openRecentFile())) ;
    }

    // add to the file menu
    separatorProjectAct = ui->menuFile->addSeparator() ;
    for ( int i = 0 ; i < MAXRECENTFILES ; ++i )
        ui->menuFile->addAction( recentProjectActs[ i ] ) ;

    // add to the file menu
    separatorFileAct = ui->menuFile->addSeparator() ;
    for ( int i = 0 ; i < MAXRECENTFILES ; ++i )
        ui->menuFile->addAction( recentFileActs[ i ] ) ;


    // create a project manager
    projectHandler = new QmtxProjectHandler( this ) ;
    splitter->addWidget( projectHandler->getVariantEditor() ) ;

    // log
    logBox = new QPlainTextEdit() ;
    splitter->addWidget( logBox ) ;
    logBox->setReadOnly( true ) ;
    logBox->setFont( projectHandler->getFont() ) ;
    connect( logBox, SIGNAL(cursorPositionChanged()), this, SLOT(highlightLogBox())) ;
    highlighter = new Highlighter( logBox->document() ) ;


    // our source editor
    textEdit = new QsciScintilla( this ) ;
    tabWidget->addTab( textEdit, "Source" ) ;

    // and its lexer for Picoblaze Assembler source
    lexer = new QsciLexerPsm() ;
    textEdit->setLexer( lexer ) ;
    lexer->setFont( projectHandler->getFont() ) ;

    // editor settings
    textEdit->setMarginWidth( 0, QString("00000") ) ;
    textEdit->setMarginType( 0, QsciScintilla::NumberMargin ) ;
    textEdit->setMarginWidth( 1, QString("0") ) ;
    textEdit->setMarginType( 1, QsciScintilla::SymbolMargin ) ;
    textEdit->setMarginWidth( 2, QString("0") ) ;
    textEdit->setMarginType( 2, QsciScintilla::SymbolMargin ) ;
    textEdit->setMarginWidth( 3, QString("0") ) ;
    textEdit->setMarginType( 3, QsciScintilla::SymbolMargin ) ;

    textEdit->markerDefine( 'M', 2 ) ;
    textEdit->setMarginMarkerMask( 0, 1 ) ;
    textEdit->setMarginMarkerMask( 1, 2 ) ;
    textEdit->setMarginMarkerMask( 2, 4 ) ;

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

    splitter->setStretchFactor( 0, 40 ) ;
    splitter->setStretchFactor( 1, 60 ) ;

    readSettings() ;
}

MainWindow::~MainWindow() {
    delete ui ;
}

void MainWindow::highlightLogBox() {
    QTextEdit::ExtraSelection highlight ;
    highlight.cursor = logBox->textCursor() ;
    highlight.format.setProperty(QTextFormat::FullWidthSelection, true ) ;
    highlight.format.setBackground( Qt::magenta ) ;
    QList<QTextEdit::ExtraSelection> extras ;
    extras << highlight ;
    logBox->setExtraSelections( extras ) ;

    QTextCursor cursor = logBox->textCursor() ;
    cursor.select( QTextCursor::LineUnderCursor ) ;
    QString text = cursor.selectedText() ;

    QRegExp regexp( QString("^([a-z]:[^:]+):([0-9]+):"), Qt::CaseInsensitive, QRegExp::RegExp ) ;
    regexp.indexIn( text ) ;
    QStringList list ;
    list << regexp.capturedTexts() ;

    if ( list[ 0 ].isEmpty() )
         return ;

    loadFile( list[ 1 ] ) ;
    tabWidget->setCurrentWidget( textEdit ) ;
    int line = list[ 2 ].toInt() - 1 ;
    textEdit->setCursorPosition( line, 0 ) ;
    textEdit->markerDeleteAll( 2 ) ;
    textEdit->markerAdd( line, 2 ) ;
    textEdit->setFocus() ;
}


void MainWindow::readSettings() {
    QSettings settings( "Mediatronix", "pBlazBLD" ) ;
//    setWindowState( (Qt::WindowStates)settings.value( "state", (int)Qt::WindowNoState ) ) ;
    QPoint pos = settings.value("pos", QPoint(100, 100)).toPoint() ;
    QSize size = settings.value("size", QSize(800, 600)).toSize() ;

    updateRecentlyUsedActions() ;

    resize( size ) ;
    move( pos ) ;
}

void MainWindow::writeSettings() {
    QSettings settings( "Mediatronix", "pBlazBLD" ) ;
    settings.setValue( "state", (int)windowState() ) ;
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
    ui->actionUndo->setEnabled(textEdit->isUndoAvailable()) ;
    ui->actionRedo->setEnabled(textEdit->isRedoAvailable()) ;
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
        loadFile(action->data().toString()) ;
}

void MainWindow::openRecentProject() {
    QAction * action = qobject_cast<QAction *>(sender() ) ;
    if ( action )
        loadProject(action->data().toString()) ;
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
            mb.setInformativeText( filename ) ;
            mb.setText( "Could not open file:" ) ;
            mb.exec() ;
            return ;
        }
        tabWidget->setCurrentWidget( textEdit ) ;
        textEdit->read( currentFile ) ;
        textEdit->setModified( false ) ;
        currentFile->close() ;
    }
}

void MainWindow::loadProject( const QString filename ) {
    if ( filename.isNull() )
        return ;
    if ( filename == currentFile->fileName() )
        return ;
    projectHandler->Load( filename ) ;
}

void MainWindow::on_actionOpen_triggered() {
    QString filename = QFileDialog::getOpenFileName(
        this, tr("Open Source File"), ".", tr( "Picoblaze source files (*.psm *.psh) ; ;All files (*.*)" ) ) ;
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
    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Open Source File"), ".", tr( "Picoblaze source files (*.psm *.psh) ; ;All files (*.*)" ) ) ;
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
    tabWidget->setCurrentWidget( splitter ) ;

    QProcess pBlazASM( this ) ;
    pBlazASM.setProcessChannelMode( QProcess::MergedChannels ) ;

    QString program = "./pBlazASM.exe " ;
    QStringList arguments =  projectHandler->asmArguments() ;
    qDebug() << program << arguments ;

    pBlazASM.start( program, arguments ) ;
    if ( ! pBlazASM.waitForStarted( 1000 ) ) {
        logBox->appendPlainText( "pBlazASM failed:\n" + pBlazASM.errorString() ) ;
        return ;
    }
    if ( ! pBlazASM.waitForFinished( 2000 ) ) {
        logBox->appendPlainText( "pBlazASM failed:\n" + pBlazASM.errorString() ) ;
    } else {
        logBox->appendPlainText( "pBlazASM output:\n" + pBlazASM.readAll() ) ;
    }
}

void MainWindow::setCurrentFile(const QString &filename) {
    currentFile->setFileName ( filename ) ;
    setWindowFilePath( filename ) ;

    QSettings settings( "Mediatronix", "pBlazBLD" ) ;

    QStringList files = settings.value("recentFileList").toStringList() ;
    files.removeAll( filename ) ;
    files.prepend( filename ) ;
    while (files.size() > MAXRECENTFILES)
        files.removeLast() ;

    settings.setValue("recentFileList", files) ;
    updateRecentlyUsedActions() ;
}

void MainWindow::updateRecentlyUsedActions() {
    QSettings settings( "Mediatronix", "pBlazBLD" ) ;

    QStringList projects = settings.value( "recentProjectList" ).toStringList() ;

    int numRecentProjects = qMin(projects.size(), (int)MAXRECENTFILES) ;

    for (int i = 0 ; i < numRecentProjects ; ++i) {
        QString text = QString("&%1 %2").arg(i + 1).arg(strippedName(projects[ i ]) ) ;
        recentProjectActs[ i ]->setText( text ) ;
        recentProjectActs[ i ]->setData( projects[ i ] ) ;
        recentProjectActs[ i ]->setVisible( true ) ;
    }
    for (int j = numRecentProjects ; j < MAXRECENTFILES ; ++j)
        recentProjectActs[j]->setVisible(false) ;

    separatorProjectAct->setVisible( numRecentProjects > 0 ) ;

    QStringList files = settings.value( "recentFileList" ).toStringList() ;

    int numRecentFiles = qMin(files.size(), (int)MAXRECENTFILES) ;

    for (int i = 0 ; i < numRecentFiles ; ++i) {
        QString text = QString("&%1 %2").arg(i + 1).arg(strippedName(files[ i ]) ) ;
        recentFileActs[ i ]->setText( text ) ;
        recentFileActs[ i ]->setData( files[ i ] ) ;
        recentFileActs[ i ]->setVisible( true ) ;
    }
    for (int j = numRecentFiles ; j < MAXRECENTFILES ; ++j)
        recentFileActs[j]->setVisible(false) ;

    separatorFileAct->setVisible( numRecentFiles > 0 ) ;
}

QString MainWindow::strippedName(const QString &fullFileName) {
    return QFileInfo(fullFileName).fileName() ;
}

void MainWindow::on_actionAdd_source_file_triggered() {
    tabWidget->setCurrentWidget( projectHandler->getVariantEditor() ) ;
    projectHandler->addSourceFile( currentFile->fileName() ) ;
}

void MainWindow::on_actionRemove_source_file_triggered() {
    tabWidget->setCurrentWidget( projectHandler->getVariantEditor() ) ;
    projectHandler->removeSourceFile( currentFile->fileName() ) ;
}

void MainWindow::on_actionNew_Project_triggered() {
    tabWidget->setCurrentWidget( projectHandler->getVariantEditor() ) ;
    projectHandler->New() ;
}

void MainWindow::on_actionOpen_Project_triggered() {
    tabWidget->setCurrentWidget( projectHandler->getVariantEditor() ) ;
    projectHandler->Load( "" ) ;

    if ( projectHandler->fileName().isEmpty() )
        return ;

    QSettings settings( "Mediatronix", "pBlazBLD" ) ;

    QStringList projects = settings.value("recentProjectList").toStringList() ;
    projects.removeAll( projectHandler->fileName() ) ;
    projects.prepend( projectHandler->fileName() ) ;
    while ( projects.size() > MAXRECENTFILES )
        projects.removeLast() ;

    settings.setValue("recentProjectList", projects ) ;
    updateRecentlyUsedActions() ;
}

void MainWindow::on_actionSave_Project_triggered() {
    tabWidget->setCurrentWidget( projectHandler->getVariantEditor() ) ;
    projectHandler->Save() ;
}

void MainWindow::on_actionClose_triggered() {
    tabWidget->setCurrentWidget( textEdit ) ;
    if ( maybeSaveFile() )
        textEdit->clear() ;
    textEdit->setModified( false ) ;
}
