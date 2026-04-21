#include "ProstyUAR.h"

ProstyUAR::ProstyUAR(ModelARX &arx, RegulatorPID &pid, Generator &gen, QObject *parent)
    : QObject(parent)
    , model(arx)
    , regulator(pid)
    , generator(gen)
    , m_timer(new QTimer(this))
    , m_interwalMs(100) // Bezpieczna wartość domyślna
    , m_aktualnyCzas(0.0)
    , m_ostatnieWyjscie(0.0)
{
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &ProstyUAR::onTimeout);
}

void ProstyUAR::onTimeout()
{
    double dt = m_interwalMs / 1000.0;
    m_aktualnyCzas += dt;

    // Pobranie wartości zadanej z generatora
    double w = generator.generuj(m_aktualnyCzas);

    // Obliczenie obiektu i regulatora
    double y = symuluj(w, dt);

    // Uchyb
    double e = w - y;

    // Suma składowych z PID
    double u = regulator.pobierzOstatnieP()
               + regulator.pobierzOstatnieI()
               + regulator.pobierzOstatnieD();

    emit krokWykonany(m_aktualnyCzas, w, y, e, u);
}

double ProstyUAR::symuluj(double wartosc_zadana, double dt)
{
    // Obliczanie uchybu
    double e = wartosc_zadana - m_ostatnieWyjscie;

    // Obliczanie sterowanie z regulatora
    double u = regulator.symuluj(e, dt);
    double y = model.symuluj(u);
    m_ostatnieWyjscie = y;

    return y;
}

void ProstyUAR::start()
{
    m_timer->start(m_interwalMs);
}

void ProstyUAR::stop()
{
    m_timer->stop();
}

void ProstyUAR::reset()
{
    m_timer->stop();
    m_aktualnyCzas    = 0.0;
    m_ostatnieWyjscie = 0.0;
}

void ProstyUAR::setInterwal(int ms)
{
    m_interwalMs = ms;
    m_timer->setInterval(ms);
}
