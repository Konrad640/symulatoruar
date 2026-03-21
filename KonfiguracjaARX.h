#ifndef KONFIGURACJAARX_H
#define KONFIGURACJAARX_H

#include <QDialog>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include "ModelARX.h"
#include <vector>

class KonfiguracjaARX : public QDialog
{
    Q_OBJECT
public:
    explicit KonfiguracjaARX(ModelARX *model, QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    ModelARX *modelArx;

    // Pola dla współczynników A
    QDoubleSpinBox *spinA1;
    QDoubleSpinBox *spinA2;
    QDoubleSpinBox *spinA3;

    // Pola dla współczynników B
    QDoubleSpinBox *spinB1;
    QDoubleSpinBox *spinB2;
    QDoubleSpinBox *spinB3;

    QSpinBox *spinOpoznienie;
    QDoubleSpinBox *spinSzum;

    void zapiszZmiany();
};

#endif
