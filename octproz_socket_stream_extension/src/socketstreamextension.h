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

#ifndef SOCKETSTREAMEXTENSION_H
#define SOCKETSTREAMEXTENSION_H

#include <QCoreApplication>
#include <QThread>
#include "octproz_devkit.h"
#include "socketstreamextensionform.h"
#include "broadcaster.h"

class SocketStreamExtension : public Extension
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID Extension_iid)
	Q_INTERFACES(Extension)
	QThread broadcasterThread;

public:
	SocketStreamExtension();
	~SocketStreamExtension();

	virtual QWidget* getWidget() override;
	virtual void activateExtension() override;
	virtual void deactivateExtension() override;
	virtual void settingsLoaded(QVariantMap settings) override;


private:
	SocketStreamExtensionForm* form;
	SocketStreamExtensionParameters params;
	bool widgetDisplayed;
	bool active;

	Broadcaster* broadcastServer;



public slots:
	void setParams(SocketStreamExtensionParameters params);
	void storeParameters();

	virtual void rawDataReceived(void* buffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) override;
	virtual void processedDataReceived(void* buffer, unsigned int bitDepth, unsigned int samplesPerLine, unsigned int linesPerFrame, unsigned int framesPerBuffer, unsigned int buffersPerVolume, unsigned int currentBufferNr) override;

signals:

};

#endif // SOCKETSTREAMEXTENSION_H
