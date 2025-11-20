
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

#ifndef ADDONSETTINGSDIALOG_H
#define ADDONSETTINGSDIALOG_H

#include <QDialog>

namespace Ui { class AddonSettingsDialog; }
class AddonSettingsDialog;

class EmuInstance;

class AddonSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddonSettingsDialog(QWidget* parent);
    ~AddonSettingsDialog();

    static AddonSettingsDialog* currentDlg;
    static AddonSettingsDialog* openDlg(QWidget* parent)
    {
        if (currentDlg)
        {
            currentDlg->activateWindow();
            return currentDlg;
        }

        currentDlg = new AddonSettingsDialog(parent);
        currentDlg->open();
        return currentDlg;
    }
    static void closeDlg()
    {
        currentDlg = nullptr;
    }

    static bool needsReset;

private slots:
    void done(int r);

private:
    Ui::AddonSettingsDialog* ui;
    EmuInstance* emuInstance;
};

#endif // ADDONSETTINGSDIALOG_H