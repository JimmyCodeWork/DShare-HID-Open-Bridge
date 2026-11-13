/*
 * SPDX-FileCopyrightText: (C) 2026 DShare-HID contributors
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "OpenBridgeSetupDialog.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTextCursor>
#include <QVBoxLayout>

namespace deskflow::gui {
OpenBridgeSetupDialog::OpenBridgeSetupDialog(QWidget *parent) : QDialog(parent)
{
  setWindowTitle(tr("Set up open bridge"));
  resize(640, 480);
  auto *layout = new QVBoxLayout(this);
  auto *intro = new QLabel(tr("Connect an ESP32-C3 with a USB data cable. This creates a new device key, "
                              "builds and flashes the firmware, then verifies the board before saving its public key. "
                              "The board remains reflashable."), this);
  intro->setWordWrap(true);
  layout->addWidget(intro);
  auto *deps = new QLabel(tr("Requires Python 3 with platformio, cryptography and pyserial. "
                             "The first build may download tools and take several minutes."), this);
  deps->setWordWrap(true);
  layout->addWidget(deps);
  auto *row = new QHBoxLayout;
  row->addWidget(new QLabel(tr("Serial port:"), this));
  m_ports = new QComboBox(this);
  m_ports->setEditable(true);
  m_ports->setInsertPolicy(QComboBox::NoInsert);
  m_ports->setPlaceholderText(tr("Select or enter a port, for example COM3"));
  row->addWidget(m_ports, 1);
  m_refresh = new QPushButton(tr("Refresh"), this);
  row->addWidget(m_refresh);
  layout->addLayout(row);
  m_status = new QLabel(tr("Ready"), this);
  m_status->setWordWrap(true);
  layout->addWidget(m_status);
  m_log = new QPlainTextEdit(this);
  m_log->setReadOnly(true);
  m_log->setMaximumBlockCount(3000);
  layout->addWidget(m_log, 1);
  auto *buttons = new QHBoxLayout;
  buttons->addStretch();
  m_start = new QPushButton(tr("Set up / replace key"), this);
  m_close = new QPushButton(tr("Close"), this);
  buttons->addWidget(m_start);
  buttons->addWidget(m_close);
  layout->addLayout(buttons);
  connect(m_refresh, &QPushButton::clicked, this, [this] { refreshPorts(); });
  connect(m_start, &QPushButton::clicked, this, [this] { startProvisioning(); });
  connect(m_close, &QPushButton::clicked, this, &QDialog::reject);
  m_process.setProcessChannelMode(QProcess::MergedChannels);
  connect(&m_process, &QProcess::readyReadStandardOutput, this, [this] {
    m_log->moveCursor(QTextCursor::End);
    m_log->insertPlainText(QString::fromLocal8Bit(m_process.readAllStandardOutput()));
    m_log->moveCursor(QTextCursor::End);
  });
  connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
    if (error == QProcess::FailedToStart && m_running)
      finishProvisioning(false, tr("Could not start Python: %1").arg(m_process.errorString()));
  });
  connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
          [this](int code, QProcess::ExitStatus status) {
            if (!m_running) return;
            const bool success = status == QProcess::NormalExit && code == 0;
            finishProvisioning(success, success ? tr("Board verified and public key saved.")
                                                : tr("Setup failed. Review the log; you can retry."));
          });
  refreshPorts();
}

void OpenBridgeSetupDialog::refreshPorts()
{
  const QString previous = m_ports->currentText();
  QStringList ports;
#ifdef Q_OS_WIN
  QSettings registry(QStringLiteral("HKEY_LOCAL_MACHINE\\HARDWARE\\DEVICEMAP\\SERIALCOMM"), QSettings::NativeFormat);
  for (const auto &key : registry.allKeys()) ports.append(registry.value(key).toString());
#else
  const QDir dev(QStringLiteral("/dev"));
#ifdef Q_OS_MACOS
  const auto names = dev.entryList({QStringLiteral("cu.*")}, QDir::System | QDir::Files);
#else
  const auto names = dev.entryList({QStringLiteral("ttyACM*"), QStringLiteral("ttyUSB*")}, QDir::System | QDir::Files);
#endif
  for (const auto &name : names) ports.append(dev.filePath(name));
#endif
  ports.removeDuplicates();
  ports.sort();
  m_ports->clear();
  m_ports->addItems(ports);
  if (!previous.isEmpty()) m_ports->setEditText(previous);
}

bool OpenBridgeSetupDialog::extractFirmware()
{
  if (!m_sources.isValid()) return false;
  const QStringList files{QStringLiteral("platformio.ini"), QStringLiteral("prepare_identity.py"),
                          QStringLiteral("provision.py"), QStringLiteral("main.cpp"),
                          QStringLiteral("usb_frame_decoder.cpp"), QStringLiteral("usb_frame_decoder.h"),
                          QStringLiteral("hid_state.cpp"), QStringLiteral("hid_state.h")};
  for (const auto &name : files) {
    const QString dest = m_sources.filePath(name);
    if (!QFile::exists(dest) && !QFile::copy(QStringLiteral(":/openbridge/") + name, dest)) return false;
  }
  return true;
}

void OpenBridgeSetupDialog::startProvisioning()
{
  const QString port = m_ports->currentText().trimmed();
  if (port.isEmpty()) { m_status->setText(tr("Select or enter a serial port.")); return; }
  if (!extractFirmware()) { m_status->setText(tr("Could not prepare the bundled firmware files.")); return; }
#ifdef Q_OS_WIN
  const QString python = QStandardPaths::findExecutable(QStringLiteral("py"));
  const QStringList args{QStringLiteral("-3"), QStringLiteral("-u"), m_sources.filePath(QStringLiteral("provision.py")),
                         QStringLiteral("--port"), port};
#else
  const QString python = QStandardPaths::findExecutable(QStringLiteral("python3"));
  const QStringList args{QStringLiteral("-u"), m_sources.filePath(QStringLiteral("provision.py")),
                         QStringLiteral("--port"), port};
#endif
  if (python.isEmpty()) { m_status->setText(tr("Python 3 was not found. Install it and the required packages.")); return; }
  if (QMessageBox::question(this, tr("Confirm board"),
                            tr("The firmware on %1 will be replaced and a new device key created. "
                               "Confirm this is your ESP32-C3 board. No security eFuses will be enabled.").arg(port))
      != QMessageBox::Yes) return;
  m_log->clear();
  m_status->setText(tr("Building, flashing and verifying. Keep the board connected and do not close the application."));
  m_running = true;
  m_start->setEnabled(false); m_close->setEnabled(false); m_refresh->setEnabled(false); m_ports->setEnabled(false);
  auto env = QProcessEnvironment::systemEnvironment();
  env.insert(QStringLiteral("PYTHONUNBUFFERED"), QStringLiteral("1"));
  m_process.setProcessEnvironment(env);
  m_process.start(python, args);
}

void OpenBridgeSetupDialog::finishProvisioning(bool success, const QString &message)
{
  m_running = false;
  m_status->setText(message);
  if (!success) {
#ifdef Q_OS_WIN
    m_log->appendPlainText(tr("If Python packages are missing, run: py -m pip install platformio cryptography pyserial"));
#else
    m_log->appendPlainText(tr("If Python packages are missing, run: python3 -m pip install platformio cryptography pyserial"));
#endif
  }
  m_start->setEnabled(true); m_close->setEnabled(true); m_refresh->setEnabled(true); m_ports->setEnabled(true);
}
void OpenBridgeSetupDialog::reject() { if (!m_running) QDialog::reject(); }
void OpenBridgeSetupDialog::closeEvent(QCloseEvent *event)
{
  if (m_running) event->ignore(); else QDialog::closeEvent(event);
}
} // namespace deskflow::gui
