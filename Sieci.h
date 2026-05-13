#ifndef SIECI_H
#define SIECI_H

#include <QDebug>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QObject>
#include <QTimer>

struct ProbkaDanych
{
    quint64 numerSeq = 0;     // Numer sekwencyjny próbki
    double czas;
    double wartoscZadana;
    double wyjscie;
    double uchyb;
    double sterowanie;

    // Serializacja binarna (dla QDataStream)
    friend QDataStream &operator<<(QDataStream &out, const ProbkaDanych &p)
    {
        out << p.numerSeq << p.czas << p.wartoscZadana << p.wyjscie << p.uchyb << p.sterowanie;
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, ProbkaDanych &p)
    {
        in >> p.numerSeq >> p.czas >> p.wartoscZadana >> p.wyjscie >> p.uchyb >> p.sterowanie;
        return in;
    }

    // Serializacja tekstowa (JSON)
    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["n"] = static_cast<qint64>(numerSeq);  // numer sekwencyjny
        obj["czas"] = czas;
        obj["w"] = wartoscZadana;
        obj["y"] = wyjscie;
        obj["e"] = uchyb;
        obj["u"] = sterowanie;
        return obj;
    }

    static ProbkaDanych fromJson(const QJsonObject &obj)
    {
        ProbkaDanych p;
        p.numerSeq = obj["n"].toVariant().toULongLong();
        p.czas = obj["czas"].toDouble();
        p.wartoscZadana = obj["w"].toDouble();
        p.wyjscie = obj["y"].toDouble();
        p.uchyb = obj["e"].toDouble();
        p.sterowanie = obj["u"].toDouble();
        return p;
    }

    // Serializacja binarna jako QByteArray
    QByteArray toBinary() const
    {
        QByteArray data;
        QDataStream out(&data, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_15);
        out << *this;
        return data;
    }

    static ProbkaDanych fromBinary(const QByteArray &data)
    {
        ProbkaDanych p;
        QDataStream in(data);
        in.setVersion(QDataStream::Qt_5_15);
        in >> p;
        return p;
    }
};

class Sieci : public QObject
{
        Q_OBJECT
    public:

        enum Tryb { Nieokreslony, Regulator, Obiekt };

        enum TypPakietu : quint8 {
                PAKIET_KONFIG = 1,
                PAKIET_PROBKA = 2,
                PAKIET_TAKT_START = 3,
                PAKIET_TAKT_STOP = 4,
                PAKIET_TAKT_INTERWAL = 5
        };

        // Numer sekwencyjny dla próbek (do śledzenia opóźnień)
        quint64 numerSeq() const { return m_numerSeq; }
        void setNumerSeq(quint64 numer) { m_numerSeq = numer; }
        quint64 nastepnyNumerSeq() { return ++m_numerSeq; }

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
        void wyslijProbkeBinarnie(const ProbkaDanych &probka);
        void wyslijProbkeTekstowo(const ProbkaDanych &probka);

        // Pakiety sterujące taktowaniem
        void wyslijTaktStart();
        void wyslijTaktStop();
        void wyslijTaktInterwal(int interwalMs);

        
    public:
        void setTimeoutJednostronny(int timeoutMs);
        void setTrybJednostronny(bool jednostronny);

    signals:
        void polaczono();
        void rozlaczono();
        void utraconoPolaczenie();
        void statusZmieniony(const QString &opis);
        void odebranoKonfiguracje(const QJsonObject &konfig);
        void odebranoProbkeBinarna(const ProbkaDanych &probka);
        void odebranoProbkeTekstowa(const ProbkaDanych &probka);
        void odebranoTaktStart();
        void odebranoTaktStop();
        void odebranoTaktInterwal(int interwalMs);

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

        // Taktowanie jednostronne
        bool m_trybJednostronny = false;
        int m_timeoutJednostronny = 2000; // ms
        QTimer *m_timerTimeout = nullptr;
        QTimer *m_timerOdswiezania = nullptr;

        // Numer sekwencyjny próbek
        quint64 m_numerSeq = 0;
        quint64 m_ostatniOdebranyNumer = 0;
        bool m_czekamNaSterowanie = false;

        void podlaczSocket();
        void czyscPolaczenie();
        void parsujBufor();
        void wyslijPakiet(TypPakietu typ, const QByteArray &payload);
        void resetujTimeout();
        void sprawdzTimeout();
};

#endif // SIECI_H
