/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QHostAddress>
#include <QList>
#include <QObject>

#include <functional>

class QTimer;
template <typename T> class QFutureWatcher;
namespace deskflow::gui {

/**
 * @brief Monitor network activity changes and provide IP address updates
 *
 * The NetworkMonitor class monitors IP address changes
 * It periodically checks network status and emits signals when changes are detected.
 *
 * Modified 2026-11-13: the addresses are read on a worker thread. Enumerating network
 * interfaces can take hundreds of milliseconds on Windows (virtual adapters from Hyper-V, WSL,
 * VPNs), which froze the GUI briefly every check.
 */
class NetworkMonitor : public QObject
{
  Q_OBJECT

public:
  /**
   * @brief Construct a new NetworkMonitor object
   * @param parent Parent QObject
   */
  explicit NetworkMonitor(QObject *parent = nullptr);

  using AddressProvider = std::function<QStringList()>;
  /**
   * @brief Construct a NetworkMonitor that reads addresses with @p provider (for tests)
   * @param provider Called on a worker thread; must be thread-safe
   */
  NetworkMonitor(AddressProvider provider, QObject *parent = nullptr);

  /**
   * @brief Destroy the NetworkMonitor object
   */
  ~NetworkMonitor() override;

  /**
   * @brief Start network monitoring
   * @param intervalMs Check interval in milliseconds, default 3000ms (3 seconds)
   */
  void startMonitoring(int intervalMs = 3000);

  /**
   * @brief Stop network monitoring
   */
  void stopMonitoring();

  /**
   * @brief Get list of all available IP addresses (excluding loopback addresses)
   * @return IP address list
   */
  static QStringList validAddresses();

  /**
   * @brief Check if a network interface is virtual
   * @param interfaceName Network interface name
   * @return true if it's a virtual interface
   */
  static bool isVirtualInterface(const QString &interfaceName);

Q_SIGNALS:
  /**
   * @brief Emitted when IP addresses change
   * @param addresses New IP address list
   */
  void ipAddressesChanged(const QStringList &addresses);

private:
  void setIpAddresses(const QStringList &newAddresses);

  /**
   * @brief Update current network status
   */
  void updateNetworkState();
  void onCheckFinished();

  AddressProvider m_provider;                    ///< Reads the addresses, on a worker thread
  QFutureWatcher<QStringList> *m_watcher;        ///< The check in progress, if any
  QTimer *m_checkTimer;        ///< Timer for periodic network checks
  QStringList m_lastAddresses; ///< Last known IP addresses
  bool m_isMonitoring = false; ///< Flag indicating if monitoring is active
};

} // namespace deskflow::gui
