#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QtCharts>

#include "Generator.h"
#include "ModelARX.h"
#include "ProstyUAR.h"
#include "RegulatorPID.h"
#include "Sieci.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

    Sieci sieci;

private slots:
    void onKrokWykonany(double czas, double w, double y, double e, double u);

    void przelaczSymulacje();
    void zresetujSymulacje();
    void otworzKonfiguracjeARX();

    void aktualizujPID();
    void aktualizujGenerator();
    void aktualizujInterwal();
    void aktualizujOknoCzasowe();

    void zresetujCalkePID();
    void zapiszKonfiguracje();
    void wczytajKonfiguracje();

    // Sloty sieciowe
    void wlaczTrybRegulator();
    void wlaczTrybObiekt();
    void rozlaczSiec();
    void onSieciPolaczono();
    void onSieciRozlaczono();
    void onSieciStatusZmieniony(const QString &opis);
    void onSieciOdebranoKonfiguracje(const QJsonObject &konfig);

private:
    ModelARX arx;
    RegulatorPID pid;
    Generator gen;
    ProstyUAR petla;

    double oknoCzasowe;
    bool   m_aplikujeZdalna;

    // GUI - Kontrolki
    QLineEdit *edycjaKp, *edycjaTi, *edycjaTd;
    QComboBox *comboMetodaCalk;

    QComboBox *comboGenTyp;
    QDoubleSpinBox *spinGenOffset, *spinGenAmp, *spinGenOkres;
    QDoubleSpinBox *spinGenWypelnienie;
    QDoubleSpinBox *spinGenCzasAkt;

    QSpinBox *spinInterwal;
    QDoubleSpinBox *spinOknoCzasowe;

    QPushButton *btnStartStop;
    QPushButton *btnReset;
    QPushButton *btnArx;
    QPushButton *btnResetI;
    QPushButton *btnZapiszJson;
    QPushButton *btnWczytajJson;

    // GUI - Sieci
    QPushButton *btnTrybRegulator;
    QPushButton *btnTrybObiekt;
    QPushButton *btnRozlacz;
    QLineEdit *edycjaIp;
    QSpinBox *spinPort;
    QLabel *lblStatusSieci;
    QLabel *lblWskaznikPolaczenia;

    QGroupBox *grpPid;
    QGroupBox *grpGen;
    QGroupBox *grpSym;

    // Wykresy
    QLineSeries *seriaZadana, *seriaWyjscie, *seriaUchyb, *seriaSterowanie;
    QLineSeries *seriaP, *seriaI, *seriaD;
    QChart *wykresGlowny, *wykresUchyb, *wykresSterowanie, *wykresPID;
    QValueAxis *osXGlowny, *osYGlowny, *osXUchyb, *osYUchyb;
    QValueAxis *osXSterowanie, *osYSterowanie, *osXPID, *osYPID;

    void konfigurujGUI();
    void konfigurujWykresy();
    void aktualizujDaneWykresow(double t, double w, double y, double e, double u);

    QJsonObject zbudujKonfiguracjeJson() const;
    void zastosujKonfiguracjeJson(const QJsonObject &konfig);
    void wyslijKonfiguracjeJesliPolaczony();
    void aktualizujStanKontrolek();
    void ustawWskaznikPolaczenia(bool polaczony);
};

#endif // MAINWINDOW_H
