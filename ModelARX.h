#ifndef MODELARX_H
#define MODELARX_H

#include <deque>
#include <random>
#include <vector>

class ModelARX
{
public:
    ModelARX(const std::vector<double> &a,
             const std::vector<double> &b,
             int k = 1,
             double stddev = 0.0);

    double symuluj(double u);

    // Settery (z logicznym zachowaniem historii)
    void setA(const std::vector<double> &a);
    std::vector<double> getA() const;

    void setB(const std::vector<double> &b);
    std::vector<double> getB() const;

    void setOpoznienie(int k);
    int getOpoznienie() const;

    void setSzum(double stddev);
    double getSzum() const;

    void zresetuj_stan();

private:
    std::vector<double> wspolczynniki_A;
    std::vector<double> wspolczynniki_B;
    int opoznienie;
    double stddev_szumu;

    // Bufory
    std::deque<double> bufor_opoznienia;
    std::vector<double> historia_U;
    std::vector<double> historia_Y;

    // Szum
    std::mt19937 gen;
    std::normal_distribution<> dist;
};

#endif // MODELARX_H
