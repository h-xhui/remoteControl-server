#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QScreen>
#include <QTimer>
#include <QBuffer>
#include <QLabel>
#include <windows.h>
#include <QKeyEvent>
#include <QClipboard>
#include <shlobj.h>
#include <bits/stdc++.h>
#include <QMimeData>
#include <winuser.h>
#include <direct.h>
#include <SocketData.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();
    void getScreenInfo();

private:
    Ui::MainWindow *ui;
    QTcpServer* m_s;
    QTcpSocket* m_tcp;
    QMap<int,int> m_KeyMap;//键盘映射表
    QString fileName;
    QFile file;
    qint64 fileSize;
    qint64 recvSize;
    bool isFile;
private:
    void init();
    void initSocket();
    void socketDataHandle();
    void mouseAndKeyboardHandler(QByteArray& msg);
    void initKeyMap();
    void copyFileToClipboard(char szFileName[]);
    void receiveFile(QByteArray& msg);
};
#endif // MAINWINDOW_H
