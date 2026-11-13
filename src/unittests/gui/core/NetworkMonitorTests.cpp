/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "NetworkMonitorTests.h"

#include "gui/core/NetworkMonitor.h"

#include <QElapsedTimer>
#include <QSignalSpy>
#include <QThread>

#include <atomic>

using namespace deskflow::gui;
void NetworkMonitorTests::testVirtualInterface()
{
  QVERIFY(NetworkMonitor::isVirtualInterface(QStringLiteral("vboxnet0")));
  QVERIFY(NetworkMonitor::isVirtualInterface(QStringLiteral("vboXnet0")));
  QVERIFY(NetworkMonitor::isVirtualInterface(QStringLiteral("vmnet0")));
  QVERIFY(NetworkMonitor::isVirtualInterface(QStringLiteral("docker-bridge")));
  QVERIFY(NetworkMonitor::isVirtualInterface(QStringLiteral("virbr0")));
  QVERIFY(NetworkMonitor::isVirtualInterface(QStringLiteral("veth0")));
  QVERIFY(NetworkMonitor::isVirtualInterface(QStringLiteral("br-0")));
  QVERIFY(NetworkMonitor::isVirtualInterface(QStringLiteral("tun0")));
  QVERIFY(NetworkMonitor::isVirtualInterface(QStringLiteral("utun0")));
  QVERIFY(NetworkMonitor::isVirtualInterface(QStringLiteral("awdl0")));
  QVERIFY(NetworkMonitor::isVirtualInterface(QStringLiteral("p2p0")));
  QVERIFY(NetworkMonitor::isVirtualInterface(QStringLiteral("llw0")));
  QVERIFY(NetworkMonitor::isVirtualInterface(QStringLiteral("anpi5")));
  QVERIFY(NetworkMonitor::isVirtualInterface(QStringLiteral("tap1")));

  QVERIFY(!NetworkMonitor::isVirtualInterface(QStringLiteral("eth0")));
  QVERIFY(!NetworkMonitor::isVirtualInterface(QStringLiteral("enp0s0f")));
  QVERIFY(!NetworkMonitor::isVirtualInterface(QStringLiteral("wifi")));
  QVERIFY(!NetworkMonitor::isVirtualInterface(QStringLiteral("wlan0")));
}

void NetworkMonitorTests::checksDoNotBlockTheCaller()
{
  const auto guiThread = QThread::currentThread();
  std::atomic<bool> ranOnGuiThread{false};
  NetworkMonitor monitor([&] {
    ranOnGuiThread = (QThread::currentThread() == guiThread);
    QThread::msleep(400); // a slow interface enumeration, as on some Windows machines
    return QStringList{QStringLiteral("192.168.1.10")};
  });
  QSignalSpy changed(&monitor, &NetworkMonitor::ipAddressesChanged);

  QElapsedTimer timer;
  timer.start();
  monitor.startMonitoring(60000);
  QVERIFY2(timer.elapsed() < 200, "startMonitoring() waited for the slow check");

  QVERIFY(changed.wait(5000));
  QCOMPARE(changed.first().first().toStringList(), QStringList{QStringLiteral("192.168.1.10")});
  QVERIFY(!ranOnGuiThread);
  monitor.stopMonitoring();
}

void NetworkMonitorTests::overlappingChecksAreSkipped()
{
  std::atomic<int> calls{0};
  NetworkMonitor monitor([&] {
    ++calls;
    QThread::msleep(300);
    return QStringList{QStringLiteral("10.0.0.2")};
  });
  QSignalSpy changed(&monitor, &NetworkMonitor::ipAddressesChanged);

  // checks every 20 ms, but each takes 300 ms: the ones that come due meanwhile are skipped
  monitor.startMonitoring(20);
  QVERIFY(changed.wait(5000));
  QTest::qWait(100);
  monitor.stopMonitoring();
  QVERIFY2(calls <= 2, qPrintable(QStringLiteral("%1 checks ran").arg(calls.load())));
}

QTEST_MAIN(NetworkMonitorTests)
