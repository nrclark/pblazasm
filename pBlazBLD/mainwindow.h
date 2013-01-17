#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QLabel>
#include <QCloseEvent>
#include <QSettings>


#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexerpsm.h>


namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow() ;
    
    void closeEvent( QCloseEvent *event ) ;

private slots:
    void onModificationchanged(bool m) ;
    void onTextchanged();
    void onMarginClicked(int margin, int line, Qt::KeyboardModifiers state);
    void onCursorpositionchanged(int line, int index);

    void on_actionAbout_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionNew_triggered();

private:
    Ui::MainWindow * ui ;

    QsciScintilla * textEdit ;
    QsciLexer * lexer ;

    QLabel * lbMode ;
    QLabel * lbModified ;
    QLabel * lbPosition ;

    QFile * currentFile ;

    bool saveFile(const QString fileName ) ;
    bool maybeSave() ;
    bool saveas() ;
    bool save() ;

    void readSettings();
    void writeSettings();
} ;

#endif // MAINWINDOW_H