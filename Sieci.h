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
        // NAPRAWIONE (UCHYBIENIE #2): dodano stan Nieokreslony, aby m_tryb miał
        // sensowną wartość początkową. Wcześniej m_tryb był niezainicjowany,
        // a set_tryb() porównywał go z nową wartością (niezdefiniowane zachowanie).
        enum Tryb { Nieokreslony, Regulator, Obiekt };

        // Typy pakietów w protokole sieciowym (nagłówek = 1B typ + 4B rozmiar)
        enum TypPakietu : quint8 {
                PAKIET_KONFIG = 1,  // JSON - pełna konfiguracja UAR
                PAKIET_PROBKA = 2   // binarne dane symulacji (kolejny etap)
        };

    public:
        explicit Sieci(QObject *parent = nullptr);
        ~Sieci() override;

        void ustawParametry(quint16 port = 45763, const QString &host = "127.0.0.1");
        void set_tryb(Tryb tryb);
        void rozlacz();

        Tryb get_tryb() const { return m_tryb; }
        bool czyPolaczony() const;

    public slots:
        void wyslijKonfiguracje(const QJsonObject &konfig);

    signals:
        void polaczono();
        void rozlaczono();
        void statusZmieniony(const QString &opis);
        void odebranoKonfiguracje(const QJsonObject &konfig);

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

        // NAPRAWIONE (UCHYBIENIE #1): bufor jako pole klasy, a nie lokalna
        // zmienna w onReadyRead. TCP może dostarczać dane we fragmentach,
        // więc niepełny pakiet musi poczekać na kolejne wywołanie readyRead.
        QByteArray m_bufor;

        void podlaczSocket();
        void czyscPolaczenie();
        void parsujBufor();
        void wyslijPakiet(TypPakietu typ, const QByteArray &payload);
};

#endif // SIECI_H
