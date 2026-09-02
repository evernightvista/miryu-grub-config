/*
    SPDX-FileCopyrightText: 2026 Evernight Vista Team <13278297951@sina.cn>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef GRUBCONFIGKAUTHHELPER_H
#define GRUBCONFIGKAUTHHELPER_H

#include <KAuth/ActionReply>

#include <QObject>
#include <QVariantMap>

// KAuth helper that runs as root to merge GRUB2 settings into
// /etc/default/grub, update BLS entries via grubby, and regenerate
// grub.cfg.  Replaces the old pkexec-based standalone helper with
// an asynchronous KAuth helper so the calling UI never blocks.
//
// The single action "save" receives the generated drop-in content
// through the QVariantMap argument "content" and returns an
// ActionReply indicating success or failure.
class GrubConfigKAuthHelper : public QObject
{
    Q_OBJECT
public:
    explicit GrubConfigKAuthHelper(QObject *parent = nullptr);

public Q_SLOTS:
    KAuth::ActionReply save(const QVariantMap &args);
};

#endif // GRUBCONFIGKAUTHHELPER_H
