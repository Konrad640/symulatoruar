#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
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
    void onZmianaTaktowania();
    void onSieciPolaczono();
    void onSieciRozlaczono();
    void onSieciStatusZmieniony(const QString &opis);
    void onSieciOdebranoKonfiguracje(const QJsonObject &konfig);
    void onSieciOdebranoProbke(const QJsonObject &probka);
    void onSieciowyTimeout();

private:
    enum class TaktowanieSieci { Jednostronne, Obustronne };

    struct ProbkaSieciowa
    {
        double t = 0.0;
        double dt = 0.0;
        double w = 0.0;
        double y = 0.0;
        double e = 0.0;
        double u = 0.0;
        double P = 0.0;
        double I = 0.0;
        double D = 0.0;
        double shadowY = 0.0;
        double shadowU = 0.0;
    };

    ModelARX arx;
    RegulatorPID pid;
    Generator gen;
    ProstyUAR petla;
    Sieci sieci;

    double oknoCzasowe = 10.0;

    QTimer *m_timerSieciowy = nullptr;
    QTimer *m_timerWydajnosci = nullptr;
    ProbkaSieciowa m_probkaSieci;
    int  m_numerProbkiSieciowej = 0;
    int  m_ostatniSeqSterowania = 0;
    int  m_ostatniSeqOdpowiedzi = 0;
    int  m_brakiSterowaniaPodRzad = 0;
    int  m_taktyBezOdpowiedzi = 0;
    int  m_ostatniDesync = 0;
    bool m_symulacjaSieciowaTrwa = false;
    bool m_oczekujeNaOdpowiedz = false;
    bool m_maNoweSterowanie = false;

    bool m_aplikujeZdalna = false;
    bool m_rozlaczanieCelowe = false;
    bool m_byloPolaczenie = false;
    int  m_ostatniIndeksTaktowaniaSieci = 0;

    int m_pakietyWyslaneOkno = 0;
    int m_pakietyOdebraneOkno = 0;
    QElapsedTimer m_zegarWydajnosci;

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
    QComboBox *comboTaktowanieSieci;
    QComboBox *comboSerializacja;
    QLabel *lblPartnerSieci;
    QLabel *lblSyncSieci;
    QLabel *lblSterowanieSieci;
    QLabel *lblWydajnoscSieci;
    QLabel *lblLampkaWydajnosci;
    QLabel *lblShadowSieci;

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
    void wyczyscWykresy();
    void aktualizujDaneWykresow(double t, double w, double y, double e, double u);
    void aktualizujDaneWykresow(
        double t, double w, double y, double e, double u, double p, double i, double d);

    QJsonObject zbudujKonfiguracjeJson() const;
    void zastosujKonfiguracjeJson(const QJsonObject &konfig);
    void wyslijKonfiguracjeJesliPolaczony();
    void aktualizujStanKontrolek();
    void ustawWskaznikPolaczenia(bool polaczony);
    void przelaczTrybSieciowy(Sieci::Tryb nowyTryb, const QString &pytanie);
    void zresetujStanSieciowy();
    void uruchomSymulacjeSieciowa();
    void zatrzymajSymulacjeSieciowa();
    void oznaczStartTransmisji();
    bool czyTrybSieciowy() const;
    bool czySiecJednostronna() const;
    TaktowanieSieci aktualneTaktowanieSieci() const;
    void wykonajKrokRegulatoraSieciowego();
    void wykonajKrokObiektuSieciowego();
    void obsluzSterowanieSieciowe(const QJsonObject &probka);
    void obsluzWyjscieSieciowe(const QJsonObject &probka);
    void obsluzKomendeSieciowa(const QJsonObject &probka);
    QJsonObject probkaBazowaJson(const QString &rodzaj, int seq) const;
    void wyslijProbkeSieciowa(QJsonObject probka);
    void wyslijKomendeSieciowa(const QString &komenda);
    void odswiezWydajnoscSieci();
    void ustawLampkeWydajnosci(bool ok, const QString &opis);
    void ustawStatusSynchronizacji(int desync, qint64 opoznienieMs);
    void aktualizujInformacjePartnera();
    void kontynuujLokalniePoRozlaczeniu();
};

#endif // MAINWINDOW_H
