#ifndef SIECI_H
#define SIECI_H

#include <QDebug>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QObject>

class Sieci : public QObject
{
        Q_OBJECT
    public:

        enum Tryb { Nieokreslony, Regulator, Obiekt };

        enum TypPakietu : quint8 {
                PAKIET_KONFIG = 1,
                PAKIET_PROBKA = 2
        };

        enum Serializacja { SER_BINARNA, SER_TEKSTOWA };

    public:
        explicit Sieci(QObject *parent = nullptr);
        ~Sieci() override;

        void ustawParametry(quint16 port = 45763, const QString &host = "127.0.0.1");
        void set_tryb(Tryb tryb);
        void rozlacz();

        Tryb get_tryb() const { return m_tryb; }
        bool czyPolaczony() const;
        QString opisPartnera() const;
        void setSerializacja(Serializacja s) { m_serializacja = s; }
        Serializacja getSerializacja() const { return m_serializacja; }

    public slots:
        void wyslijKonfiguracje(const QJsonObject &konfig);
        void wyslijProbke(const QJsonObject &probka);

    signals:
        void polaczono();
        void rozlaczono();
        void statusZmieniony(const QString &opis);
        void odebranoKonfiguracje(const QJsonObject &konfig);
        void odebranoProbke(const QJsonObject &probka);

    private slots:
        void onNewConnection();
        void onConnected();
        void onDisconnected();
        void onReadyRead();
        void onError(QAbstractSocket::SocketError blad);

    private:
        Tryb m_tryb = Nieokreslony;
        quint16 m_port = 45763;
        QString m_host = "127.0.0.1";

        QTcpServer m_server;
        QTcpSocket *m_socket = nullptr;
        QByteArray m_bufor;
        Serializacja m_serializacja = SER_BINARNA;

        void podlaczSocket();
        void ustawOpcjeSocketu();
        void czyscPolaczenie();
        void zakonczPolaczenie(const QString &opis);
        void parsujBufor();
        void wyslijPakiet(TypPakietu typ, const QByteArray &payload);
};

#endif // SIECI_H
