#include "ModelARX.h"
#include "ProstyUAR.h"
#include "RegulatorPID.h"
#include "Generator.h" // <--- DODANO NAGŁÓWEK
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>

// MECHANIZM ZLICZANIA TESTÓW
namespace StatystykiTestow {
static int liczba_sukcesow = 0;
static int liczba_wszystkich = 0;

void reset()
{
    liczba_sukcesow = 0;
    liczba_wszystkich = 0;
}

void raportKoncowy()
{
    std::cerr << "\n--------------------------------------------------\n";
    std::cerr << "PODSUMOWANIE TESTOW:\n";
    std::cerr << "Testy zakonczone powodzeniem: " << liczba_sukcesow << "/" << liczba_wszystkich
              << "\n";
    std::cerr << "--------------------------------------------------\n";
}
} // namespace StatystykiTestow

// Funkcje pomocnicze dla testów:

void raportBleduSekwencji(std::vector<double> &spodz, std::vector<double> &fakt)
{
    constexpr size_t PREC = 3;
    std::cerr << std::fixed << std::setprecision(PREC);
    std::cerr << "  Spodziewany:\t";
    for (auto &el : spodz)
        std::cerr << el << ", ";
    std::cerr << "\n  Faktyczny:\t";
    for (auto &el : fakt)
        std::cerr << el << ", ";
    std::cerr << std::endl << std::endl;
}

bool porownanieSekwencji(std::vector<double> &spodz, std::vector<double> &fakt)
{
    constexpr double TOL = 1e-3; // tolerancja dla porównań zmiennoprzecinkowych
    if (fakt.size() != spodz.size())
        return false;

    bool result = true;
    for (size_t i = 0; result && i < fakt.size(); i++)
        result = std::fabs(fakt[i] - spodz[i]) < TOL;
    return result;
}

void myAssert(std::vector<double> &spodz, std::vector<double> &fakt)
{
    StatystykiTestow::liczba_wszystkich++;
    if (porownanieSekwencji(spodz, fakt)) {
        std::cerr << "OK!\n";
        StatystykiTestow::liczba_sukcesow++;
    } else {
        std::cerr << "FAIL!\n";
        raportBleduSekwencji(spodz, fakt);
    }
}

// Przeciążenie myAssert dla pojedynczych wartości (do nowych testów)
void myAssert(double spodz, double fakt)
{
    StatystykiTestow::liczba_wszystkich++;
    if (std::fabs(spodz - fakt) < 1e-3) {
        std::cerr << "OK!\n";
        StatystykiTestow::liczba_sukcesow++;
    } else {
        std::cerr << "FAIL! Spodz: " << spodz << " Fakt: " << fakt << "\n";
    }
}

namespace TESTY_ModelARX {
void wykonaj_testy();
void test_brakPobudzenia();
void test_skokJednostkowy_1();
void test_skokJednostkowy_2();
void test_skokJednostkowy_3();
} // namespace TESTY_ModelARX

void TESTY_ModelARX::wykonaj_testy()
{
    test_brakPobudzenia();
    test_skokJednostkowy_1();
    test_skokJednostkowy_2();
    test_skokJednostkowy_3();
}

void TESTY_ModelARX::test_brakPobudzenia()
{
    std::cerr << "ModelARX (-0.4 | 0.6 | 1 | 0 ) -> test zerowego pobudzenia: ";
    try {
        ModelARX instancjaTestowa({-0.4}, {0.6}, 1, 0);
        constexpr size_t LICZ_ITER = 30;
        std::vector<double> sygWe(LICZ_ITER, 0.0);
        std::vector<double> spodzSygWy(LICZ_ITER, 0.0);
        std::vector<double> faktSygWy(LICZ_ITER);

        for (int i = 0; i < LICZ_ITER; i++)
            faktSygWy[i] = instancjaTestowa.symuluj(sygWe[i]);
        myAssert(spodzSygWy, faktSygWy);
    } catch (...) {
        std::cerr << "EXCEPTION!\n";
    }
}

void TESTY_ModelARX::test_skokJednostkowy_1()
{
    std::cerr << "ModelARX (-0.4 | 0.6 | 1 | 0 ) -> test skoku jednostkowego nr 1: ";
    try {
        ModelARX instancjaTestowa({-0.4}, {0.6}, 1, 0);
        constexpr size_t LICZ_ITER = 30;
        std::vector<double> sygWe(LICZ_ITER);
        std::vector<double> spodzSygWy = {0,        0,        0.6,      0.84,     0.936,
                                          0.9744,   0.98976,  0.995904, 0.998362, 0.999345,
                                          0.999738, 0.999895, 0.999958, 0.999983, 0.999993,
                                          0.999997, 0.999999, 1,        1,        1,
                                          1,        1,        1,        1,        1,
                                          1,        1,        1,        1,        1};
        std::vector<double> faktSygWy(LICZ_ITER);

        for (int i = 0; i < LICZ_ITER; i++) {
            sygWe[i] = !!i;
            faktSygWy[i] = instancjaTestowa.symuluj(sygWe[i]);
        }
        myAssert(spodzSygWy, faktSygWy);
    } catch (...) {
        std::cerr << "EXCEPTION!\n";
    }
}

void TESTY_ModelARX::test_skokJednostkowy_2()
{
    std::cerr << "ModelARX (-0.4 | 0.6 | 2 | 0 ) -> test skoku jednostkowego nr 2: ";
    try {
        ModelARX instancjaTestowa({-0.4}, {0.6}, 2, 0);
        constexpr size_t LICZ_ITER = 30;
        std::vector<double> sygWe(LICZ_ITER);
        std::vector<double> spodzSygWy = {0,        0,        0,        0.6,      0.84,
                                          0.936,    0.9744,   0.98976,  0.995904, 0.998362,
                                          0.999345, 0.999738, 0.999895, 0.999958, 0.999983,
                                          0.999993, 0.999997, 0.999999, 1,        1,
                                          1,        1,        1,        1,        1,
                                          1,        1,        1,        1,        1};
        std::vector<double> faktSygWy(LICZ_ITER);

        for (int i = 0; i < LICZ_ITER; i++) {
            sygWe[i] = !!i;
            faktSygWy[i] = instancjaTestowa.symuluj(sygWe[i]);
        }
        myAssert(spodzSygWy, faktSygWy);
    } catch (...) {
        std::cerr << "EXCEPTION!\n";
    }
}

void TESTY_ModelARX::test_skokJednostkowy_3()
{
    std::cerr << "ModelARX (-0.4, 0.2 | 0.6, 0.3 | 2 | 0 ) -> test skoku jednostkowego nr 3: ";
    try {
        ModelARX instancjaTestowa({-0.4, 0.2}, {0.6, 0.3}, 2, 0);
        constexpr size_t LICZ_ITER = 30;
        std::vector<double> sygWe(LICZ_ITER);
        std::vector<double> spodzSygWy = {0,       0,       0,       0.6,     1.14,    1.236,
                                          1.1664,  1.11936, 1.11446, 1.12191, 1.12587, 1.12597,
                                          1.12521, 1.12489, 1.12491, 1.12499, 1.12501, 1.12501,
                                          1.125,   1.125,   1.125,   1.125,   1.125,   1.125,
                                          1.125,   1.125,   1.125,   1.125,   1.125,   1.125};
        std::vector<double> faktSygWy(LICZ_ITER);

        for (int i = 0; i < LICZ_ITER; i++) {
            sygWe[i] = !!i;
            faktSygWy[i] = instancjaTestowa.symuluj(sygWe[i]);
        }
        myAssert(spodzSygWy, faktSygWy);
    } catch (...) {
        std::cerr << "EXCEPTION!\n";
    }
}

namespace TESTY_RegulatorPID {
void wykonaj_testy();
void test_P_brakPobudzenia();
void test_P_skokJednostkowy();
void test_PI_skokJednostkowy_1();
void test_PI_skokJednostkowy_2();
void test_PID_skokJednostkowy();
void test_PI_skokJednostkowy_3();
} // namespace TESTY_RegulatorPID

void TESTY_RegulatorPID::wykonaj_testy()
{
    test_P_brakPobudzenia();
    test_P_skokJednostkowy();
    test_PI_skokJednostkowy_1();
    test_PI_skokJednostkowy_2();
    test_PID_skokJednostkowy();
    test_PI_skokJednostkowy_3();
}

void TESTY_RegulatorPID::test_P_brakPobudzenia()
{
    std::cerr << "RegP (k = 0.5) -> test zerowego pobudzenia: ";
    try {
        RegulatorPID instancjaTestowa(0.5);
        constexpr size_t LICZ_ITER = 30;
        std::vector<double> spodzSygWy(LICZ_ITER, 0.0);
        std::vector<double> faktSygWy(LICZ_ITER);
        for (int i = 0; i < LICZ_ITER; i++)
            faktSygWy[i] = instancjaTestowa.symuluj(0.0);
        myAssert(spodzSygWy, faktSygWy);
    } catch (...) {
    }
}

void TESTY_RegulatorPID::test_P_skokJednostkowy()
{
    std::cerr << "RegP (k = 0.5) -> test skoku jednostkowego: ";
    try {
        RegulatorPID instancjaTestowa(0.5);
        std::vector<double> spodz(30, 0.5);
        spodz[0] = 0.0;
        std::vector<double> fakt(30);
        for (int i = 0; i < 30; i++)
            fakt[i] = instancjaTestowa.symuluj(!!i);
        myAssert(spodz, fakt);
    } catch (...) {
    }
}

void TESTY_RegulatorPID::test_PI_skokJednostkowy_1()
{
    std::cerr << "RegPI (k = 0.5, TI = 1.0) -> test skoku jednostkowego nr 1: ";
    try {
        RegulatorPID instancjaTestowa(0.5, 1.0);
        std::vector<double> spodz = {0,    1.5,  2.5,  3.5,  4.5,  5.5,  6.5,  7.5,  8.5,  9.5,
                                     10.5, 11.5, 12.5, 13.5, 14.5, 15.5, 16.5, 17.5, 18.5, 19.5,
                                     20.5, 21.5, 22.5, 23.5, 24.5, 25.5, 26.5, 27.5, 28.5, 29.5};
        std::vector<double> fakt(30);
        for (int i = 0; i < 30; i++)
            fakt[i] = instancjaTestowa.symuluj(!!i);
        myAssert(spodz, fakt);
    } catch (...) {
    }
}

void TESTY_RegulatorPID::test_PI_skokJednostkowy_2()
{
    std::cerr << "RegPI (k = 0.5, TI = 10.0) -> test skoku jednostkowego nr 2: ";
    try {
        RegulatorPID instancjaTestowa(0.5, 10.0);
        std::vector<double> spodz = {0,   0.6, 0.7, 0.8, 0.9, 1, 1.1, 1.2, 1.3, 1.4,
                                     1.5, 1.6, 1.7, 1.8, 1.9, 2, 2.1, 2.2, 2.3, 2.4,
                                     2.5, 2.6, 2.7, 2.8, 2.9, 3, 3.1, 3.2, 3.3, 3.4};
        std::vector<double> fakt(30);
        for (int i = 0; i < 30; i++)
            fakt[i] = instancjaTestowa.symuluj(!!i);
        myAssert(spodz, fakt);
    } catch (...) {
    }
}

void TESTY_RegulatorPID::test_PID_skokJednostkowy()
{
    std::cerr << "RegPID (k = 0.5, TI = 10.0, TD = 0.2) -> test skoku jednostkowego: ";
    try {
        RegulatorPID instancjaTestowa(0.5, 10.0, 0.2);
        std::vector<double> spodz = {0,   0.8, 0.7, 0.8, 0.9, 1, 1.1, 1.2, 1.3, 1.4,
                                     1.5, 1.6, 1.7, 1.8, 1.9, 2, 2.1, 2.2, 2.3, 2.4,
                                     2.5, 2.6, 2.7, 2.8, 2.9, 3, 3.1, 3.2, 3.3, 3.4};
        std::vector<double> fakt(30);
        for (int i = 0; i < 30; i++)
            fakt[i] = instancjaTestowa.symuluj(!!i);
        myAssert(spodz, fakt);
    } catch (...) {
    }
}

void TESTY_RegulatorPID::test_PI_skokJednostkowy_3()
{
    std::cerr << "RegPI (zmiana parametrow) -> test skoku jednostkowego nr 3: ";
    try {
        RegulatorPID it(0.5, 10.0);
        std::vector<double> spodz = {0,   0.6, 0.7, 0.8, 0.9,  1,    1.1,  1.2,  1.3,  1.4,
                                     1.5, 1.6, 1.8, 2,   2.2,  2.4,  2.6,  2.8,  3,    3.2,
                                     3.4, 3.6, 3.8, 4,   2.35, 2.45, 2.55, 2.65, 2.75, 2.85};
        std::vector<double> fakt(30);
        for (int i = 0; i < 30; i++) {
            if (i == 6)
                it.setLiczCalk(RegulatorPID::LiczCalk::Wew);
            if (i == 12)
                it.setStalaCalk(5.0);
            if (i == 18)
                it.setLiczCalk(RegulatorPID::LiczCalk::Zew);
            if (i == 24)
                it.setStalaCalk(10.0);
            fakt[i] = it.symuluj(!!i);
        }
        myAssert(spodz, fakt);
    } catch (...) {
    }
}

namespace TESTY_ProstyUAR {
void wykonaj_testy();
void test_UAR_1_brakPobudzenia();
void test_UAR_1_skokJednostkowyPID();
void test_UAR_2_skokJednostkowyPID();
void test_UAR_3_skokJednostkowyPID();
} // namespace TESTY_ProstyUAR

void TESTY_ProstyUAR::wykonaj_testy()
{
    test_UAR_1_brakPobudzenia();
    test_UAR_1_skokJednostkowyPID();
    test_UAR_2_skokJednostkowyPID();
    test_UAR_3_skokJednostkowyPID();
}

void TESTY_ProstyUAR::test_UAR_1_brakPobudzenia()
{
    std::cerr << "UAR_1 -> test zerowego pobudzenia: ";
    RegulatorPID pid(0.5, 5.0, 0.2);
    ModelARX arx({-0.4}, {0.6});
    Generator gen(Generator::TypSygnalu::PROSTOKATNY, 0.0, 0.0, 10.0, 0.5, 1.0); // <--- DODANY GENERATOR
    ProstyUAR uar(arx, pid, gen);
    std::vector<double> spodz(30, 0.0), fakt(30);
    for (int i = 0; i < 30; i++)
        fakt[i] = uar.symuluj(0.0);
    myAssert(spodz, fakt);
}

void TESTY_ProstyUAR::test_UAR_1_skokJednostkowyPID()
{
    std::cerr << "UAR_1 PID -> test skoku jednostkowego: ";
    RegulatorPID pid(0.5, 5.0, 0.2);
    ModelARX arx({-0.4}, {0.6});
    Generator gen(Generator::TypSygnalu::PROSTOKATNY, 0.0, 0.0, 10.0, 0.5, 1.0); // <--- DODANY GENERATOR
    ProstyUAR uar(arx, pid, gen);
    std::vector<double> spodz = {0.0,      0.0,      0.54,     0.756,    0.6708,   0.64008,
                                 0.729,    0.810437, 0.834499, 0.843338, 0.8664,   0.8936,
                                 0.911886, 0.923312, 0.93404,  0.944929, 0.954065, 0.961042,
                                 0.966815, 0.971965, 0.97642,  0.980096, 0.983143, 0.985741,
                                 0.987964, 0.989839, 0.991411, 0.992739, 0.993865, 0.994818};
    std::vector<double> fakt(30);
    for (int i = 0; i < 30; i++)
        fakt[i] = uar.symuluj(!!i);
    myAssert(spodz, fakt);
}

void TESTY_ProstyUAR::test_UAR_2_skokJednostkowyPID()
{
    std::cerr << "UAR_2 PID -> test skoku jednostkowego: ";
    RegulatorPID pid(0.5, 5.0, 0.2);
    ModelARX arx({-0.4}, {0.6}, 2);
    Generator gen(Generator::TypSygnalu::PROSTOKATNY, 0.0, 0.0, 10.0, 0.5, 1.0); // <--- DODANY GENERATOR
    ProstyUAR uar(arx, pid, gen);
    std::vector<double> spodz = {0.0,      0.0,      0.0,      0.54,     0.756,    0.9624,
                                 0.87336,  0.841104, 0.771946, 0.821644, 0.863453, 0.93272,
                                 0.952656, 0.965421, 0.954525, 0.955787, 0.957472, 0.969711,
                                 0.978075, 0.985968, 0.987821, 0.989149, 0.989053, 0.990645,
                                 0.992248, 0.994403, 0.995707, 0.996677, 0.997024, 0.997388};
    std::vector<double> fakt(30);
    for (int i = 0; i < 30; i++)
        fakt[i] = uar.symuluj(!!i);
    myAssert(spodz, fakt);
}

void TESTY_ProstyUAR::test_UAR_3_skokJednostkowyPID()
{
    std::cerr << "UAR_3 PID -> test skoku jednostkowego: ";
    RegulatorPID pid(1.0, 2.0, 0.2);
    ModelARX arx({-0.4}, {0.6}, 1);
    Generator gen(Generator::TypSygnalu::PROSTOKATNY, 0.0, 0.0, 10.0, 0.5, 1.0);
    ProstyUAR uar(arx, pid, gen);
    std::vector<double> spodz = {0.0,      0.0,      1.02,     1.608,    1.1028,   0.41736,
                                 0.546648, 1.20605,  1.43047,  0.999176, 0.615056, 0.799121,
                                 1.21304,  1.26025,  0.939289, 0.748507, 0.927166, 1.17292,
                                 1.14155,  0.921616, 0.843258, 0.990018, 1.12577,  1.068,
                                 0.927024, 0.908125, 1.01702,  1.08484,  1.02618,  0.941508};
    std::vector<double> fakt(30);
    for (int i = 0; i < 30; i++)
        fakt[i] = uar.symuluj(!!i);
    myAssert(spodz, fakt);
}

// NOWE TESTY - 10 DODATKOWYCH SCENARIUSZY

namespace TESTY_Dodatkowe {
void wykonaj_testy();

void test_ARX_OdpowiedzImpulsowa();
void test_ARX_ZmianaB_w_locie();
void test_ARX_ResetStanu();
void test_ARX_DuzeOpoznienie();
void test_ARX_Szum();
void test_PID_SamoWzmocnienieP();
void test_PID_ZeroweNastawy();
void test_UAR_ZmianaWartosciZadanej();
void test_UAR_UkladNiestabilny();
void test_UAR_UchybUstalonyPI();
} // namespace TESTY_Dodatkowe

void TESTY_Dodatkowe::wykonaj_testy()
{
    test_ARX_OdpowiedzImpulsowa();
    test_ARX_ZmianaB_w_locie();
    test_ARX_ResetStanu();
    test_ARX_DuzeOpoznienie();
    test_ARX_Szum();
    test_PID_SamoWzmocnienieP();
    test_PID_ZeroweNastawy();
    test_UAR_ZmianaWartosciZadanej();
    test_UAR_UkladNiestabilny();
    test_UAR_UchybUstalonyPI();
}

// 1. Test odpowiedzi impulsowej ARX
// Dla y[k] = 0.5 y[k-1] + 1.0 u[k]. Impuls u={1,0,0...} -> y={1, 0.5, 0.25, 0.125...}
void TESTY_Dodatkowe::test_ARX_OdpowiedzImpulsowa()
{
    std::cerr << "NOWY: ModelARX -> test odpowiedzi impulsowej: ";
    try {
        ModelARX model({-0.5}, {1.0}, 0); // Bez opóźnienia dla uproszczenia
        std::vector<double> spodz = {1.0, 0.5, 0.25, 0.125, 0.0625};
        std::vector<double> fakt;

        fakt.push_back(model.symuluj(1.0)); // Impuls
        for (int i = 0; i < 4; i++)
            fakt.push_back(model.symuluj(0.0));

        myAssert(spodz, fakt);
    } catch (...) {
        std::cerr << "EXCEPTION!\n";
    }
}

// 2. Test dynamicznej zmiany wielomianu B
void TESTY_Dodatkowe::test_ARX_ZmianaB_w_locie()
{
    std::cerr << "NOWY: ModelARX -> zmiana B w trakcie symulacji: ";
    try {
        ModelARX model({0.0}, {1.0}, 0); // y = u
        double y1 = model.symuluj(1.0);  // y = 1.0

        model.setB({2.0});              // Teraz y = 2u
        double y2 = model.symuluj(1.0); // y = 2.0

        std::vector<double> spodz = {1.0, 2.0};
        std::vector<double> fakt = {y1, y2};

        myAssert(spodz, fakt);
    } catch (...) {
        std::cerr << "EXCEPTION!\n";
    }
}

// 3. Test resetowania stanu
void TESTY_Dodatkowe::test_ARX_ResetStanu()
{
    std::cerr << "NOWY: ModelARX -> reset stanu: ";
    try {
        ModelARX model({-0.5}, {1.0}, 1);
        model.symuluj(1.0); // Nakręcamy stan
        model.symuluj(1.0);

        model.zresetuj_stan();

        // Po resecie, dla zerowego wejścia wyjście powinno być 0 (brak pamięci)
        double y_after_reset = model.symuluj(0.0);
        myAssert(0.0, y_after_reset);
    } catch (...) {
        std::cerr << "EXCEPTION!\n";
    }
}

// 4. Test dużego opóźnienia
void TESTY_Dodatkowe::test_ARX_DuzeOpoznienie()
{
    std::cerr << "NOWY: ModelARX -> duze opoznienie (k=5): ";
    try {
        ModelARX model({0.0}, {1.0}, 5);
        std::vector<double> we = {1, 1, 1, 1, 1, 1, 1, 1};
        std::vector<double> spodz = {0, 0, 0, 0, 0, 1, 1, 1}; // 5 zer na początku
        std::vector<double> fakt;
        for (double u : we)
            fakt.push_back(model.symuluj(u));
        myAssert(spodz, fakt);
    } catch (...) {
        std::cerr << "EXCEPTION!\n";
    }
}

// 5. Test obecności szumu (statystyczny)
void TESTY_Dodatkowe::test_ARX_Szum()
{
    std::cerr << "NOWY: ModelARX -> weryfikacja szumu: ";
    try {
        ModelARX model({0.0}, {1.0}, 0, 0.1); // StdDev = 0.1
        std::vector<double> fakt;
        double sum = 0;
        for (int i = 0; i < 100; i++) {
            double y = model.symuluj(0.0); // Wejście 0, wyjście to sam szum
            fakt.push_back(y);
            sum += y;
        }

        // Sprawdzamy czy nie wszystkie są zerami i czy średnia jest bliska 0
        bool saRozneOdZera = false;
        for (double v : fakt)
            if (std::abs(v) > 0.0001)
                saRozneOdZera = true;

        if (saRozneOdZera && std::abs(sum / 100.0) < 0.1) {
            std::cerr << "OK!\n";
            StatystykiTestow::liczba_sukcesow++;
        } else {
            std::cerr << "FAIL! (Szum nie działa lub statystyka dziwna)\n";
        }
        StatystykiTestow::liczba_wszystkich++;
    } catch (...) {
        std::cerr << "EXCEPTION!\n";
    }
}

// 6. Test regulatora PID - tylko człon P
void TESTY_Dodatkowe::test_PID_SamoWzmocnienieP()
{
    std::cerr << "NOWY: PID -> czyste P, duze wzmocnienie: ";
    try {
        // Duże Ti i małe Td wyłącza wpływ I i D w pierwszym kroku lub w ogóle
        RegulatorPID pid(10.0, 10000.0, 0.0);
        double u = pid.symuluj(2.0); // Uchyb 2.0 -> Sterowanie powinno być 20.0
        // Uwaga: PID może mieć składnik D od startu (0->2), ale przy Td=0 powinno byc czyste P

        // Pierwsza próbka: u = Kp * e = 10 * 2 = 20.
        myAssert(20.0, u);
    } catch (...) {
        std::cerr << "EXCEPTION!\n";
    }
}

// 7. Test PID - zerowe nastawy
void TESTY_Dodatkowe::test_PID_ZeroweNastawy()
{
    std::cerr << "NOWY: PID -> zerowe nastawy (wyjscie 0): ";
    try {
        RegulatorPID pid(0.0, 10000.0, 0.0);
        double u = pid.symuluj(100.0);
        myAssert(0.0, u);
    } catch (...) {
        std::cerr << "EXCEPTION!\n";
    }
}

// 8. Test UAR - zmiana wartości zadanej (skok 1 -> -1)
void TESTY_Dodatkowe::test_UAR_ZmianaWartosciZadanej()
{
    std::cerr << "NOWY: UAR -> zmiana setpointu (1 -> -1): ";
    try {
        RegulatorPID pid(1.0);          // P=1
        ModelARX arx({-0.5}, {0.5}, 1); // Prosty obiekt I rzędu
        Generator gen(Generator::TypSygnalu::PROSTOKATNY, 0.0, 0.0, 10.0, 0.5, 1.0); // <--- DODANY GENERATOR
        ProstyUAR uar(arx, pid, gen);

        // Stabilizacja na 1
        for (int i = 0; i < 10; i++)
            uar.symuluj(1.0);

        // Skok na -1. Spodziewamy się, że wyjście zacznie spadać
        double y_prev = uar.symuluj(-1.0);
        double y_curr = uar.symuluj(-1.0);

        StatystykiTestow::liczba_wszystkich++;
        if (y_curr < y_prev) {
            std::cerr << "OK!\n";
            StatystykiTestow::liczba_sukcesow++;
        } else {
            std::cerr << "FAIL! Wyjscie nie spada po zmianie znaku zadanej.\n";
        }
    } catch (...) {
        std::cerr << "EXCEPTION!\n";
    }
}

// 9. Test UAR - układ niestabilny (zbyt duże wzmocnienie)
void TESTY_Dodatkowe::test_UAR_UkladNiestabilny()
{
    std::cerr << "NOWY: UAR -> test niestabilnosci (rosnaca amplituda): ";
    try {
        RegulatorPID pid(50.0); // Ogromne wzmocnienie
        ModelARX arx({-0.9}, {1.0}, 1);
        Generator gen(Generator::TypSygnalu::PROSTOKATNY, 0.0, 0.0, 10.0, 0.5, 1.0); // <--- DODANY GENERATOR
        ProstyUAR uar(arx, pid, gen);

        uar.symuluj(1.0);
        double y1 = uar.symuluj(1.0);
        double y2 = uar.symuluj(1.0);
        double y3 = uar.symuluj(1.0);

        // Sprawdzamy czy oscylacje rosną (co do modułu)
        StatystykiTestow::liczba_wszystkich++;
        if (std::abs(y3) > std::abs(y2) && std::abs(y2) > std::abs(y1)) {
            std::cerr << "OK!\n";
            StatystykiTestow::liczba_sukcesow++;
        } else {
            std::cerr << "FAIL! Uklad powinien byc niestabilny.\n";
        }
    } catch (...) {
        std::cerr << "EXCEPTION!\n";
    }
}

// 10. Test UAR - Uchyb ustalony dla PI vs P
void TESTY_Dodatkowe::test_UAR_UchybUstalonyPI()
{
    std::cerr << "NOWY: UAR -> redukcja uchybu przez czlon I: ";
    try {
        ModelARX arx({-0.8}, {0.2}, 1); // Obiekt ze wzmocnieniem statycznym 1.0 (0.2 / (1-0.8))
        Generator gen(Generator::TypSygnalu::PROSTOKATNY, 0.0, 0.0, 10.0, 0.5, 1.0); // <--- DODANY GENERATOR

        // Regulator P
        RegulatorPID regP(1.0);
        ProstyUAR uarP(arx, regP, gen); // <--- PODANY GENERATOR
        double yP = 0;
        for (int i = 0; i < 50; i++)
            yP = uarP.symuluj(1.0);
        double errP = 1.0 - yP;

        // Regulator PI
        RegulatorPID regPI(1.0, 2.0);
        ProstyUAR uarPI(arx, regPI, gen); // <--- PODANY GENERATOR
        double yPI = 0;
        for (int i = 0; i < 50; i++)
            yPI = uarPI.symuluj(1.0);
        double errPI = 1.0 - yPI;

        // Uchyb przy PI powinien być bliski 0 i mniejszy niż przy P
        StatystykiTestow::liczba_wszystkich++;
        if (std::abs(errPI) < 0.01 && std::abs(errPI) < std::abs(errP)) {
            std::cerr << "OK!\n";
            StatystykiTestow::liczba_sukcesow++;
        } else {
            std::cerr << "FAIL! PI nie zredukowal uchybu. P_err: " << errP << " PI_err: " << errPI
                      << "\n";
        }
    } catch (...) {
        std::cerr << "EXCEPTION!\n";
    }
}
