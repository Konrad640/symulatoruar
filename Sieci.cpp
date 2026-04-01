#include "Sieci.h"

Sieci::Sieci(QObject *parent)
    : QObject(parent) { }

void Sieci::start(quint16 port, QString host) {
        this->port = port;
        this->host = host;
}

void Sieci::wyslijWiadomosc(const QString &msg, int number) {
        if (!socket)
                return;

        // QByteArray block;
        // QDataStream out(&block, QIODevice::WriteOnly);
        // out << msg << number;

        // socket->write(block);

        QByteArray data;
        QDataStream out(&data, QIODevice::WriteOnly);
        out << msg << number;

        QByteArray block;
        QDataStream out2(&block, QIODevice::WriteOnly);
        out2 << (quint32)data.size();
        block.append(data);

        socket->write(block);
}

void Sieci::setupRegulator() {
        connect(&server, &QTcpServer::newConnection, this, &Sieci::onNewConnection);
}

void Sieci::onNewConnection() {
        socket = server.nextPendingConnection();

        connect(socket, &QTcpSocket::readyRead, this, &Sieci::onReadyRead);

        connect(socket, &QTcpSocket::disconnected, this, [=]() {
                socket->deleteLater();
                socket = nullptr;
        });

        qDebug() << "Obiekt polaczony";
}

void Sieci::setupObiekt() {
        socket = new QTcpSocket(this);

        connect(socket, &QTcpSocket::connected, this, [=]() {
                qDebug() << "Polaczono z regulatorem";
        });

        connect(socket, &QTcpSocket::readyRead, this, &Sieci::onReadyRead);
}

void Sieci::czyscPolaczenie() {
        if (socket) {
                socket->disconnect();
                socket->disconnectFromHost();
                socket->deleteLater();
                socket = nullptr;
        }

        server.disconnect();

        if (server.isListening()) {
                server.close();
        }
}

void Sieci::onReadyRead() {
        // QDataStream in(socket);

        // while (!in.atEnd()) {
        //         QString msg;
        //         int number;
        //         in >> msg >> number;

        //         qDebug() << "Odebrano:" << msg << number;
        // }
        QByteArray buffer;
        buffer.append(socket->readAll());

        QDataStream in(&buffer, QIODevice::ReadOnly);

        while (true) {
                if (buffer.size() < sizeof(quint32))
                        return;

                quint32 size;
                in >> size;

                if (buffer.size() < sizeof(quint32) + size)
                        return;

                QString msg;
                int number;
                in >> msg >> number;

                qDebug() << "Odebrano:" << msg << number;

                buffer.remove(0, sizeof(quint32) + size);
                in.device()->seek(0);
        }
}

void Sieci::set_tryb(Tryb tryb) {
        if (m_tryb == tryb)
                return;

        czyscPolaczenie();

        m_tryb = tryb;

        if (m_tryb == Tryb::Regulator) {
                setupRegulator();
                server.listen(QHostAddress::Any, port);
                qDebug() << "Tryb: REGULATOR";
        }
        else if (m_tryb == Tryb::Obiekt) {
                setupObiekt();
                socket->connectToHost(host, port);
                qDebug() << "Tryb: OBIEKT";
        }

}
