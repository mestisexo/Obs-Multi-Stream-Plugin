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

#include "multi-stream-dock.hpp"

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMetaObject>
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QUuid>
#include <QVBoxLayout>

#include <mutex>
#include <unordered_set>
#include <utility>

/* Context handed to libobs signal callbacks. It stays registered while the
 * output is running and is unregistered by releaseRuntime(), which also
 * disconnects the signals before retiring it. */
struct OutputRunCtx {
	MultiStreamDock *dock;
	QString id;
	quint64 runId;
};

namespace {

const char *kConfigFile = "multi-stream-ms.json";

/* libobs signal callbacks run on output threads and receive a raw
 * OutputRunCtx pointer. signal_handler_disconnect() does not wait for a
 * callback that is already running, so the context cannot be freed right
 * after disconnecting. postSignal() only touches the context while holding
 * g_ctxMutex, and releaseRuntime() removes it from the registry under the
 * same mutex before deleting it. */
std::mutex g_ctxMutex;
std::unordered_set<OutputRunCtx *> g_liveCtx;

void registerRunCtx(OutputRunCtx *ctx)
{
	std::lock_guard<std::mutex> lock(g_ctxMutex);
	g_liveCtx.insert(ctx);
}

void unregisterAndDeleteRunCtx(OutputRunCtx *ctx)
{
	if (!ctx)
		return;
	{
		std::lock_guard<std::mutex> lock(g_ctxMutex);
		g_liveCtx.erase(ctx);
	}
	/* No callback can be inside the critical section anymore, and future
	 * callbacks will not find the context in the registry. */
	delete ctx;
}

/* ---- libobs signal callbacks (called on libobs threads, NOT the UI thread).
 * They only copy plain values and queue the real work onto the UI thread. */

void postSignal(void *data, OutputSignal sig, int code, const char *err)
{
	auto *ctx = static_cast<OutputRunCtx *>(data);
	MultiStreamDock *dock;
	QString id;
	quint64 runId;
	QString error;

	{
		std::lock_guard<std::mutex> lock(g_ctxMutex);
		if (g_liveCtx.find(ctx) == g_liveCtx.end())
			return;
		dock = ctx->dock;
		id = ctx->id;
		runId = ctx->runId;
		error = err ? QString::fromUtf8(err) : QString();
	}

	QMetaObject::invokeMethod(
		dock, [dock, id, runId, sig, code, error]() { dock->handleOutputSignal(id, runId, sig, code, error); },
		Qt::QueuedConnection);
}

void cbStarting(void *data, calldata_t *)
{
	postSignal(data, OutputSignal::Starting, 0, nullptr);
}

void cbStart(void *data, calldata_t *)
{
	postSignal(data, OutputSignal::Started, 0, nullptr);
}

void cbReconnect(void *data, calldata_t *)
{
	postSignal(data, OutputSignal::Reconnecting, 0, nullptr);
}

void cbReconnectSuccess(void *data, calldata_t *)
{
	postSignal(data, OutputSignal::Reconnected, 0, nullptr);
}

void cbStop(void *data, calldata_t *cd)
{
	const int code = static_cast<int>(calldata_int(cd, "code"));
	const char *err = calldata_string(cd, "last_error");
	postSignal(data, OutputSignal::Stopped, code, err);
}

struct SignalBinding {
	const char *name;
	signal_callback_t cb;
};

const SignalBinding kSignals[] = {
	{"starting", cbStarting}, {"start", cbStart},
	{"reconnect", cbReconnect}, {"reconnect_success", cbReconnectSuccess},
	{"stop", cbStop},
};

QString stateText(DestState s)
{
	switch (s) {
	case DestState::Stopped:
		return QString::fromUtf8(obs_module_text("MultiStreamMS.State.Stopped"));
	case DestState::Connecting:
		return QString::fromUtf8(obs_module_text("MultiStreamMS.State.Connecting"));
	case DestState::Live:
		return QString::fromUtf8(obs_module_text("MultiStreamMS.State.Live"));
	case DestState::Reconnecting:
		return QString::fromUtf8(obs_module_text("MultiStreamMS.State.Reconnecting"));
	case DestState::Stopping:
		return QString::fromUtf8(obs_module_text("MultiStreamMS.State.Stopping"));
	case DestState::Error:
		return QString::fromUtf8(obs_module_text("MultiStreamMS.State.Error"));
	}
	return QString();
}

const char *stateColor(DestState s)
{
	switch (s) {
	case DestState::Live:
		return "#2e8b57";
	case DestState::Connecting:
	case DestState::Reconnecting:
	case DestState::Stopping:
		return "#b8860b";
	case DestState::Error:
		return "#c0392b";
	case DestState::Stopped:
		break;
	}
	return "#666666";
}

bool validServer(const QString &server)
{
	return server.startsWith(QStringLiteral("rtmp://"), Qt::CaseInsensitive) ||
	       server.startsWith(QStringLiteral("rtmps://"), Qt::CaseInsensitive);
}

} // namespace

MultiStreamDock::MultiStreamDock(QWidget *parent) : QWidget(parent)
{
	setObjectName(QStringLiteral("multiStreamMsDockContent"));
	setMinimumWidth(260);

	auto *rootLayout = new QVBoxLayout(this);
	rootLayout->setContentsMargins(8, 8, 8, 8);
	rootLayout->setSpacing(6);

	auto *headerLayout = new QHBoxLayout();
	headerLayout->setContentsMargins(0, 0, 0, 0);
	headerLayout->setSpacing(6);

	auto *heading = new QLabel(obs_module_text("MultiStreamMS.DockHeading"), this);
	QFont headingFont = heading->font();
	headingFont.setBold(true);
	heading->setFont(headingFont);

	addButton_ = new QPushButton(obs_module_text("MultiStreamMS.AddDestination"), this);
	addButton_->setAutoDefault(false);

	headerLayout->addWidget(heading);
	headerLayout->addStretch(1);
	headerLayout->addWidget(addButton_);
	rootLayout->addLayout(headerLayout);

	scrollArea_ = new QScrollArea(this);
	scrollArea_->setWidgetResizable(true);
	scrollArea_->setFrameShape(QFrame::NoFrame);

	listContainer_ = new QWidget(scrollArea_);
	listLayout_ = new QVBoxLayout(listContainer_);
	listLayout_->setContentsMargins(0, 0, 0, 0);
	listLayout_->setSpacing(6);

	emptyLabel_ = new QLabel(obs_module_text("MultiStreamMS.EmptyList"), listContainer_);
	emptyLabel_->setAlignment(Qt::AlignCenter);
	emptyLabel_->setWordWrap(true);
	emptyLabel_->setEnabled(false);

	listLayout_->addWidget(emptyLabel_);
	listLayout_->addStretch(1); /* rows are inserted before this stretch */

	scrollArea_->setWidget(listContainer_);
	rootLayout->addWidget(scrollArea_, 1);

	connect(addButton_, &QPushButton::clicked, this, &MultiStreamDock::onAddClicked);

	metricsTimer_.setInterval(1000);
	connect(&metricsTimer_, &QTimer::timeout, this, &MultiStreamDock::onMetricsTick);
	metricsTimer_.start();
}

MultiStreamDock::~MultiStreamDock()
{
	metricsTimer_.stop();
	stopAllNow();
}

/* ------------------------------------------------------------------ */
/* Persistence (per profile, obs_data_t JSON)                          */
/* ------------------------------------------------------------------ */

QString MultiStreamDock::configPath() const
{
	char *profilePath = obs_frontend_get_current_profile_path();
	if (!profilePath)
		return QString();
	const QString path = QString::fromUtf8(profilePath) + QStringLiteral("/") + QString::fromUtf8(kConfigFile);
	bfree(profilePath);
	return path;
}

void MultiStreamDock::loadConfig()
{
	dests_.clear();

	const QString path = configPath();
	if (path.isEmpty())
		return;

	obs_data_t *root = obs_data_create_from_json_file(path.toUtf8().constData());
	if (!root) {
		obs_log(LOG_INFO, "no destinations file for this profile (%s)", path.toUtf8().constData());
		return;
	}

	obs_data_array_t *arr = obs_data_get_array(root, "destinations");
	const size_t count = arr ? obs_data_array_count(arr) : 0;
	QSet<QString> usedIds;
	for (size_t i = 0; i < count; i++) {
		obs_data_t *item = obs_data_array_item(arr, i);
		auto d = std::make_unique<Destination>();
		d->id = QString::fromUtf8(obs_data_get_string(item, "id"));
		d->name = QString::fromUtf8(obs_data_get_string(item, "name"));
		d->server = QString::fromUtf8(obs_data_get_string(item, "server"));
		d->key = QString::fromUtf8(obs_data_get_string(item, "streamKey"));
		obs_data_release(item);

		if (d->server.isEmpty())
			continue;
		/* Ids double as internal output names, so they must be unique
		 * even if the JSON was edited by hand. */
		if (d->id.isEmpty() || usedIds.contains(d->id))
			d->id = QUuid::createUuid().toString(QUuid::WithoutBraces);
		usedIds.insert(d->id);
		dests_.push_back(std::move(d));
	}
	if (arr)
		obs_data_array_release(arr);
	obs_data_release(root);

	obs_log(LOG_INFO, "loaded %d destination(s) from profile", static_cast<int>(dests_.size()));
}

void MultiStreamDock::saveConfig() const
{
	const QString path = configPath();
	if (path.isEmpty())
		return;

	obs_data_t *root = obs_data_create();
	obs_data_set_int(root, "version", 1);

	obs_data_array_t *arr = obs_data_array_create();
	for (const auto &d : dests_) {
		obs_data_t *item = obs_data_create();
		obs_data_set_string(item, "id", d->id.toUtf8().constData());
		obs_data_set_string(item, "name", d->name.toUtf8().constData());
		obs_data_set_string(item, "server", d->server.toUtf8().constData());
		obs_data_set_string(item, "streamKey", d->key.toUtf8().constData());
		obs_data_array_push_back(arr, item);
		obs_data_release(item);
	}
	obs_data_set_array(root, "destinations", arr);
	obs_data_array_release(arr);

	/* Writes to a temp file and renames it over the target. */
	if (!obs_data_save_json_safe(root, path.toUtf8().constData(), "tmp", "bak"))
		obs_log(LOG_WARNING, "failed to save destinations to %s", path.toUtf8().constData());
	obs_data_release(root);
}

/* ------------------------------------------------------------------ */
/* UI                                                                  */
/* ------------------------------------------------------------------ */

MultiStreamDock::Destination *MultiStreamDock::find(const QString &id)
{
	for (auto &d : dests_)
		if (d->id == id)
			return d.get();
	return nullptr;
}

void MultiStreamDock::updateEmptyLabel()
{
	emptyLabel_->setVisible(dests_.empty());
}

void MultiStreamDock::buildRow(Destination &d)
{
	auto *row = new QFrame(listContainer_);
	row->setFrameShape(QFrame::StyledPanel);

	auto *v = new QVBoxLayout(row);
	v->setContentsMargins(6, 6, 6, 6);
	v->setSpacing(4);

	auto *top = new QHBoxLayout();
	top->setSpacing(6);

	d.nameLabel = new QLabel(d.name, row);
	d.statusLabel = new QLabel(row);
	d.toggleButton = new QPushButton(row);
	d.toggleButton->setAutoDefault(false);
	d.removeButton = new QPushButton(QStringLiteral("\u00d7"), row);
	d.removeButton->setAutoDefault(false);
	d.removeButton->setFixedWidth(28);
	d.removeButton->setToolTip(QString::fromUtf8(obs_module_text("MultiStreamMS.Remove")));

	top->addWidget(d.nameLabel, 1);
	top->addWidget(d.statusLabel);
	top->addWidget(d.toggleButton);
	top->addWidget(d.removeButton);

	d.metricsLabel = new QLabel(row);
	d.metricsLabel->setWordWrap(true);
	d.metricsLabel->setEnabled(false);

	v->addLayout(top);
	v->addWidget(d.metricsLabel);

	d.row = row;
	listLayout_->insertWidget(listLayout_->count() - 1, row);

	const QString id = d.id;
	connect(d.toggleButton, &QPushButton::clicked, this, [this, id]() { onToggleClicked(id); });
	connect(d.removeButton, &QPushButton::clicked, this, [this, id]() { removeDestination(id); });

	setState(d, DestState::Stopped);
	updateEmptyLabel();
}

void MultiStreamDock::setState(Destination &d, DestState state)
{
	d.state = state;

	d.statusLabel->setText(stateText(state));
	d.statusLabel->setStyleSheet(QStringLiteral("QLabel { padding: 2px 6px; border-radius: 3px; "
						    "background: %1; color: white; }")
					     .arg(QString::fromLatin1(stateColor(state))));

	switch (state) {
	case DestState::Stopped:
		d.toggleButton->setText(QString::fromUtf8(obs_module_text("MultiStreamMS.Start")));
		break;
	case DestState::Error:
		d.toggleButton->setText(QString::fromUtf8(obs_module_text("MultiStreamMS.Retry")));
		break;
	default:
		d.toggleButton->setText(QString::fromUtf8(obs_module_text("MultiStreamMS.Stop")));
		break;
	}
	d.toggleButton->setEnabled(state != DestState::Stopping);

	if (state == DestState::Error)
		d.metricsLabel->setText(d.lastError);
	else if (state == DestState::Stopped || state == DestState::Connecting)
		d.metricsLabel->clear();
}

void MultiStreamDock::clearAll()
{
	for (auto &d : dests_) {
		releaseRuntime(*d, true);
		if (d->row) {
			d->row->hide();
			d->row->deleteLater();
		}
	}
	dests_.clear();
	updateEmptyLabel();
}

void MultiStreamDock::onAddClicked()
{
	QDialog dlg(this);
	dlg.setWindowTitle(QString::fromUtf8(obs_module_text("MultiStreamMS.AddDestination")));

	auto *form = new QFormLayout(&dlg);
	auto *nameEdit = new QLineEdit(&dlg);
	auto *serverEdit = new QLineEdit(&dlg);
	auto *keyEdit = new QLineEdit(&dlg);
	serverEdit->setPlaceholderText(QStringLiteral("rtmp://live.example.com/app"));
	keyEdit->setEchoMode(QLineEdit::Password);

	form->addRow(QString::fromUtf8(obs_module_text("MultiStreamMS.Name")), nameEdit);
	form->addRow(QString::fromUtf8(obs_module_text("MultiStreamMS.Server")), serverEdit);
	form->addRow(QString::fromUtf8(obs_module_text("MultiStreamMS.StreamKey")), keyEdit);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
	form->addRow(buttons);
	connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

	while (dlg.exec() == QDialog::Accepted) {
		const QString server = serverEdit->text().trimmed();
		const QString key = keyEdit->text().trimmed();
		if (!validServer(server) || key.isEmpty()) {
			QMessageBox::warning(this, QString::fromUtf8(obs_module_text("MultiStreamMS.AddDestination")),
					     QString::fromUtf8(obs_module_text("MultiStreamMS.Error.InvalidServerOrKey")));
			continue; /* keep the dialog values and ask again */
		}
		QString name = nameEdit->text().trimmed();
		if (name.isEmpty())
			name = QString::fromUtf8(obs_module_text("MultiStreamMS.DefaultName"))
				       .arg(static_cast<int>(dests_.size()) + 1);
		addDestination(name, server, key);
		break;
	}
}

void MultiStreamDock::addDestination(const QString &name, const QString &server, const QString &key)
{
	auto d = std::make_unique<Destination>();
	d->id = QUuid::createUuid().toString(QUuid::WithoutBraces);
	d->name = name;
	d->server = server;
	d->key = key;

	Destination &ref = *d;
	dests_.push_back(std::move(d));
	buildRow(ref);
	saveConfig();
}

void MultiStreamDock::removeDestination(const QString &id)
{
	Destination *d = find(id);
	if (!d)
		return;

	const auto answer = QMessageBox::question(
		this, QString::fromUtf8(obs_module_text("MultiStreamMS.Remove")),
		QString::fromUtf8(obs_module_text("MultiStreamMS.ConfirmRemove")).arg(d->name));
	if (answer != QMessageBox::Yes)
		return;

	releaseRuntime(*d, true);
	if (d->row) {
		d->row->hide();
		d->row->deleteLater();
	}
	for (auto it = dests_.begin(); it != dests_.end(); ++it) {
		if ((*it)->id == id) {
			dests_.erase(it);
			break;
		}
	}
	saveConfig();
	updateEmptyLabel();
}

/* ------------------------------------------------------------------ */
/* Output lifecycle                                                    */
/* ------------------------------------------------------------------ */

void MultiStreamDock::onToggleClicked(const QString &id)
{
	Destination *d = find(id);
	if (!d)
		return;

	if (d->output)
		requestStop(*d);
	else
		startDestination(*d);
}

void MultiStreamDock::startDestination(Destination &d)
{
	if (d.output)
		return;

	const QString title = QString::fromUtf8(obs_module_text("MultiStreamMS.DockHeading"));

	if (!obs_frontend_streaming_active()) {
		QMessageBox::information(this, title,
					 QString::fromUtf8(obs_module_text("MultiStreamMS.Error.MainStreamNotLive")));
		return;
	}

	/* Borrow the main stream's encoders. We never create or configure an
	 * encoder; the new output simply takes its own reference to them. */
	obs_output_t *mainOutput = obs_frontend_get_streaming_output();
	obs_encoder_t *video = mainOutput ? obs_output_get_video_encoder(mainOutput) : nullptr;
	obs_encoder_t *audio = mainOutput ? obs_output_get_audio_encoder(mainOutput, 0) : nullptr;
	if (!mainOutput || !video || !audio) {
		if (mainOutput)
			obs_output_release(mainOutput);
		QMessageBox::information(this, title,
					 QString::fromUtf8(obs_module_text("MultiStreamMS.Error.MainStreamNotLive")));
		return;
	}

	/* libobs requires unique output/service names. The display name is not
	 * guaranteed to be unique (and may contain any character), so the
	 * internal name is built from the id and the run counter. */
	d.runId = ++runCounter_;
	const QByteArray name = QStringLiteral("%1-%2").arg(d.id).arg(d.runId).toUtf8();

	obs_data_t *svcSettings = obs_data_create();
	obs_data_set_string(svcSettings, "server", d.server.toUtf8().constData());
	obs_data_set_string(svcSettings, "key", d.key.toUtf8().constData());
	obs_service_t *service = obs_service_create("rtmp_custom", name.constData(), svcSettings, nullptr);
	obs_data_release(svcSettings);

	obs_output_t *output = obs_output_create("rtmp_output", name.constData(), nullptr, nullptr);

	if (!service || !output) {
		if (output)
			obs_output_release(output);
		if (service)
			obs_service_release(service);
		obs_output_release(mainOutput);
		d.lastError = QString::fromUtf8(obs_module_text("MultiStreamMS.Error.OutputCreationFailed"));
		setState(d, DestState::Error);
		return;
	}

	obs_output_set_video_encoder(output, video);
	obs_output_set_audio_encoder(output, audio, 0);
	obs_output_set_service(output, service);
	obs_output_set_reconnect_settings(output, 10, 10);
	obs_output_release(mainOutput); /* the new output now holds its own encoder refs */

	d.output = output;
	d.service = service;
	d.ctx = new OutputRunCtx{this, d.id, d.runId};
	registerRunCtx(d.ctx);
	d.lastError.clear();

	signal_handler_t *sh = obs_output_get_signal_handler(output);
	for (const auto &b : kSignals)
		signal_handler_connect(sh, b.name, b.cb, d.ctx);

	setState(d, DestState::Connecting);

	if (!obs_output_start(output)) {
		const char *err = obs_output_get_last_error(output);
		d.lastError = (err && *err) ? QString::fromUtf8(err)
					    : QString::fromUtf8(obs_module_text("MultiStreamMS.Error.StartFailed"));
		releaseRuntime(d, true);
		setState(d, DestState::Error);
	}
}

void MultiStreamDock::requestStop(Destination &d)
{
	if (!d.output || d.state == DestState::Stopping)
		return;

	setState(d, DestState::Stopping);
	obs_output_stop(d.output); /* the "stop" signal releases it */

	/* Safety net in case the stop signal never arrives. */
	const QString id = d.id;
	const quint64 run = d.runId;
	QTimer::singleShot(8000, this, [this, id, run]() {
		Destination *cur = find(id);
		if (cur && cur->output && cur->runId == run) {
			releaseRuntime(*cur, true);
			setState(*cur, DestState::Stopped);
		}
	});
}

void MultiStreamDock::releaseRuntime(Destination &d, bool force)
{
	if (d.output) {
		/* Disconnect first: guarantees no callback touches ctx afterwards. */
		signal_handler_t *sh = obs_output_get_signal_handler(d.output);
		for (const auto &b : kSignals)
			signal_handler_disconnect(sh, b.name, b.cb, d.ctx);

		if (force && obs_output_active(d.output))
			obs_output_force_stop(d.output);

		obs_output_release(d.output);
		d.output = nullptr;
	}
	if (d.service) {
		obs_service_release(d.service);
		d.service = nullptr;
	}
	OutputRunCtx *ctx = d.ctx;
	d.ctx = nullptr;
	d.runId = 0;
	unregisterAndDeleteRunCtx(ctx);
}

void MultiStreamDock::handleOutputSignal(const QString &id, quint64 runId, OutputSignal sig, int code,
					 const QString &lastError)
{
	Destination *d = find(id);
	/* Ignore events from a previous run or from a destination already gone. */
	if (!d || !d->output || d->runId != runId)
		return;

	switch (sig) {
	case OutputSignal::Starting:
		setState(*d, DestState::Connecting);
		break;
	case OutputSignal::Started:
	case OutputSignal::Reconnected:
		setState(*d, DestState::Live);
		break;
	case OutputSignal::Reconnecting:
		setState(*d, DestState::Reconnecting);
		break;
	case OutputSignal::Stopped:
		if (code != OBS_OUTPUT_SUCCESS) {
			d->lastError = !lastError.isEmpty()
					       ? lastError
					       : QString::fromUtf8(obs_module_text("MultiStreamMS.Error.Generic"))
							 .arg(code);
			releaseRuntime(*d, false);
			setState(*d, DestState::Error);
		} else {
			releaseRuntime(*d, false);
			setState(*d, DestState::Stopped);
		}
		break;
	}
}

void MultiStreamDock::onMetricsTick()
{
	for (auto &d : dests_) {
		if (!d->output || (d->state != DestState::Live && d->state != DestState::Reconnecting))
			continue;

		const double mb = static_cast<double>(obs_output_get_total_bytes(d->output)) / (1024.0 * 1024.0);
		const int dropped = obs_output_get_frames_dropped(d->output);
		const int congestion = static_cast<int>(obs_output_get_congestion(d->output) * 100.0f);

		d->metricsLabel->setText(QString::fromUtf8(obs_module_text("MultiStreamMS.Metrics"))
						 .arg(mb, 0, 'f', 1)
						 .arg(dropped)
						 .arg(congestion));
	}
}

/* ------------------------------------------------------------------ */
/* Called from frontend events                                         */
/* ------------------------------------------------------------------ */

void MultiStreamDock::reloadFromProfile()
{
	clearAll();
	loadConfig();
	for (auto &d : dests_)
		buildRow(*d);
	updateEmptyLabel();
}

void MultiStreamDock::stopAllNow()
{
	for (auto &d : dests_) {
		if (d->output) {
			releaseRuntime(*d, true);
			if (d->row)
				setState(*d, DestState::Stopped);
		}
	}
}

void MultiStreamDock::stopAllGraceful()
{
	for (auto &d : dests_)
		if (d->output)
			requestStop(*d);
}
