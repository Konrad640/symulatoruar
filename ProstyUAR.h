#ifndef PROSTYUAR_H
#define PROSTYUAR_H

#include "ModelARX.h"

class RegulatorPID;

class ProstyUAR
{
public:
    ProstyUAR(ModelARX &arx, RegulatorPID &pid);

    // Główna funkcja symulacji pętli zamkniętej
    // w - wartość zadana
    // Zwraca y - wyjście z obiektu
    double symuluj(double w);

private:
    ModelARX &model;
    RegulatorPID &regulator;
    double poprzednie_y; // Do obliczenia uchybu e = w - y(k-1)
};

#endif // PROSTYUAR_H
