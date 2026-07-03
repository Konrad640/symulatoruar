#include "RegulatorPID.h"

RegulatorPID::RegulatorPID(double k, double ti, double td) : Kp(k), Ti(ti), Td(td), metoda(MetodaCalkowania::STALA_W_SUMIE) {
        zresetuj();
}

void RegulatorPID::zresetuj() {
        suma_uchybow = 0.0;
        poprzedni_uchyb = 0.0;
        ostatnie_P = 0.0;
        ostatnie_I = 0.0;
        ostatnie_D = 0.0;
}
void RegulatorPID::zresetujCalke() {
        suma_uchybow = 0.0;
        ostatnie_I = 0.0;
}

void RegulatorPID::setMetodaCalkowania(MetodaCalkowania m) {
        if (m == metoda)
                return;

        if (m == MetodaCalkowania::STALA_W_SUMIE) {
                if (Ti != 0)
                        suma_uchybow /= Ti;
                else
                        suma_uchybow = 0.0;
        } else
                suma_uchybow *= Ti;

        metoda = m;
}

RegulatorPID::MetodaCalkowania RegulatorPID::getMetodaCalkowania() const {
        return metoda;
}

double RegulatorPID::symuluj(double e) {
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
        double D = Td * (e - poprzedni_uchyb);
        ostatnie_D = D;

        poprzedni_uchyb = e;

        return P + I + D;
}

void RegulatorPID::setKp(double k) {
        Kp = k;
}

void RegulatorPID::setTi(double ti) {
        Ti = ti;
}

void RegulatorPID::setTd(double td) {
        Td = td;
}
