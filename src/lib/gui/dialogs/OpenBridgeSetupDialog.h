/*
 * SPDX-FileCopyrightText: (C) 2026 DShare-HID contributors
 * SPDX-License-Identifier: GPL-2.0-only
 */
#pragma once

#include <QDialog>
#include <QProcess>
#include <QTemporaryDir>

class QCloseEvent;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;

namespace deskflow::gui {
class OpenBridgeSetupDialog : public QDialog
{
public:
  explicit OpenBridgeSetupDialog(QWidget *parent = nullptr);

protected:
  void reject() override;
  void closeEvent(QCloseEvent *event) override;

private:
  void refreshPorts();
  void startProvisioning();
  void finishProvisioning(bool success, const QString &message);
  bool extractFirmware();

  QTemporaryDir m_sources;
  QProcess m_process;
  QComboBox *m_ports;
  QLabel *m_status;
  QPlainTextEdit *m_log;
  QPushButton *m_refresh;
  QPushButton *m_start;
  QPushButton *m_close;
  bool m_running = false;
};
} // namespace deskflow::gui
