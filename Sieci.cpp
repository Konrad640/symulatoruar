#include "Sieci.h"

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

void Sieci::set_tryb(Tryb tryb)
{
        // Zawsze zaczynamy od czystego stanu - unika przecieków
        // przy ponownym kliknięciu tego samego trybu.
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
        czyscPolaczenie();
        m_tryb = Nieokreslony;
        emit statusZmieniony("Rozłączony");
        emit rozlaczono();
}

void Sieci::onNewConnection()
{
        QTcpSocket *nowy = m_server.nextPendingConnection();
        if (!nowy)
                return;

        // Tylko jedno połączenie na raz - odrzucamy dodatkowych klientów.
        if (m_socket) {
                nowy->disconnectFromHost();
                nowy->deleteLater();
                return;
        }

        m_socket = nowy;
        podlaczSocket();

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

void Sieci::onConnected()
{
        qDebug() << "Sieci: połączono z regulatorem";
        emit statusZmieniony("Obiekt: połączono z regulatorem");
        emit polaczono();
}

void Sieci::onDisconnected()
{
        qDebug() << "Sieci: rozłączono";
        emit statusZmieniony("Rozłączono");
        emit rozlaczono();

        if (m_socket) {
                m_socket->deleteLater();
                m_socket = nullptr;
        }
        m_bufor.clear();
}

void Sieci::onError(QAbstractSocket::SocketError)
{
        if (m_socket) {
                qDebug() << "Sieci: błąd -" << m_socket->errorString();
                emit statusZmieniony(QString("Błąd sieci: %1").arg(m_socket->errorString()));
        }
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

        QJsonDocument doc(konfig);
        wyslijPakiet(PAKIET_KONFIG, doc.toJson(QJsonDocument::Compact));
}

void Sieci::wyslijPakiet(TypPakietu typ, const QByteArray &payload)
{
        if (!czyPolaczony())
                return;

        QByteArray naglowek;
        QDataStream out(&naglowek, QIODevice::WriteOnly);
        // NAPRAWIONE: jawnie ustawiona wersja QDataStream - gwarantuje zgodność
        // formatu między instancjami niezależnie od wersji Qt.
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
                        // Niepełny pakiet - czekamy na kolejne readyRead
                        return;
                }

                QByteArray payload = m_bufor.mid(ROZMIAR_NAGLOWKA, rozmiar);
                m_bufor.remove(0, ROZMIAR_NAGLOWKA + rozmiar);

                if (typ == PAKIET_KONFIG) {
                        QJsonDocument doc = QJsonDocument::fromJson(payload);
                        if (doc.isObject()) {
                                qDebug() << "Sieci: odebrano konfigurację";
                                emit odebranoKonfiguracje(doc.object());
                        }
                }
                // PAKIET_PROBKA - obsługa w kolejnym etapie (symulacja sieciowa)
        }
}
