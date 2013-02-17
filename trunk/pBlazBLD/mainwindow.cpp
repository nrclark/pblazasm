#include "mainwindow.h"
#include "ui_mainwindow.h"


logHighlighter::logHighlighter( QTextDocument * parent ) : QSyntaxHighlighter(parent) {
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

void logHighlighter::highlightBlock(const QString &text) {
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
    hSplitter = new QSplitter() ;
    hSplitter->setOrientation( Qt::Horizontal ) ;
    tabWidget->addTab( hSplitter, "Project" ) ;

    vSplitter = new QSplitter( hSplitter ) ;
    vSplitter->setOrientation( Qt::Vertical ) ;

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
    vSplitter->addWidget( projectHandler->getVariantEditor() ) ;

    // create a settings manager
    settingsHandler = new QmtxSettingsHandler( this ) ;
    vSplitter->addWidget( settingsHandler->getVariantEditor() ) ;


    // log
    logBox = new QPlainTextEdit() ;
    hSplitter->addWidget( logBox ) ;
    logBox->setReadOnly( true ) ;
    logBox->setFont( projectHandler->getFont() ) ;
    connect( logBox, SIGNAL(cursorPositionChanged()), this, SLOT(highlightLogBox())) ;
    highlighter = new logHighlighter( logBox->document() ) ;


    // our source editor
    textEdit = new QPlainTextEdit( this ) ;
    tabWidget->addTab( textEdit, "Source" ) ;
    textEdit->document()->setDefaultFont( projectHandler->getFont() );

    // and its lexer for Picoblaze Assembler source
//
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
//    connect( ui->actionDelete, SIGNAL (triggered() ), textEdit, SLOT( delete_selection() ) ) ;
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
//    connect( textEdit, SIGNAL( marginClicked(int, int, Qt::KeyboardModifiers) ),
//        this, SLOT( onMarginClicked(int, int, Qt::KeyboardModifiers) ) ) ;

    // editor status
    connect( textEdit, SIGNAL( cursorPositionChanged() ), this, SLOT( onCursorpositionchanged() ) ) ;
    connect( textEdit, SIGNAL( modificationChanged(bool) ), this, SLOT( onModificationchanged(bool) ) ) ;
    connect( textEdit, SIGNAL( textChanged() ), this, SLOT( onTextchanged() ) ) ;

    ui->actionSave->setEnabled( false ) ;
    connect( textEdit, SIGNAL( modificationChanged(bool) ), ui->actionSave, SLOT( setEnabled(bool) ) ) ;

    // exit
    connect( ui->actionExit, SIGNAL( triggered() ), this, SLOT( close() ) ) ;

    readSettings() ;

    // builders
    ui->actionAssemble->setEnabled( QFile::exists( settingsHandler->pBlazASM() ) ) ;
    ui->actionMerge->setEnabled( QFile::exists( settingsHandler->pBlazMRG() ) ) ;
    ui->actionBitfile->setEnabled( QFile::exists( settingsHandler->pBlazBIT() ) ) ;

    hSplitter->setStretchFactor( 0, 40 ) ;
    hSplitter->setStretchFactor( 1, 60 ) ;

    vSplitter->setStretchFactor( 0, 80 ) ;
    vSplitter->setStretchFactor( 1, 20 ) ;
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
 //   textEdit->document()->set
 //           setCursorPosition( line, 0 ) ;
    textEdit->setFocus() ;
}


void MainWindow::readSettings() {
    QSettings settings( "Mediatronix", "pBlazBLD" ) ;

    settingsHandler->Read() ;
//    setWindowState( (Qt::WindowStates)settings.value( "state", (int)Qt::WindowNoState ) ) ;
    QPoint pos = settings.value("pos", QPoint(100, 100)).toPoint() ;
    QSize size = settings.value("size", QSize(800, 600)).toSize() ;

    updateRecentlyUsedActions() ;


    resize( size ) ;
    move( pos ) ;
}

void MainWindow::writeSettings() {
    QSettings settings( "Mediatronix", "pBlazBLD" ) ;

    settingsHandler->Write() ;
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
        textEdit->document()->setModified( false ) ;
        setCurrentFile( "" ) ;
    }
}
void MainWindow::onCursorpositionchanged() {
//    int line = textEdit->textCursor()->row ;
//    int column = textEdit->textCursor()->columnNumber() ;
//    lbPosition->setText( QString( "%1 : %2" ).arg( line + 1 ).arg( column + 1 ) ) ;
}

void MainWindow::onTextchanged() {
    ui->actionUndo->setEnabled(textEdit->document()->isUndoAvailable()) ;
    ui->actionRedo->setEnabled(textEdit->document()->isRedoAvailable()) ;
}

void MainWindow::onMarginClicked( int margin, int line, Qt::KeyboardModifiers state ) {
    Q_UNUSED(line ) ;
//    textEdit->markerAdd( margin, state ) ;
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
//        textEdit->document()->( currentFile ) ;
        textEdit->document()->setModified( false ) ;
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
        return saveAs() ;
    else
        return saveFile( currentFile->fileName() ) ;
}

void MainWindow::on_actionSaveAs_triggered() {
    saveAs() ;
}

bool MainWindow::saveAs() {
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

//    textEdit->write( &file ) ;
    textEdit->document()->setModified( false ) ;
    currentFile->setFileName( fileName ) ;
    statusBar()->showMessage (tr("File saved"), 2000 ) ;
    return true ;
}

bool MainWindow::maybeSaveFile() {
//    if ( textEdit->isModified() ) {
        int ret = QMessageBox::warning(this, tr("pBlazBLD"),
             "The document has been modified.\nDo you want to save your changes?",
             QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel
        ) ;
        if ( ret == QMessageBox::Yes )
            return save() ;
        else if ( ret == QMessageBox::Cancel )
            return false ;
//    }
    return true ;
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

    settings.setValue( "recentProjectList", projects ) ;
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
//    textEdit->setModified( false ) ;
}

void MainWindow::on_actionAssemble_triggered() {
    tabWidget->setCurrentWidget( hSplitter ) ;

    QProcess pBlazASM( this ) ;
    pBlazASM.setProcessChannelMode( QProcess::MergedChannels ) ;

    QString program = settingsHandler->pBlazASM() ;
    if ( program.isEmpty() )
        return ;

    QStringList arguments =  projectHandler->asmArguments() ;
    qDebug() << program << arguments ;

    pBlazASM.start( program, arguments ) ;
    if ( ! pBlazASM.waitForStarted( 1000 ) ) {
        logBox->appendPlainText( "pBlazASM failed:\n" + pBlazASM.errorString() ) ;
        return ;
    }
    if ( ! pBlazASM.waitForFinished( 2000 ) )
        logBox->appendPlainText( "pBlazASM failed:\n" + pBlazASM.errorString() ) ;
    else
        logBox->appendPlainText( "pBlazASM output:\n" + pBlazASM.readAll() ) ;
}

void MainWindow::on_actionMerge_triggered() {
    tabWidget->setCurrentWidget( hSplitter ) ;

    QProcess pBlazMRG( this ) ;
    pBlazMRG.setProcessChannelMode( QProcess::MergedChannels ) ;

    QString program = settingsHandler->pBlazMRG() ;
    if ( program.isEmpty() )
        return ;

    QStringList arguments = projectHandler->mrgArguments() ;
    qDebug() << program << arguments ;

    pBlazMRG.start( program, arguments ) ;
    if ( ! pBlazMRG.waitForStarted( 1000 ) ) {
        logBox->appendPlainText( "pBlazMRG failed:\n" + pBlazMRG.errorString() ) ;
        return ;
    }
    if ( ! pBlazMRG.waitForFinished( 2000 ) )
        logBox->appendPlainText( "pBlazMRG failed:\n" + pBlazMRG.errorString() ) ;
    else
        logBox->appendPlainText( "pBlazMRG output:\n" + pBlazMRG.readAll() ) ;
}

void MainWindow::on_actionBitfile_triggered() {
    tabWidget->setCurrentWidget( hSplitter ) ;

    QProcess pBlazBIT( this ) ;
    pBlazBIT.setProcessChannelMode( QProcess::MergedChannels ) ;

    QString program = settingsHandler->pBlazBIT() ;
    if ( program.isEmpty() )
        return ;

    QStringList arguments =  projectHandler->bitArguments() ;
    qDebug() << program << arguments ;

    pBlazBIT.start( program, arguments ) ;
    if ( ! pBlazBIT.waitForStarted( 1000 ) ) {
        logBox->appendPlainText( "pBlazBIT failed:\n" + pBlazBIT.errorString() ) ;
        return ;
    }
    if ( ! pBlazBIT.waitForFinished( 2000 ) )
        logBox->appendPlainText( "pBlazBIT failed:\n" + pBlazBIT.errorString() ) ;
    else
        logBox->appendPlainText( "pBlazBIT output:\n" + pBlazBIT.readAll() ) ;
}
