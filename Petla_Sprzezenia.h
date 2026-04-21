/*#ifndef PETLA_SPRZEZENIA_H
#define PETLA_SPRZEZENIA_H

#include "Generator.h"
#include "ModelARX.h"
#include "RegulatorPID.h"
#include <QObject>
#include <QTimer>

class Petla_Sprzezenia : public QObject
{
    Q_OBJECT

public:
    explicit Petla_Sprzezenia(ModelARX &arx, RegulatorPID &pid, Generator &gen,
                              QObject *parent = nullptr);

    // Sterowanie timerem
    void start();
    void stop();
    void reset();
    void setInterwal(int ms);

    // Dostęp do stanu
    double getAktualnyCzas() const { return m_aktualnyCzas; }
    int    getInterwal()     const { return m_interwalMs; }

    // Pojedynczy krok — nadal publiczny, używany przez ProstyUAR i testy
    double wykonaj_krok(double wartosc_zadana, double dt);

signals:
    // Emitowany po każdym kroku — MainWindow podpina się tutaj zamiast mieć własny timer
    void krokWykonany(double czas, double w, double y, double e, double u);

private slots:
    void onTimeout();

private:
    ModelARX     &model;
    RegulatorPID &regulator;
    Generator    &generator;

    QTimer *m_timer;
    double  m_aktualnyCzas    = 0.0;
    int     m_interwalMs      = 200;
    double  m_ostatnieWyjscie = 0.0;
};

#endif // PETLA_SPRZEZENIA_H*/
