#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    this->setWindowTitle( "pBlazBLD V0.1 (Qt4.8.4) - http://www.mediatronix.com" ) ;
    this->setWindowIcon( QIcon( ":/files/worker.ico" ) ) ;
    qApp->setWindowIcon( QIcon( ":/files/worker.ico" )  ) ;
    qApp->setApplicationName("pBlazBLD V0.1 (Qt4.8.4)");

    QFont fixedFont( "Consolas [Monaco]", 9 ) ;

    // source editor
    textEdit = new QsciScintilla() ;
    setCentralWidget( textEdit ) ;

    lexer = new QsciLexerPsm() ;
    textEdit->setLexer( lexer ) ;
    lexer->setFont( fixedFont ) ;

    textEdit->marginLineNumbers( 50 ) ;
    currentFile = new QFile() ;

    connect( textEdit, SIGNAL( modificationChanged(bool) ), this, SLOT( on_modificationChanged(bool) ) ) ;

    connect( ui->actionAbout_Qt, SIGNAL( triggered() ), qApp, SLOT( aboutQt() ) ) ;

    connect( ui->actionCut, SIGNAL( triggered() ), textEdit, SLOT( cut() ) ) ;
    connect( ui->actionCopy, SIGNAL( triggered() ), textEdit, SLOT( copy() ) ) ;
    connect( ui->actionPaste, SIGNAL( triggered() ), textEdit, SLOT( paste() ) ) ;

    ui->actionCut->setEnabled(false);
    ui->actionCopy->setEnabled(false);
    connect( textEdit, SIGNAL( copyAvailable(bool) ), ui->actionCut, SLOT( setEnabled(bool) ) ) ;
    connect( textEdit, SIGNAL( copyAvailable(bool) ), ui->actionCopy, SLOT( setEnabled(bool) ) ) ;
}

MainWindow::~MainWindow() {
    delete ui ;
}

void MainWindow::on_actionAbout_triggered() {
    QMessageBox::about( this, "pBlazBLD", windowTitle() +
        "\n\nThis program comes with ABSOLUTELY NO WARRANTY." +
        "\nThis is free software, and you are welcome to redistribute it" +
        "\nunder certain conditions. See <http://www.gnu.org/licenses/>"
    ) ;
}

void MainWindow::on_actionExit_triggered() {
    qApp->exit();
}


void MainWindow::on_actionNew_triggered() {
    if ( maybeSave() ) {
        textEdit->clear() ;
        textEdit->setModified( false ) ;
        currentFile->setFileName( "" ) ;
    }
}

void MainWindow::on_modificationChanged(bool m) {
    statusBar()->showMessage( QString("modified %1" ).arg( m ), 2000 ) ;
}

void MainWindow::on_actionOpen_triggered() {
    if ( maybeSave() ) {
        QString fileName = QFileDialog::getOpenFileName(
            this, tr("Open Source File"), ".", tr( "Picoblaze source files (*.psm *.psh)" ) ) ;
        currentFile->setFileName ( fileName ) ;

        if (! currentFile->open(QIODevice::ReadOnly | QIODevice::Text) ) {
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

    if ( !file.open(QFile::WriteOnly) ) {
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
            return save();
        else if ( ret == QMessageBox::Cancel )
            return false ;
    }
    return true;
}



