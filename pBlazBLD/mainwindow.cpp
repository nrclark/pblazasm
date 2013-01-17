#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    QIcon icon ;
    icon.addFile( ":/files/worker.ico" ) ;
    this->setWindowTitle( "pBlazBLD V0.1 (Qt4.8.4) - http://www.mediatronix.com" ) ;
    this->setWindowIcon( icon ) ;
    qApp->setWindowIcon( icon ) ;
    qApp->setApplicationName("pBlazBLD V0.1 (Qt4.8.4)");

    QFont fixedFont( "Consolas [Monaco]", 9 ) ;

    lbMode = new QLabel( tr( "insert" ) ) ;
    ui->statusBar->addWidget( lbMode, 50 ) ;
    lbModified = new QLabel( tr("N") ) ;
    ui->statusBar->addWidget( lbModified, 50 ) ;
    lbPosition = new QLabel( tr("00:00") ) ;
    ui->statusBar->addWidget( lbPosition, 50 ) ;

    // source editor
    textEdit = new QsciScintilla() ;
    setCentralWidget( textEdit ) ;

    // lexer for Picoblaze Assembler text
    lexer = new QsciLexerPsm() ;
    textEdit->setLexer( lexer ) ;
    lexer->setFont( fixedFont ) ;

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

    readSettings() ;
}

MainWindow::~MainWindow() {
    delete ui ;
}

void MainWindow::readSettings() {
    QSettings settings( "Mediatronix", "pBlazBLD" ) ;
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();
    resize( size ) ;
    move( pos ) ;
}

void MainWindow::writeSettings() {
    QSettings settings( "Mediatronix", "pBlazBLD" ) ;
    settings.setValue( "pos", pos() ) ;
    settings.setValue( "size", size() ) ;
}

void MainWindow::closeEvent( QCloseEvent *event ) {
    if ( maybeSave() ) {
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
    if ( maybeSave() ) {
        textEdit->clear() ;
        textEdit->setModified( false ) ;
        currentFile->setFileName( "" ) ;
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
    textEdit->markerAdd( margin, state ) ;
}

void MainWindow::onModificationchanged( bool m ) {
    lbModified->setText( m ? "Y" : "N" ) ;
}

void MainWindow::on_actionOpen_triggered() {
    QString fileName = QFileDialog::getOpenFileName(
        this, tr("Open Source File"), ".", tr( "Picoblaze source files (*.psm *.psh)" ) ) ;
    if ( fileName == "" )
        return ;

    if ( maybeSave() ) {
        currentFile->setFileName ( fileName ) ;

        if ( ! currentFile->open(QIODevice::ReadOnly | QIODevice::Text) ) {
            QMessageBox mb ;
            mb.setStandardButtons( QMessageBox::Ok ) ;
            mb.setInformativeText( fileName );
            mb.setText( "Could not open file:" ) ;
            mb.exec() ;
            return ;
        }
        textEdit->read( currentFile ) ;
        textEdit->setModified( false ) ;
        currentFile->close() ;
    }
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
            tr("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
        return false ;
    }

    textEdit->write( &file ) ;
    textEdit->setModified( false ) ;
    currentFile->setFileName( fileName ) ;
    statusBar()->showMessage (tr("File saved"), 2000 ) ;
    return true ;
}

bool MainWindow::maybeSave() {
    if ( textEdit->isModified() ) {
        int ret = QMessageBox::warning(this, tr("pBlazBLD"),
             tr("The document has been modified.\n"
                "Do you want to save your changes?"),
             QMessageBox::Yes | QMessageBox::Default,
             QMessageBox::No,
             QMessageBox::Cancel | QMessageBox::Escape
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

    QString program = "./pBlazASM.exe" ;

    QStringList arguments ;
    arguments << "-v" << "-6" << "x" ; // << currentFile->fileName() ;
    qDebug() << arguments ;

    pBlazASM.start( program, arguments ) ;
    if ( ! pBlazASM.waitForStarted( 1000 ) )
             return ;
    if ( ! pBlazASM.waitForFinished( 2000 ) )
        qDebug() << "pBlazASM failed:" << pBlazASM.errorString();
    else
        qDebug() << "pBlazASM output:" << pBlazASM.readAll();
}
