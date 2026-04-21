#ifndef PROSTYUAR_H
#define PROSTYUAR_H

#include <QObject>
#include <QTimer>
#include "ModelARX.h"
#include "RegulatorPID.h"
#include "Generator.h"

class ProstyUAR : public QObject
{
    Q_OBJECT

public:
    explicit ProstyUAR(ModelARX &arx, RegulatorPID &pid, Generator &gen, QObject *parent = nullptr);

    void start();
    void stop();
    void reset();
    void setInterwal(int ms);
    int getInterwal() const { return m_interwalMs; }

    double symuluj(double wartosc_zadana, double dt = 1.0);

signals:
    void krokWykonany(double t, double w, double y, double e, double u);

private slots:
    void onTimeout();

private:
    ModelARX &model;
    RegulatorPID &regulator;
    Generator &generator;

    QTimer *m_timer;
    int m_interwalMs = 100;
    double m_aktualnyCzas = 0.0;
    double m_ostatnieWyjscie = 0.0;
};

#endif // PROSTYUAR_H
