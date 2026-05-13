#include "Sieci.h"
#include <QTimer>

Sieci::Sieci(QObject *parent)
    : QObject(parent)
    , m_timerTimeout(new QTimer(this))
    , m_timerOdswiezania(new QTimer(this))
{
        connect(&m_server, &QTcpServer::newConnection, this, &Sieci::onNewConnection);

        m_timerTimeout->setSingleShot(true);
        connect(m_timerTimeout, &QTimer::timeout, this, &Sieci::sprawdzTimeout);

        m_timerOdswiezania->setInterval(500);
        connect(m_timerOdswiezania, &QTimer::timeout, this, &Sieci::resetujTimeout);
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

void Sieci::wyslijProbkeBinarnie(const ProbkaDanych &probka)
{
        if (!czyPolaczony())
                return;

        wyslijPakiet(PAKIET_PROBKA, probka.toBinary());
}

void Sieci::wyslijProbkeTekstowo(const ProbkaDanych &probka)
{
        if (!czyPolaczony())
                return;

        QJsonDocument doc(probka.toJson());
        wyslijPakiet(PAKIET_PROBKA, doc.toJson(QJsonDocument::Compact));
}

void Sieci::setTimeoutJednostronny(int timeoutMs)
{
        m_timeoutJednostronny = timeoutMs;
}

void Sieci::setTrybJednostronny(bool jednostronny)
{
        m_trybJednostronny = jednostronny;

        if (m_trybJednostronny) {
                m_timerOdswiezania->start();
        }
        else {
                m_timerOdswiezania->stop();
                m_timerTimeout->stop();
        }
}

void Sieci::resetujTimeout()
{
        if (m_timerTimeout->isActive()) {
                m_timerTimeout->stop();
        }
        m_timerTimeout->start(m_timeoutJednostronny);
}

void Sieci::sprawdzTimeout()
{
        if (!m_trybJednostronny)
                return;

        // W trybie jednostronnym - sprawdź czy czekamy na sterowanie
        if (m_czekamNaSterowanie) {
                qDebug() << "Sieci: timeout - brak sterowania od obiektu";
                emit statusZmieniony("Brak sterowania - utracono polaczenie");
                emit utraconoPolaczenie();
        }
}

void Sieci::wyslijTaktStart()
{
        if (!czyPolaczony())
                return;

        wyslijPakiet(PAKIET_TAKT_START, QByteArray());
        qDebug() << "Sieci: wysłano TAKT_START";
}

void Sieci::wyslijTaktStop()
{
        if (!czyPolaczony())
                return;

        wyslijPakiet(PAKIET_TAKT_STOP, QByteArray());
        qDebug() << "Sieci: wysłano TAKT_STOP";
}

void Sieci::wyslijTaktInterwal(int interwalMs)
{
        if (!czyPolaczony())
                return;

        QByteArray dane;
        QDataStream out(&dane, QIODevice::WriteOnly);
        out << interwalMs;
        wyslijPakiet(PAKIET_TAKT_INTERWAL, dane);
        qDebug() << "Sieci: wysłano TAKT_INTERWAL:" << interwalMs;
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
                        QJsonDocument doc = QJsonDocument::fromJson(payload);
                        if (doc.isObject()) {
                                qDebug() << "Sieci: odebrano konfigurację";
                                emit odebranoKonfiguracje(doc.object());
                        }
                }
                else if (typ == PAKIET_PROBKA) {
                        // Próbuj najpierw jako JSON (tekstowy)
                        QJsonDocument doc = QJsonDocument::fromJson(payload);
                        if (doc.isObject()) {
                                ProbkaDanych p = ProbkaDanych::fromJson(doc.object());
                                qDebug() << "Sieci: odebrano próbkę tekstową n=" << p.numerSeq;
                                emit odebranoProbkeTekstowa(p);

                                // Resetuj flagę czekania na sterowanie
                                m_czekamNaSterowanie = false;
                        }
                        else {
                                // Spróbuj jako format binarny
                                ProbkaDanych p = ProbkaDanych::fromBinary(payload);
                                qDebug() << "Sieci: odebrano próbkę binarną n=" << p.numerSeq;
                                emit odebranoProbkeBinarna(p);
                                m_czekamNaSterowanie = false;
                        }
                }
                else if (typ == PAKIET_TAKT_START) {
                        qDebug() << "Sieci: odebrano TAKT_START";
                        emit odebranoTaktStart();
                }
                else if (typ == PAKIET_TAKT_STOP) {
                        qDebug() << "Sieci: odebrano TAKT_STOP";
                        emit odebranoTaktStop();
                }
                else if (typ == PAKIET_TAKT_INTERWAL) {
                        QDataStream in(payload);
                        int interwal;
                        in >> interwal;
                        qDebug() << "Sieci: odebrano TAKT_INTERWAL:" << interwal;
                        emit odebranoTaktInterwal(interwal);
                }

                // Resetuj timeout przy odbiorze czegokolwiek w trybie jednostronnym
                if (m_trybJednostronny) {
                        resetujTimeout();
                }
        }
}
