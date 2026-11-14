/*
 * SPDX-FileCopyrightText: 2026 DShare-HID Open Bridge contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class PointerHomingTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void splitMoveKeepsTheTotal();
  void splitMoveStaysWithinTheStep();
  void splitMoveOfNothingIsEmpty();
  void splitMoveHandlesLargeAndInvalidInput();
  void homingStartsInTheTopLeftCorner();
  void homingThenReachesTheTarget();
  void homingToTheCornerOnlyHomes();
  void homingClampsNegativeTargets();
  void absoluteCoversTheWholeRange();
  void absoluteClampsToTheScreen();
  void recalibrateShortcut();
};
