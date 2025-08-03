#include "mainwindow.h"

#include "global.h"


int main(int argc, char *argv[])
{
    // 创建应用程序对象，负责处理交互事件等
    QApplication a(argc, argv);
    QString appPath = QCoreApplication::applicationDirPath();

    QFile qss(":/styles/stylesheet.qss");
    if(qss.open(QFile::ReadOnly)){
        qDebug("stylesheet.qss open success");
        QString style = QLatin1String(qss.readAll());
        a.setStyleSheet(style);
        qss.close();
    }
    else{
        qDebug("stylesheet.qss open failed");
    }

    QString configFileName = "config.ini";
    QString configPath = QDir::toNativeSeparators(appPath + QDir::separator() + configFileName);

    // 读取并显示文件原始内容
    QFile debugFile(configPath);
    if (debugFile.open(QIODevice::ReadOnly)) {
        QByteArray rawData = debugFile.readAll();
        qDebug() << "File size:" << rawData.size() << "bytes";
        qDebug() << "Raw content (hex):" << rawData.toHex();
        qDebug() << "Text content:" << QString::fromUtf8(rawData);
        debugFile.close();
    } else {
        qDebug() << "Cannot open file for debugging!";
    }


    // 读取GateServer Host Post配置信息
    QSettings settings(configPath, QSettings::IniFormat);

    qDebug() << "Settings fileName:" << settings.fileName();
    qDebug() << "Settings format:" << settings.format();
    qDebug() << "Settings status:" << settings.status();

    QString gateHost = settings.value("GateServer/host").toString();
    qDebug() << "gateHost: " << gateHost;
    QString gatePort = settings.value("GateServer/port").toString();
    qDebug() << "gatePort: " << gatePort;
    gate_url_prefix = "http://" + gateHost + ":" + gatePort;


    // 窗口对象，根据事件执行渲染。
    MainWindow w;

    w.show();

    // 执行应用
    return a.exec();
}
