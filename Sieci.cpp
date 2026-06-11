#include "Sieci.h"

#include <QCborValue>
#include <QDataStream>

namespace {
const QString FORMAT_PROBKI = "UAR_PROBKA_BIN_1";

QJsonObject deserializujKonfiguracje(const QByteArray &payload)
{
        QJsonDocument doc = QJsonDocument::fromJson(payload);
        if (doc.isObject())
                return doc.object();

        QCborParserError blad;
        const QCborValue wartosc = QCborValue::fromCbor(payload, &blad);
        if (blad.error == QCborError::NoError && wartosc.isMap())
                return wartosc.toJsonValue().toObject();

        return QJsonObject();
}

double pobierzDouble(const QJsonObject &obj, const QString &klucz)
{
        return obj.value(klucz).toDouble();
}

int pobierzInt(const QJsonObject &obj, const QString &klucz)
{
        return obj.value(klucz).toInt();
}

qint64 pobierzInt64(const QJsonObject &obj, const QString &klucz)
{
        return obj.value(klucz).toVariant().toLongLong();
}

QByteArray serializujProbkeBinarnie(const QJsonObject &probka)
{
        QByteArray payload;
        QDataStream out(&payload, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_15);

        out << FORMAT_PROBKI;
        out << probka.value("rodzaj").toString();
        out << probka.value("komenda").toString();
        out << static_cast<qint32>(pobierzInt(probka, "seq"));
        out << pobierzDouble(probka, "t");
        out << pobierzDouble(probka, "dt");
        out << pobierzDouble(probka, "w");
        out << pobierzDouble(probka, "y");
        out << pobierzDouble(probka, "y_poprzednie");
        out << pobierzDouble(probka, "e");
        out << pobierzDouble(probka, "u");
        out << pobierzDouble(probka, "P");
        out << pobierzDouble(probka, "I");
        out << pobierzDouble(probka, "D");
        out << pobierzDouble(probka, "shadow_y");
        out << pobierzDouble(probka, "shadow_u");
        out << static_cast<qint32>(pobierzInt(probka, "taktowanie"));
        out << pobierzInt64(probka, "nadano_ms");
        out << probka.value("brak_nowego_u").toBool();

        return payload;
}

QJsonObject deserializujProbkeBinarnie(const QByteArray &payload)
{
        QJsonObject probka;
        QDataStream in(payload);
        in.setVersion(QDataStream::Qt_5_15);

        QString format;
        QString rodzaj;
        QString komenda;
        qint32 seq = 0;
        qint32 taktowanie = 0;
        qint64 nadanoMs = 0;
        bool brakNowegoU = false;
        double t = 0.0;
        double dt = 0.0;
        double w = 0.0;
        double y = 0.0;
        double yPoprzednie = 0.0;
        double e = 0.0;
        double u = 0.0;
        double p = 0.0;
        double i = 0.0;
        double d = 0.0;
        double shadowY = 0.0;
        double shadowU = 0.0;

        in >> format;
        if (format != FORMAT_PROBKI || in.status() != QDataStream::Ok)
                return probka;

        in >> rodzaj >> komenda >> seq >> t >> dt >> w >> y >> yPoprzednie >> e >> u >> p >> i
           >> d >> shadowY >> shadowU >> taktowanie >> nadanoMs >> brakNowegoU;

        if (in.status() != QDataStream::Ok)
                return QJsonObject();

        probka["rodzaj"] = rodzaj;
        probka["komenda"] = komenda;
        probka["seq"] = seq;
        probka["t"] = t;
        probka["dt"] = dt;
        probka["w"] = w;
        probka["y"] = y;
        probka["y_poprzednie"] = yPoprzednie;
        probka["e"] = e;
        probka["u"] = u;
        probka["P"] = p;
        probka["I"] = i;
        probka["D"] = d;
        probka["shadow_y"] = shadowY;
        probka["shadow_u"] = shadowU;
        probka["taktowanie"] = taktowanie;
        probka["nadano_ms"] = QString::number(nadanoMs);
        probka["brak_nowego_u"] = brakNowegoU;

        return probka;
}
}

Sieci::Sieci(QObject *parent)
    : QObject(parent)
{
        connect(&m_server, &QTcpServer::newConnection, this, &Sieci::onNewConnection);
}

Sieci::~Sieci()
{
        czyscPolaczenie();
}

void Sieci::ustawParametry(quint16 port, const QString &host)
{
        m_port = port;
        m_host = host;
}

bool Sieci::czyPolaczony() const
{
        return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

QString Sieci::opisPartnera() const
{
        if (!czyPolaczony())
                return "brak";

        return QString("%1:%2").arg(m_socket->peerAddress().toString()).arg(m_socket->peerPort());
}

void Sieci::set_tryb(Tryb tryb)
{
        czyscPolaczenie();
        m_tryb = tryb;

        if (m_tryb == Regulator) {
                if (!m_server.listen(QHostAddress::Any, m_port)) {
                        qDebug() << "Sieci: nie można nasłuchiwać:" << m_server.errorString();
                        emit statusZmieniony(QString("Błąd: %1").arg(m_server.errorString()));
                        m_tryb = Nieokreslony;
                        return;
                }
                qDebug() << "Sieci: tryb REGULATOR, nasłuchiwanie na porcie" << m_port;
                emit statusZmieniony(QString("Regulator: nasłuchiwanie na porcie %1…").arg(m_port));
        }
        else if (m_tryb == Obiekt) {
                m_socket = new QTcpSocket(this);
                podlaczSocket();
                qDebug() << "Sieci: tryb OBIEKT, łączenie z" << m_host << ":" << m_port;
                emit statusZmieniony(QString("Obiekt: łączenie z %1:%2…").arg(m_host).arg(m_port));
                m_socket->connectToHost(m_host, m_port);
        }
        else {
                emit statusZmieniony("Rozłączony");
        }
}

void Sieci::rozlacz()
{
        zakonczPolaczenie("Rozłączony");
}

void Sieci::onNewConnection()
{
        QTcpSocket *nowy = m_server.nextPendingConnection();
        if (!nowy)
                return;

        if (m_socket) {
                nowy->disconnectFromHost();
                nowy->deleteLater();
                return;
        }

        m_socket = nowy;
        podlaczSocket();
        ustawOpcjeSocketu();

        qDebug() << "Sieci: podłączył się obiekt";
        emit statusZmieniony("Regulator: połączono z obiektem");
        emit polaczono();
}

void Sieci::podlaczSocket()
{
        if (!m_socket)
                return;

        connect(m_socket, &QTcpSocket::connected, this, &Sieci::onConnected);
        connect(m_socket, &QTcpSocket::disconnected, this, &Sieci::onDisconnected);
        connect(m_socket, &QTcpSocket::readyRead, this, &Sieci::onReadyRead);
        connect(m_socket,
                QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
                this,
                &Sieci::onError);
}

void Sieci::ustawOpcjeSocketu()
{
        if (!m_socket)
                return;

        m_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
        m_socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
}

void Sieci::onConnected()
{
        ustawOpcjeSocketu();
        qDebug() << "Sieci: połączono z regulatorem";
        emit statusZmieniony("Obiekt: połączono z regulatorem");
        emit polaczono();
}

void Sieci::onDisconnected()
{
        qDebug() << "Sieci: rozłączono";
        zakonczPolaczenie("Rozłączono");
}

void Sieci::onError(QAbstractSocket::SocketError)
{
        const QString opis = m_socket ? m_socket->errorString() : QString("nieznany błąd");
        qDebug() << "Sieci: błąd -" << opis;

        zakonczPolaczenie(QString("Błąd sieci: %1").arg(opis));
}

void Sieci::zakonczPolaczenie(const QString &opis)
{
        czyscPolaczenie();
        m_tryb = Nieokreslony;
        emit statusZmieniony(opis);
        emit rozlaczono();
}

void Sieci::czyscPolaczenie()
{
        if (m_socket) {
                m_socket->disconnect(this);
                m_socket->abort();
                m_socket->deleteLater();
                m_socket = nullptr;
        }
        if (m_server.isListening()) {
                m_server.close();
        }
        m_bufor.clear();
}

void Sieci::wyslijKonfiguracje(const QJsonObject &konfig)
{
        if (!czyPolaczony())
                return;

        if (m_serializacja == SER_BINARNA) {
                wyslijPakiet(PAKIET_KONFIG, QCborValue::fromJsonValue(konfig).toCbor());
        } else {
                QJsonDocument doc(konfig);
                wyslijPakiet(PAKIET_KONFIG, doc.toJson(QJsonDocument::Compact));
        }
}

void Sieci::wyslijProbke(const QJsonObject &probka)
{
        if (!czyPolaczony())
                return;

        if (m_serializacja == SER_BINARNA) {
                wyslijPakiet(PAKIET_PROBKA, serializujProbkeBinarnie(probka));
        } else {
                QJsonDocument doc(probka);
                wyslijPakiet(PAKIET_PROBKA, doc.toJson(QJsonDocument::Compact));
        }
}

void Sieci::wyslijPakiet(TypPakietu typ, const QByteArray &payload)
{
        if (!czyPolaczony())
                return;

        QByteArray naglowek;
        QDataStream out(&naglowek, QIODevice::WriteOnly);

        out.setVersion(QDataStream::Qt_5_15);
        out << static_cast<quint8>(typ);
        out << static_cast<quint32>(payload.size());

        m_socket->write(naglowek);
        m_socket->write(payload);
        m_socket->flush();
}

void Sieci::onReadyRead()
{
        if (!m_socket)
                return;

        m_bufor.append(m_socket->readAll());
        parsujBufor();
}

void Sieci::parsujBufor()
{
        const int ROZMIAR_NAGLOWKA = sizeof(quint8) + sizeof(quint32);

        while (m_bufor.size() >= ROZMIAR_NAGLOWKA) {
                QDataStream in(m_bufor);
                in.setVersion(QDataStream::Qt_5_15);

                quint8 typ;
                quint32 rozmiar;
                in >> typ >> rozmiar;

                if (m_bufor.size() < ROZMIAR_NAGLOWKA + (int) rozmiar) {
                        return;
                }

                QByteArray payload = m_bufor.mid(ROZMIAR_NAGLOWKA, rozmiar);
                m_bufor.remove(0, ROZMIAR_NAGLOWKA + rozmiar);

                if (typ == PAKIET_KONFIG) {
                        QJsonObject konfig = deserializujKonfiguracje(payload);
                        if (!konfig.isEmpty()) {
                                qDebug() << "Sieci: odebrano konfigurację";
                                emit odebranoKonfiguracje(konfig);
                        }
                } else if (typ == PAKIET_PROBKA) {
                        QJsonObject probka = deserializujProbkeBinarnie(payload);
                        if (probka.isEmpty()) {
                                QJsonDocument doc = QJsonDocument::fromJson(payload);
                                if (doc.isObject())
                                        probka = doc.object();
                        }
                        if (!probka.isEmpty()) {
                                emit odebranoProbke(probka);
                        }
                }
        }
}
