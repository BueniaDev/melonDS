/*
    Copyright 2016-2025 melonDS team

    This file is part of melonDS.

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#include <stdio.h>

#include "types.h"
#include "Config.h"
#include "Platform.h"
#include "main.h"

#include "AddonSettingsDialog.h"
#include "ui_AddonSettingsDialog.h"

using namespace melonDS::Platform;
namespace Platform = melonDS::Platform;

AddonSettingsDialog* AddonSettingsDialog::currentDlg = nullptr;

AddonSettingsDialog::AddonSettingsDialog(QWidget* parent) : QDialog(parent), ui(new Ui::AddonSettingsDialog)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    emuInstance = ((MainWindow*)parent)->getEmuInstance();

    auto& cfg = emuInstance->getLocalConfig();
    ui->sBxMagicReader->setValue(cfg.GetInt("MagicReader.Index"));

#define SET_ORIGVAL(type, val) \
    for (type* w : findChildren<type*>(nullptr)) \
        w->setProperty("user_originalValue", w->val());

    SET_ORIGVAL(QSpinBox, value);

#undef SET_ORIGVAL
}

AddonSettingsDialog::~AddonSettingsDialog()
{
    delete ui;
}

void AddonSettingsDialog::done(int r)
{
    if (!((MainWindow*)parent())->getEmuInstance())
    {
        QDialog::done(r);
        closeDlg();
        return;
    }

    if (r == QDialog::Accepted)
    {
        bool modified = false;

#define CHECK_ORIGVAL(type, val) \
        if (!modified) for (type* w : findChildren<type*>(nullptr)) \
        {                        \
            QVariant v = w->val();                   \
            if (v != w->property("user_originalValue")) \
            {                    \
                modified = true; \
                break;                   \
            }\
        }

        CHECK_ORIGVAL(QSpinBox, value);

        if (modified)
        {
            auto& cfg = emuInstance->getLocalConfig();
            cfg.SetInt("MagicReader.Index", ui->sBxMagicReader->value());

            Config::Save();
        }
    }

#undef CHECK_ORIGVAL

    QDialog::done(r);

    closeDlg();
}