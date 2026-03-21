#ifndef PETLA_SPRZEZENIA_H
#define PETLA_SPRZEZENIA_H

#include "ModelARX.h"
#include "RegulatorPID.h"

class Petla_Sprzezenia
{
public:
    Petla_Sprzezenia(ModelARX &arx, RegulatorPID &pid);
    double wykonaj_krok(double wartosc_zadana, double dt);

private:
    ModelARX &model;
    RegulatorPID &regulator;
    double ostatnie_wyjscie;
};
#endif
