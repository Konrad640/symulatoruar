#ifndef REGULATORPID_H
#define REGULATORPID_H

// class ProstyUAR;

class RegulatorPID {
       public:
        enum class MetodaCalkowania { STALA_PRZED_SUMA, STALA_W_SUMIE };

        explicit RegulatorPID(double k, double ti = 0.0, double td = 0.0);

        // Główna funkcja symulacji
        double symuluj(double e);

        void setMetodaCalkowania(MetodaCalkowania metoda);
        MetodaCalkowania getMetodaCalkowania() const;

        void setKp(double k);
        double getKp() const { return Kp; };
        void setTi(double ti);
        double getTi() const { return Ti; };
        void setTd(double td);
        double getTd() const { return Td; };
        void zresetuj();
        void zresetujCalke();

        double pobierzOstatnieP() const { return ostatnie_P; }
        double pobierzOstatnieI() const { return ostatnie_I; }
        double pobierzOstatnieD() const { return ostatnie_D; }

       private:
        double Kp, Ti, Td;
        MetodaCalkowania metoda;

        double suma_uchybow;

        double poprzedni_uchyb;
        double ostatnie_P;
        double ostatnie_I;
        double ostatnie_D;
};

#endif  // REGULATORPID_H
