#include "TestRunner.h"
#include "Testy_UAR.cpp"
#include <iostream>

void TestRunner::runAllTests()
{
    StatystykiTestow::reset();

    std::cerr << "=== ROZPOCZECIE TESTOW ===\n\n";

    // Wywołujemy funkcje z istniejących przestrzeni nazw
    TESTY_ModelARX::wykonaj_testy();
    TESTY_RegulatorPID::wykonaj_testy();
    TESTY_ProstyUAR::wykonaj_testy();

    // Wywołujemy nowe testy
    TESTY_Dodatkowe::wykonaj_testy();

    // Podsumowanie
    StatystykiTestow::raportKoncowy();
}
