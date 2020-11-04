/**
**  This file is part of SocketStreamExtension for OCTproZ.
**  Copyright (C) 2020 Miroslav Zabic
**
**  SocketStreamExtension is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program. If not, see http://www.gnu.org/licenses/.
**
****
** Author:	Miroslav Zabic
** Contact:	zabic
**			at
**			iqo.uni-hannover.de
****
**/

#ifndef SOCKETSTREAMEXTENSIONFORM_H
#define SOCKETSTREAMEXTENSIONFORM_H

#define HOST_IP "host_ip"
#define HOST_PORT "host_port"

#include <QWidget>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QRadioButton>
#include <QLineEdit>



namespace Ui {
class SocketStreamExtensionForm;
}

struct SocketStreamExtensionParameters {
	QString ip;
	qint16 port;
};

class SocketStreamExtensionForm : public QWidget
{
	Q_OBJECT

public:
	explicit SocketStreamExtensionForm(QWidget *parent = 0);
	~SocketStreamExtensionForm();

	void setSettings(QVariantMap settings);
	void getSettings(QVariantMap* settings);

	Ui::SocketStreamExtensionForm* ui;

private:
	void findGuiElements();
	void connectGuiElementsToUpdateParams();
	void initValidators();

	SocketStreamExtensionParameters parameters;
	QList<QCheckBox*> checkBoxes;
	QList<QDoubleSpinBox*> doubleSpinBoxes;
	QList<QSpinBox*> spinBoxes;
	QList<QComboBox*> comboBoxes;
	QList<QRadioButton*> radioButtons;
	QList<QLineEdit*> lineEdits;


public slots:
	void updateParams();
	void onStartPressed();
	void onStopPressed();
	void enableButtonsForBroadcastingEnabledState(bool braodcastingActive);

signals:
	void paramsChanged(SocketStreamExtensionParameters params);

	void error(QString);
	void info(QString);
	void startPressed();
	void stopPressed();

};

#endif // SOCKETSTREAMEXTENSIONFORM_H
