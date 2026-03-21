#include "Generator.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Generator::Generator()
    : typ_sygnalu(TypSygnalu::PROSTOKATNY)
    , wartosc_poczatkowa(0)
    , wartosc_zmiany(1)
    , czas_zmiany(10)
    , wypelnienie(0.5)
    , czas_aktywacji(1.0)
{}

Generator::Generator(
    TypSygnalu typ, double poczatkowa, double zmiana, double czas_zmiany, double wyp, double t_akt)
    : typ_sygnalu(typ)
    , wartosc_poczatkowa(poczatkowa)
    , wartosc_zmiany(zmiana)
    , czas_zmiany(czas_zmiany)
    , wypelnienie(wyp)
    , czas_aktywacji(t_akt)
{}

void Generator::ustawParametry(
    TypSygnalu typ, double pocz, double zm, double czas, double wyp, double t_akt)
{
    typ_sygnalu = typ;
    wartosc_poczatkowa = pocz;
    wartosc_zmiany = zm;
    czas_zmiany = czas;
    wypelnienie = wyp;
    czas_aktywacji = t_akt;

    if (czas_zmiany <= 0.001)
        czas_zmiany = 0.001;
    if (wypelnienie < 0.0)
        wypelnienie = 0.0;
    if (wypelnienie > 1.0)
        wypelnienie = 1.0;
}

double Generator::generuj(double aktualny_czas) const
{
    switch (typ_sygnalu) {
    case TypSygnalu::SKOK: {
        if (aktualny_czas >= czas_aktywacji) {
            return wartosc_poczatkowa + wartosc_zmiany;
        }
        return wartosc_poczatkowa;
    }

    case TypSygnalu::PROSTOKATNY: {
        const double okres = (czas_zmiany > 0) ? czas_zmiany : 1.0;
        const double czas_w_okresie = fmod(aktualny_czas, okres);

        if (czas_w_okresie < okres * wypelnienie) {
            return wartosc_poczatkowa + wartosc_zmiany;
        } else {
            return wartosc_poczatkowa;
        }
    }

    case TypSygnalu::SINUSOIDALNY: {
        const double okres = (czas_zmiany > 0) ? czas_zmiany : 1.0;
        const double czestosc_katowa = 2.0 * M_PI / okres;
        return wartosc_poczatkowa + wartosc_zmiany * sin(czestosc_katowa * aktualny_czas);
    }

    default:
        return wartosc_poczatkowa;
    }
}
