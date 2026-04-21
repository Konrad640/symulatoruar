/*#include "Petla_Sprzezenia.h"

Petla_Sprzezenia::Petla_Sprzezenia(ModelARX &arx, RegulatorPID &pid, Generator &gen,
                                   QObject *parent)
    : QObject(parent)
    , model(arx)
    , regulator(pid)
    , generator(gen)
    , m_timer(new QTimer(this))
{
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &Petla_Sprzezenia::onTimeout);
}

void Petla_Sprzezenia::onTimeout()
{
    double dt = m_interwalMs / 1000.0;
    m_aktualnyCzas += dt;

    double w = generator.generuj(m_aktualnyCzas);
    double y = wykonaj_krok(w, dt);
    double e = w - y;
    double u = regulator.pobierzOstatnieP()
               + regulator.pobierzOstatnieI()
               + regulator.pobierzOstatnieD();

    emit krokWykonany(m_aktualnyCzas, w, y, e, u);
}

double Petla_Sprzezenia::wykonaj_krok(double wartosc_zadana, double dt)
{
    double e = wartosc_zadana - m_ostatnieWyjscie;
    double u = regulator.symuluj(e, dt);
    double y = model.symuluj(u);
    m_ostatnieWyjscie = y;
    return y;
}

void Petla_Sprzezenia::start()
{
    m_timer->start(m_interwalMs);
}

void Petla_Sprzezenia::stop()
{
    m_timer->stop();
}

void Petla_Sprzezenia::reset()
{
    m_timer->stop();
    m_aktualnyCzas    = 0.0;
    m_ostatnieWyjscie = 0.0;
}

void Petla_Sprzezenia::setInterwal(int ms)
{
    m_interwalMs = ms;
    m_timer->setInterval(ms);
}*/
