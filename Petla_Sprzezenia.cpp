#include "Petla_Sprzezenia.h"

Petla_Sprzezenia::Petla_Sprzezenia(ModelARX &arx, RegulatorPID &pid)
    : model(arx)
    , regulator(pid)
    , ostatnie_wyjscie(0.0)
{}

double Petla_Sprzezenia::wykonaj_krok(double wartosc_zadana, double dt)
{
    double e = wartosc_zadana - ostatnie_wyjscie;
    double u = regulator.symuluj(e, dt);
    double y = model.symuluj(u);
    ostatnie_wyjscie = y;
    return y;
}
