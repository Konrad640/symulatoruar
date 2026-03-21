#ifndef GENERATOR_H
#define GENERATOR_H

class Generator
{
public:
    enum class TypSygnalu { PROSTOKATNY, SINUSOIDALNY, SKOK };

    Generator();
    Generator(TypSygnalu typ,
              double poczatkowa,
              double zmiana,
              double czas_zmiany,
              double wyp = 0.5,
              double t_akt = 1.0);

    double generuj(double aktualny_czas) const;
    void ustawParametry(
        TypSygnalu typ, double pocz, double zm, double czas, double wyp, double t_akt);

    // Gettery
    TypSygnalu getTyp() const { return typ_sygnalu; }
    double getPoczatkowa() const { return wartosc_poczatkowa; }
    double getZmiana() const { return wartosc_zmiany; }
    double getCzasZmiany() const { return czas_zmiany; }
    double getWypelnienie() const { return wypelnienie; }
    double getCzasAktywacji() const { return czas_aktywacji; }

private:
    TypSygnalu typ_sygnalu;
    double wartosc_poczatkowa;
    double wartosc_zmiany;
    double czas_zmiany;
    double wypelnienie;
    double czas_aktywacji;
};

#endif // GENERATOR_H
