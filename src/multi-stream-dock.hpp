/*
Multi-Stream MS
Copyright (C) 2026 Multi-Stream MS contributors

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

#include <obs.h>

#include <QString>
#include <QTimer>
#include <QWidget>

#include <memory>
#include <vector>

class QLabel;
class QPushButton;
class QScrollArea;
class QVBoxLayout;

struct OutputRunCtx; /* defined in multi-stream-dock.cpp */

/* Output events coming from libobs signal handlers (non-UI threads). */
enum class OutputSignal { Starting, Started, Reconnecting, Reconnected, Stopped };

enum class DestState { Stopped, Connecting, Live, Reconnecting, Stopping, Error };

/* Dock that manages extra RTMP destinations. Every destination is an
 * "rtmp_output" that is handed the SAME video/audio encoder instances as the
 * main stream, so nothing is encoded twice. */
class MultiStreamDock : public QWidget {
	Q_OBJECT

public:
	explicit MultiStreamDock(QWidget *parent = nullptr);
	~MultiStreamDock() override;

	/* Called from the frontend event callback (UI thread). */
	void reloadFromProfile();
	void stopAllNow();
	void stopAllGraceful();

	/* Called (queued, UI thread) from libobs signal callbacks. */
	void handleOutputSignal(const QString &id, quint64 runId, OutputSignal sig, int code,
				const QString &lastError);

private slots:
	void onAddClicked();
	void onMetricsTick();

private:
	struct Destination {
		QString id;
		QString name;
		QString server;
		QString key;

		DestState state = DestState::Stopped;
		QString lastError;

		QWidget *row = nullptr;
		QLabel *nameLabel = nullptr;
		QLabel *statusLabel = nullptr;
		QLabel *metricsLabel = nullptr;
		QPushButton *toggleButton = nullptr;
		QPushButton *removeButton = nullptr;

		obs_output_t *output = nullptr;
		obs_service_t *service = nullptr;
		OutputRunCtx *ctx = nullptr;
		quint64 runId = 0;
	};

	Destination *find(const QString &id);
	QString configPath() const;
	void loadConfig();
	void saveConfig() const;

	void buildRow(Destination &d);
	void clearAll();
	void updateEmptyLabel();
	void setState(Destination &d, DestState state);

	void addDestination(const QString &name, const QString &server, const QString &key);
	void removeDestination(const QString &id);
	void onToggleClicked(const QString &id);
	void startDestination(Destination &d);
	void requestStop(Destination &d);
	void releaseRuntime(Destination &d, bool force);

	std::vector<std::unique_ptr<Destination>> dests_;
	quint64 runCounter_ = 0;

	QPushButton *addButton_ = nullptr;
	QScrollArea *scrollArea_ = nullptr;
	QWidget *listContainer_ = nullptr;
	QVBoxLayout *listLayout_ = nullptr;
	QLabel *emptyLabel_ = nullptr;
	QTimer metricsTimer_;
};
