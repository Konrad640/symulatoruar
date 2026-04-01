#include "RegulatorPID.h"

RegulatorPID::RegulatorPID(double k, double ti, double td)
    : Kp(k)
    , Ti(ti)
    , Td(td)
    , metoda(MetodaCalkowania::STALA_W_SUMIE)
{
    zresetuj();
}

void RegulatorPID::zresetuj()
{
    suma_uchybow = 0.0;
    calka_wewnetrzna = 0.0;
    poprzedni_uchyb = 0.0;
    ostatnie_P = 0.0;
    ostatnie_I = 0.0;
    ostatnie_D = 0.0;
}

void RegulatorPID::setMetodaCalkowania(MetodaCalkowania m)
{
    if (m == metoda)
        return;

    if (m == MetodaCalkowania::STALA_W_SUMIE) {
        if (Ti != 0)
            calka_wewnetrzna = suma_uchybow * (1.0 / Ti);
        else
            calka_wewnetrzna = 0.0;
    } else {
        if (Ti != 0)
            suma_uchybow = calka_wewnetrzna * Ti;
        else
            suma_uchybow = 0.0;
    }
    metoda = m;
}

RegulatorPID::MetodaCalkowania RegulatorPID::getMetodaCalkowania() const
{
    return metoda;
}

double RegulatorPID::symuluj(double e, double dt) {
        // P
        double P = Kp * e;
        ostatnie_P = P;

        // I
        double I = 0.0;
        if (Ti == 0.0)
                I = 0.0;
        else {
                if (metoda == MetodaCalkowania::STALA_PRZED_SUMA) {
                        suma_uchybow += e;
                        I = suma_uchybow / Ti;
                } else {
                        suma_uchybow += e / Ti;
                        I = suma_uchybow;
                }
        }
        ostatnie_I = I;

        // D
        double D =Td * (e - poprzedni_uchyb);
        ostatnie_D = D;

        poprzedni_uchyb = e;

        return P + I + D;
}

void RegulatorPID::setKp(double k)
{
    Kp = k;
}
double RegulatorPID::getKp() const
{
    return Kp;
}

void RegulatorPID::setTi(double ti)
{
    Ti = ti;
}
double RegulatorPID::getTi() const
{
    return Ti;
}

void RegulatorPID::setTd(double td)
{
    Td = td;
}
double RegulatorPID::getTd() const
{
    return Td;
}

void RegulatorPID::setLiczCalk(LiczCalk m){
        if(metoda == MetodaCalkowania::STALA_PRZED_SUMA && m == LiczCalk::Wew)
                suma_uchybow /= Ti;
        if(metoda == MetodaCalkowania::STALA_W_SUMIE && m == LiczCalk::Zew)
                suma_uchybow *= Ti;

        if (m == LiczCalk::Wew)
                setMetodaCalkowania(MetodaCalkowania::STALA_W_SUMIE);
        else
                setMetodaCalkowania(MetodaCalkowania::STALA_PRZED_SUMA);
}

void RegulatorPID::setStalaCalk(double ti)
{
    setTi(ti);
}

double RegulatorPID::symuluj(double e)
{
    // Testy zakładają symulację krok po kroku bez podawania czasu,
    // przyjmujemy dt = 1.0
    return symuluj(e, 1.0);
}
