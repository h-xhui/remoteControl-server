#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
   init();
   socketDataHandle();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init() {
    ui->setupUi(this);
    initKeyMap();
    initSocket();
    isFile = false;
}

void MainWindow::getScreenInfo(){
    QScreen *screen = QGuiApplication::primaryScreen();
    QPixmap pix = screen->grabWindow(0);
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    pix.save(&buffer, "jpg", 100);
    QByteArray dataArray;
    dataArray.append(buffer.data());
    m_tcp->write(dataArray);
}

void MainWindow::on_pushButton_clicked()
{
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(getScreenInfo()));
    timer->start(45);

    //getScreenInfo();
}

void MainWindow::initSocket() {
    m_s = new QTcpServer(this);
    m_s->listen(QHostAddress::Any, 8888);
}

void MainWindow::socketDataHandle() {
    connect(m_s, &QTcpServer::newConnection, this, [=](){
        m_tcp = m_s->nextPendingConnection();
        connect(m_tcp, &QTcpSocket::readyRead, this, [=](){
            QByteArray msg = m_tcp->readAll();
            int controlMsgSize = sizeof(SocketData);
            if (msg.size() % controlMsgSize == 0) {
                mouseAndKeyboardHandler(msg);
            } else {
                receiveFile(msg);
            }
        });
    });
}

void MainWindow::mouseAndKeyboardHandler(QByteArray& msg) {
    if (QString(msg.mid(0, 1)) == "0" || QString(msg.mid(1, 1)) == "1") {
        receiveFile(msg);
        return;
    }
    int controlMsgSize = sizeof(SocketData);
    for (int i = 0; i < msg.size(); i += controlMsgSize) {
        SocketData *data = new SocketData;
        memcpy(data, msg.mid(i, controlMsgSize).data(), controlMsgSize);
        //qDebug() <<"! "<< data->type << " " << data->x << " " << data->y;
        if (data->type == 2) {
            SetCursorPos(data->x,data->y);
            mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTDOWN, data->x, data->y, 0, 0);
         } else if (data->type == 3) {
            SetCursorPos(data->x,data->y);
            mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_RIGHTDOWN, data->x, data->y, 0, 0);
        } else if (data->type == 4) {
            keybd_event(m_KeyMap[data->x], 0, 0, 0);
        } else if (data->type == 5) {
            SetCursorPos(data->x,data->y);
            mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_WHEEL, data->x, data->y, data->delta, 0);
        } else if (data->type == 6) {
            SetCursorPos(data->x,data->y);
        } else if (data->type == 7) {
            SetCursorPos(data->x,data->y);
            mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTUP, data->x, data->y, 0, 0);
        } else if (data->type == 8) {
            SetCursorPos(data->x,data->y);
            mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_RIGHTUP, data->x, data->y, 0, 0);
        } else if (data->type == 9) {
            keybd_event(m_KeyMap[data->x], 0, KEYEVENTF_KEYUP, 0);
        }
    }
}

void MainWindow::copyFileToClipboard(char *szFileName){
    UINT uDropEffect;
        HGLOBAL hGblEffect;
        LPDWORD lpdDropEffect;
        DROPFILES stDrop;

        HGLOBAL hGblFiles;
        LPSTR lpData;

        uDropEffect = RegisterClipboardFormat((wchar_t*)"Preferred DropEffect");
        hGblEffect = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE, sizeof(DWORD));
        lpdDropEffect = (LPDWORD)GlobalLock(hGblEffect);
        *lpdDropEffect = DROPEFFECT_COPY;//复制; 剪贴则du用DROPEFFECT_MOVE
        GlobalUnlock(hGblEffect);

        stDrop.pFiles = sizeof(DROPFILES);
        stDrop.pt.x = 0;
        stDrop.pt.y = 0;
        stDrop.fNC = FALSE;
        stDrop.fWide = FALSE;

        hGblFiles = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE, \
            sizeof(DROPFILES) + strlen(szFileName) + 2);
        lpData = (LPSTR)GlobalLock(hGblFiles);
        memcpy(lpData, &stDrop, sizeof(DROPFILES));
        strcpy(lpData + sizeof(DROPFILES), szFileName);
        GlobalUnlock(hGblFiles);

        OpenClipboard(NULL);
        EmptyClipboard();
        SetClipboardData(CF_HDROP, hGblFiles);
        SetClipboardData(uDropEffect, hGblEffect);
        CloseClipboard();
}

void MainWindow::receiveFile(QByteArray &msg) {
    if(!msg[0]){//接收头
        QString fileInfo(msg.mid(1));
        QStringList list = fileInfo.split("&");
        fileName = list[0];
        fileSize = list[1].toInt();
        recvSize = 0;

        file.setFileName("C:\\Users\\hongjinhui\\Documents\\"+fileName);
        qDebug() << fileName << " " << fileSize;
        if (fileSize == 0) {
            qDebug() << msg;
            return;
        }
        int count = 0, i = 0;
        for (; i < fileInfo.size(); ++i) {
            if (fileInfo[i] == '&') ++count;
            if (count == 2) {
                break;
            }
        }
        isFile = true;
        if(!file.open(QIODevice::WriteOnly)){
            qDebug() << "打开出错";
            return;
        }
        if (list[2].size() == 0) {
            return;
        }
        qint64 len = file.write(msg.mid(i+3));
        recvSize += len;
        file.close();
        qDebug() <<len<< " 文件接收完成";
        std::string s = "C:\\Users\\hongjinhui\\Documents\\" + fileName.toStdString();
        copyFileToClipboard((char*)s.data());
        isFile = false;
    }
    else{//接收文件
       if (!isFile) {
           qDebug() << QString(msg);
           return;
       }
       qint64 len = file.write(msg.mid(1));
       recvSize += len;
       file.close();
       qDebug() <<len<< " 文件接收完成";
       std::string s = "C:\\Users\\hongjinhui\\Documents\\" + fileName.toStdString();
       copyFileToClipboard((char*)s.data());
       isFile = false;
    }
}

void MainWindow::initKeyMap() {
    m_KeyMap.insert(Qt::Key_Left,0x25);
    m_KeyMap.insert(Qt::Key_Up,0x26);
    m_KeyMap.insert(Qt::Key_Right,0x27);
    m_KeyMap.insert(Qt::Key_Down,0x28);
    m_KeyMap.insert(Qt::Key_Backspace,0x08);
    m_KeyMap.insert(Qt::Key_Tab,0x09);
    m_KeyMap.insert(Qt::Key_Clear,0x0C);
    m_KeyMap.insert(Qt::Key_Return,0x0D);
    m_KeyMap.insert(Qt::Key_Enter,0x0D);
    m_KeyMap.insert(Qt::Key_Shift,0x10);
    m_KeyMap.insert(Qt::Key_Control,0x11);
    m_KeyMap.insert(Qt::Key_Alt,0x12);
    m_KeyMap.insert(Qt::Key_Pause,0x13);
    m_KeyMap.insert(Qt::Key_CapsLock,0x14);
    m_KeyMap.insert(Qt::Key_Escape,0x1B);
    m_KeyMap.insert(Qt::Key_Space,0x20);
    m_KeyMap.insert(Qt::Key_PageUp,0x21);
    m_KeyMap.insert(Qt::Key_PageDown,0x22);
    m_KeyMap.insert(Qt::Key_End,0x23);
    m_KeyMap.insert(Qt::Key_Home,0x24);
    m_KeyMap.insert(Qt::Key_Select,0x29);
    m_KeyMap.insert(Qt::Key_Print,0x2A);
    m_KeyMap.insert(Qt::Key_Execute,0x2B);
    m_KeyMap.insert(Qt::Key_Printer,0x2C);
    m_KeyMap.insert(Qt::Key_Insert,0x2D);
    m_KeyMap.insert(Qt::Key_Delete,0x2E);
    m_KeyMap.insert(Qt::Key_Help,0x2F);
    m_KeyMap.insert(Qt::Key_0,0x30);
    m_KeyMap.insert(Qt::Key_ParenRight,0x30); // )
    m_KeyMap.insert(Qt::Key_1,0x31);
    m_KeyMap.insert(Qt::Key_Exclam,0x31); // !
    m_KeyMap.insert(Qt::Key_2,0x32);
    m_KeyMap.insert(Qt::Key_At,0x32); // @
    m_KeyMap.insert(Qt::Key_3,0x33);
    m_KeyMap.insert(Qt::Key_NumberSign,0x33); // #
    m_KeyMap.insert(Qt::Key_4,0x34);
    m_KeyMap.insert(Qt::Key_Dollar,0x34); // $
    m_KeyMap.insert(Qt::Key_5,0x35);
    m_KeyMap.insert(Qt::Key_Percent,0x35); // %
    m_KeyMap.insert(Qt::Key_6,0x36);
    m_KeyMap.insert(Qt::Key_AsciiCircum,0x36); // ^
    m_KeyMap.insert(Qt::Key_7,0x37);
    m_KeyMap.insert(Qt::Key_Ampersand,0x37); // &
    m_KeyMap.insert(Qt::Key_8,0x38);
    m_KeyMap.insert(Qt::Key_Asterisk,0x38); // *
    m_KeyMap.insert(Qt::Key_9,0x39);
    m_KeyMap.insert(Qt::Key_ParenLeft,0x39); // (
    m_KeyMap.insert(Qt::Key_A,0x41);
    m_KeyMap.insert(Qt::Key_B,0x42);
    m_KeyMap.insert(Qt::Key_C,0x43);
    m_KeyMap.insert(Qt::Key_D,0x44);
    m_KeyMap.insert(Qt::Key_E,0x45);
    m_KeyMap.insert(Qt::Key_F,0x46);
    m_KeyMap.insert(Qt::Key_G,0x47);
    m_KeyMap.insert(Qt::Key_H,0x48);
    m_KeyMap.insert(Qt::Key_I,0x49);
    m_KeyMap.insert(Qt::Key_J,0x4A);
    m_KeyMap.insert(Qt::Key_K,0x4B);
    m_KeyMap.insert(Qt::Key_L,0x4C);
    m_KeyMap.insert(Qt::Key_M,0x4D);
    m_KeyMap.insert(Qt::Key_N,0x4E);
    m_KeyMap.insert(Qt::Key_O,0x4F);
    m_KeyMap.insert(Qt::Key_P,0x50);
    m_KeyMap.insert(Qt::Key_Q,0x51);
    m_KeyMap.insert(Qt::Key_R,0x52);
    m_KeyMap.insert(Qt::Key_S,0x53);
    m_KeyMap.insert(Qt::Key_T,0x54);
    m_KeyMap.insert(Qt::Key_U,0x55);
    m_KeyMap.insert(Qt::Key_V,0x56);
    m_KeyMap.insert(Qt::Key_W,0x57);
    m_KeyMap.insert(Qt::Key_X,0x58);
    m_KeyMap.insert(Qt::Key_Y,0x59);
    m_KeyMap.insert(Qt::Key_Z,0x5A);
    m_KeyMap.insert(Qt::Key_multiply,0x6A);
    m_KeyMap.insert(Qt::Key_F1,0x70);
    m_KeyMap.insert(Qt::Key_F2,0x71);
    m_KeyMap.insert(Qt::Key_F3,0x72);
    m_KeyMap.insert(Qt::Key_F4,0x73);
    m_KeyMap.insert(Qt::Key_F5,0x74);
    m_KeyMap.insert(Qt::Key_F6,0x75);
    m_KeyMap.insert(Qt::Key_F7,0x76);
    m_KeyMap.insert(Qt::Key_F8,0x77);
    m_KeyMap.insert(Qt::Key_F9,0x78);
    m_KeyMap.insert(Qt::Key_F10,0x79);
    m_KeyMap.insert(Qt::Key_F11,0x7A);
    m_KeyMap.insert(Qt::Key_F12,0x7B);
    m_KeyMap.insert(Qt::Key_F13,0x7C);
    m_KeyMap.insert(Qt::Key_F14,0x7D);
    m_KeyMap.insert(Qt::Key_F15,0x7E);
    m_KeyMap.insert(Qt::Key_F16,0x7F);
    m_KeyMap.insert(Qt::Key_F17,0x80);
    m_KeyMap.insert(Qt::Key_F18,0x81);
    m_KeyMap.insert(Qt::Key_F19,0x82);
    m_KeyMap.insert(Qt::Key_F20,0x83);
    m_KeyMap.insert(Qt::Key_F21,0x84);
    m_KeyMap.insert(Qt::Key_F22,0x85);
    m_KeyMap.insert(Qt::Key_F23,0x86);
    m_KeyMap.insert(Qt::Key_F24,0x87);
    m_KeyMap.insert(Qt::Key_NumLock,0x90);
    m_KeyMap.insert(Qt::Key_ScrollLock,0x91);
    m_KeyMap.insert(Qt::Key_VolumeDown,0xAE);
    m_KeyMap.insert(Qt::Key_VolumeUp,0xAF);
    m_KeyMap.insert(Qt::Key_VolumeMute,0xAD);
    m_KeyMap.insert(Qt::Key_MediaStop,0xB2);
    m_KeyMap.insert(Qt::Key_MediaPlay,0xB3);
    m_KeyMap.insert(Qt::Key_Plus,0xBB); // +
    m_KeyMap.insert(Qt::Key_Minus,0xBD); // -
    m_KeyMap.insert(Qt::Key_Underscore,0xBD); // _
    m_KeyMap.insert(Qt::Key_Equal,0xBB); // =
    m_KeyMap.insert(Qt::Key_Semicolon,0xBA); // ;
    m_KeyMap.insert(Qt::Key_Colon,0xBA); // :
    m_KeyMap.insert(Qt::Key_Comma,0xBC); // ,
    m_KeyMap.insert(Qt::Key_Less,0xBC); // <
    m_KeyMap.insert(Qt::Key_Period,0xBE); // .
    m_KeyMap.insert(Qt::Key_Greater,0xBE); // >
    m_KeyMap.insert(Qt::Key_Slash,0xBF);  // /
    m_KeyMap.insert(Qt::Key_Question,0xBF); // ?
    m_KeyMap.insert(Qt::Key_BracketLeft,0xDB); // [
    m_KeyMap.insert(Qt::Key_BraceLeft,0xDB); // {
    m_KeyMap.insert(Qt::Key_BracketRight,0xDD); // ]
    m_KeyMap.insert(Qt::Key_BraceRight,0xDD); // }
    m_KeyMap.insert(Qt::Key_Bar,0xDC); // |
    m_KeyMap.insert(Qt::Key_Backslash,0xDC); // \\
    m_KeyMap.insert(Qt::Key_Apostrophe,0xDE); // '
    m_KeyMap.insert(Qt::Key_QuoteDbl,0xDE); // "
    m_KeyMap.insert(Qt::Key_QuoteLeft,0xC0); // `
    m_KeyMap.insert(Qt::Key_AsciiTilde,0xC0); // ~1
}
