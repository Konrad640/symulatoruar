#ifndef SIECI_H
#define SIECI_H

#include <QDebug>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QObject>

class Sieci : public QObject
{
        Q_OBJECT
    public:
        enum Tryb { Regulator, Obiekt };

    private:
        Tryb m_tryb;

        quint16 port = 95763;
        QString host = "127.0.0.1";

        QTcpServer server;
        QTcpSocket *socket = nullptr;

    public:
        Sieci(QObject *parent = nullptr);

        void start(quint16 port = 95763, QString host = "127.0.0.1");

        void wyslijWiadomosc(const QString &msg, int number);

    private:
        // regulator
        void setupRegulator();
        void onNewConnection();

        // obiekt
        void setupObiekt();

        // wspolne
        void czyscPolaczenie();
        void onReadyRead();

    public:
        // akcesory
        Tryb get_tryb() { return m_tryb; }
        void set_tryb(Tryb tryb);
};

#endif // SIECI_H
