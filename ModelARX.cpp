#include "ModelARX.h"

ModelARX::ModelARX(const std::vector<double> &a, const std::vector<double> &b, int k, double stddev)
    : wspolczynniki_A(a)
    , wspolczynniki_B(b)
    , opoznienie(k)
    , stddev_szumu(stddev)
    , gen(std::random_device{}())
    , dist(0.0, 1.0)
{
    zresetuj_stan();
}

void ModelARX::zresetuj_stan()
{
    bufor_opoznienia.clear();
    bufor_opoznienia.resize(opoznienie, 0.0);

    historia_Y.assign(wspolczynniki_A.size(), 0.0);
    historia_U.assign(wspolczynniki_B.size(), 0.0);
}

void ModelARX::setA(const std::vector<double> &a)
{
    if (a == wspolczynniki_A)
        return;

    // Przepisanie starej historii do nowego wektora (zapobiega skokom/resetowi)
    std::vector<double> nowaHistoria(a.size(), 0.0);
    size_t doSkopiowania = std::min(historia_Y.size(), nowaHistoria.size());
    for (size_t i = 0; i < doSkopiowania; ++i)
        nowaHistoria[i] = historia_Y[i];

    historia_Y = nowaHistoria;
    wspolczynniki_A = a;
}

void ModelARX::setB(const std::vector<double> &b)
{
    if (b == wspolczynniki_B)
        return;

    std::vector<double> nowaHistoria(b.size(), 0.0);
    size_t doSkopiowania = std::min(historia_U.size(), nowaHistoria.size());
    for (size_t i = 0; i < doSkopiowania; ++i)
        nowaHistoria[i] = historia_U[i];

    historia_U = nowaHistoria;
    wspolczynniki_B = b;
}

void ModelARX::setOpoznienie(int k)
{
    if (k < 0)
        k = 0;
    if (k == opoznienie)
        return;

    // Dostosowanie bufora opóźnienia
    if (k > opoznienie) {
        bufor_opoznienia.resize(k, 0.0);
    } else {
        while (bufor_opoznienia.size() > (size_t) k) {
            bufor_opoznienia.pop_front();
        }
    }
    opoznienie = k;
}

void ModelARX::setSzum(double stddev)
{
    stddev_szumu = stddev;
}
double ModelARX::getSzum() const
{
    return stddev_szumu;
}
std::vector<double> ModelARX::getA() const
{
    return wspolczynniki_A;
}
std::vector<double> ModelARX::getB() const
{
    return wspolczynniki_B;
}
int ModelARX::getOpoznienie() const
{
    return opoznienie;
}

double ModelARX::symuluj(double u)
{
    // Opóźnienie
    bufor_opoznienia.push_back(u);
    double u_delayed = 0.0;

    if (opoznienie == 0) {
        u_delayed = u;
        bufor_opoznienia.clear();
    } else {
        if (bufor_opoznienia.size() > (size_t) opoznienie) {
            u_delayed = bufor_opoznienia.front();
            bufor_opoznienia.pop_front();
        } else {
            u_delayed = 0.0;
        }
    }

    // Równanie ARX
    double y = 0.0;
    for (size_t i = 0; i < wspolczynniki_A.size(); ++i) {
        y -= wspolczynniki_A[i] * historia_Y[i];
    }

    // Aktualizacja historii U (przesunięcie)
    if (!historia_U.empty()) {
        for (size_t i = historia_U.size() - 1; i > 0; --i) {
            historia_U[i] = historia_U[i - 1];
        }
        historia_U[0] = u_delayed;
    }

    for (size_t i = 0; i < wspolczynniki_B.size(); ++i) {
        y += wspolczynniki_B[i] * historia_U[i];
    }

    // Szum
    if (stddev_szumu > 0.0001) {
        y += dist(gen) * stddev_szumu;
    }

    // Aktualizacja historii Y
    if (!historia_Y.empty()) {
        for (size_t i = historia_Y.size() - 1; i > 0; --i) {
            historia_Y[i] = historia_Y[i - 1];
        }
        historia_Y[0] = y;
    }

    return y;
}
