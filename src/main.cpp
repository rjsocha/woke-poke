#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QLockFile>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QByteArray>
#include <QHostAddress>
#include <QUdpSocket>
#include <QRegularExpression>

static QByteArray parseMAC(const QString &macStr) {
  QString s = macStr;
  s.remove(QRegularExpression("[^0-9A-Fa-f]"));
  if (s.size() != 12)
    return {};
  QByteArray mac;
  for (int i = 0; i < 12; i += 2) {
    bool ok = false;
    char byte = static_cast<char>(s.mid(i, 2).toUInt(&ok, 16));
    if (!ok)
      return {};
    mac.append(byte);
  }
  return mac;
}

bool WOL(const QByteArray mac, const QHostAddress &broadcast = QHostAddress::Broadcast, quint16 port = 9) {
  QByteArray packet;
  packet.fill(char(0xFF), 6);
  for (int i = 0; i < 16; ++i) packet.append(mac);
  QUdpSocket sock;
  sock.bind(QHostAddress::AnyIPv4, 0, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
  qint64 sent = sock.writeDatagram(packet, broadcast, port);
  if (sent != packet.size()) {
    return false;
  }
  return true;
}

int main(int argc, char *argv[]) {

/*
  QLockFile lock("/dev/shm/woke-poke");
  if (!lock.tryLock()) {
    qWarning("another instance is already running ...");
    return 0;
  }
*/

  QApplication app(argc, argv);
  QIcon icon(":/woke-poke.png");
  if (icon.isNull()) {
    qWarning("Tray icon not found!");
    return 1;
  }

  QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/woke-poke";
  QFileInfo fi(configPath);
  if (!fi.exists()) {
    return 0;
  }

  QSystemTrayIcon *tray = new QSystemTrayIcon(icon, &app);
  QMenu *menu = new QMenu();

  QSettings settings(configPath, QSettings::IniFormat);
  settings.setIniCodec("UTF-8");
  settings.beginGroup("wol");
  QStringList keys = settings.childKeys();
  for (const QString &key : keys) {
    QString macStr = settings.value(key).toString().trimmed();
    QByteArray mac = parseMAC(macStr);
    if (mac.size() != 6) {
      qWarning().noquote() << QString("Invalid MAC: %1 (%2)").arg(macStr,key);
      continue;
    }
    QAction *wake = new QAction(key, menu);
    menu->addAction(wake);
    QObject::connect(wake, &QAction::triggered, menu, [mac](bool) {
      WOL(mac);
    });
  }
  settings.endGroup();

  QAction *quitAction = new QAction("Quit");
  menu->addSeparator();
  menu->addAction(quitAction);
  tray->setContextMenu(menu);
  QObject::connect(quitAction, &QAction::triggered, [&]() {
   app.quit();
  });
  tray->show();
  return app.exec();
}
