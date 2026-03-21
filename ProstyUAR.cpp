#include "ProstyUAR.h"
#include "RegulatorPID.h"

ProstyUAR::ProstyUAR(ModelARX &arx, RegulatorPID &pid)
    : model(arx)
    , regulator(pid)
    , poprzednie_y(0.0)
{}

double ProstyUAR::symuluj(double w)
{
    // Oblicz uchyb (wartość zadana - wyjście z poprzedniego kroku)
    double e = w - poprzednie_y;

    // Oblicz sterowanie z regulatora (zakładamy dt = 1.0 dla symulacji dyskretnej)
    // Używamy przeciążonej metody symuluj(e) z dodanego adaptera w RegulatorzePID
    double u = regulator.symuluj(e);

    // Symuluj obiekt
    double y = model.symuluj(u);

    // Zapamiętaj wyjście dla następnego kroku
    poprzednie_y = y;

    return y;
}
