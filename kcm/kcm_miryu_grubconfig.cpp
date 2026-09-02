/*
    SPDX-FileCopyrightText: 2026 Evernight Vista Team <13278297951@sina.cn>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

// KDE System Settings module that embeds the full Miryu GRUB2 Boot Config GUI
// directly (the boot-menu and kernel-parameter controls), instead of merely
// launching the standalone application. Registered under the "System
// Administration" section via X-KDE-System-Settings-Parent-Category in
// kcm_miryu_grubconfig.json.
//
// Button layout: only Apply is declared in setButtons(), which makes KDE
// System Settings show the standard pair at the bottom of the page --
// "Reset" on the left (reverts unsaved changes by calling load()) and
// "Apply" on the right (calls save()). No "Restore Defaults" button is
// shown because the Default flag is intentionally omitted.
//
// The Apply button icon is overridden to dialog-password (a key icon that
// signals "applying changes requires administrator authentication") through
// a property-based search that works with both QWidget (QPushButton) and
// QML (QQuickItem) button objects.
//
// The System Settings framework may create the button bar asynchronously
// and — in KDE Plasma 6 — the main window may be a QQuickWindow (a QWindow,
// not a QWidget), so both the QWidget and the QWindow hierarchies are
// searched. A two-phase timer first polls quickly to discover the button,
// then switches to a slower maintenance interval that re-applies the icon
// in case the framework resets it when the button's enabled state changes.

#include "grubconfigwidget.h"

#include <KCModule>
#include <KPluginFactory>
#include <KPluginMetaData>

#include <QApplication>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QIcon>
#include <QMetaType>
#include <QObject>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Returns true when *text* contains the localised word for "Apply" in any of
// the locales supported by the KCM's translation catalog (en, de, fr, ja, ko,
// zh_CN, zh_TW). Mnemonic markers (&) are ignored so that "&Apply" still
// matches.
static bool isApplyButtonText(const QString &text)
{
    // English
    if (text.contains(QLatin1String("Apply"), Qt::CaseInsensitive)) {
        return true;
    }

    // Chinese (Simplified) — 应用
    if (text.contains(QString::fromUtf8("\xe5\xba\x94\xe7\x94\xa8"))) {
        return true;
    }
    // Chinese (Traditional) — 套用 / 應用
    if (text.contains(QString::fromUtf8("\xe5\xa5\x97\xe7\x94\xa8"))) {  // 套用
        return true;
    }
    if (text.contains(QString::fromUtf8("\xe6\x87\x89\xe7\x94\xa8"))) {  // 應用
        return true;
    }
    // Japanese — 適用
    if (text.contains(QString::fromUtf8("\xe9\x81\xa9\xe7\x94\xa8"))) {
        return true;
    }
    // Korean — 적용
    if (text.contains(QString::fromUtf8("\xec\xa0\x81\xec\x9a\xa9"))) {
        return true;
    }
    // German — Anwenden
    if (text.contains(QLatin1String("Anwenden"), Qt::CaseInsensitive)) {
        return true;
    }
    // French — Appliquer
    if (text.contains(QLatin1String("Appliquer"), Qt::CaseInsensitive)) {
        return true;
    }

    return false;
}

// Try to set the auth icon on a single QObject that is assumed to be a
// button. Three property paths are attempted, covering QPushButton, Kirigami
// buttons, and QtQuick.Controls 2 buttons. Returns true when the icon was
// successfully changed.
static bool setAuthIconOnObject(QObject *obj, const QIcon &authIcon,
                                const QString &authIconName)
{
    const QMetaObject *mo = obj->metaObject();

    // The object must expose a "text" property.
    if (mo->indexOfProperty("text") < 0) {
        return false;
    }

    if (!isApplyButtonText(obj->property("text").toString())) {
        return false;
    }

    const int iconIdx = mo->indexOfProperty("icon");

    // --- Path 1: QWidget (QPushButton) — "icon" expects a QIcon ---
    if (iconIdx >= 0) {
        QVariant iconVar = obj->property("icon");
        if (iconVar.userType() == QMetaType::QIcon) {
            if (obj->setProperty("icon", QVariant::fromValue(authIcon))) {
                return true;
            }
        }
    }

    // --- Path 2: Kirigami — "iconName" is a plain string ---
    if (mo->indexOfProperty("iconName") >= 0) {
        if (obj->setProperty("iconName", authIconName)) {
            return true;
        }
    }

    // --- Path 3: QtQuick.Controls 2 — "icon" is a grouped property
    //     (QQuickIcon) whose "name" sub-property holds the icon-theme name.
    if (iconIdx >= 0) {
        QVariant iv = obj->property("icon");
        if (iv.isValid() && iv.userType() == QMetaType::QObjectStar) {
            QObject *iconObj = iv.value<QObject *>();
            if (iconObj && iconObj->setProperty("name", authIconName)) {
                return true;
            }
        }
    }

    return false;
}

// ---------------------------------------------------------------------------
// KCM class
// ---------------------------------------------------------------------------

class MiryuGrubConfigKcm : public KCModule
{
    Q_OBJECT
public:
    MiryuGrubConfigKcm(QObject *parent, const KPluginMetaData &data)
        : KCModule(parent, data)
    {
        setButtons(KCModule::Apply);

        auto *layout = new QVBoxLayout(widget());
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        m_panel = new GrubConfigWidget(widget());
        layout->addWidget(m_panel);

        connect(m_panel, &GrubConfigWidget::changed, this, [this](bool dirty) {
            setNeedsSave(dirty);
            // Re-apply the Apply button icon after a state change, because
            // the System Settings framework may re-set the default icon
            // when the button transitions between enabled and disabled.
            tryApplyAuthIcon();
        });

        connect(m_panel, &GrubConfigWidget::saveSucceeded, this, [this]() {
            setNeedsSave(false);
        });
        connect(m_panel, &GrubConfigWidget::saveFailed, this, [this](const QString &) {
            setNeedsSave(m_panel->isDirty());
        });

        setNeedsSave(false);

        // Two-phase timer to discover and then maintain the Apply button icon.
        //
        // Phase 1 — fast polling (300 ms, up to ~10 s) to find the button and
        //   set its icon. The System Settings framework creates the button bar
        //   asynchronously, so a one-shot search is unreliable.
        // Phase 2 — slow maintenance polling (2 s) that keeps re-applying the
        //   icon, because the framework may reset it when the button's
        //   enabled state or the visual theme changes.
        m_iconTimer = new QTimer(this);
        m_iconTimer->setSingleShot(false);
        m_iconTimer->setInterval(300);
        m_iconRetryCount = 0;
        connect(m_iconTimer, &QTimer::timeout, this, [this]() {
            if (tryApplyAuthIcon()) {
                // Icon was set — switch to slow maintenance polling.
                if (m_iconTimer->interval() != 2000) {
                    m_iconTimer->setInterval(2000);
                    m_iconRetryCount = 0;
                }
            } else if (m_iconTimer->interval() == 300 && ++m_iconRetryCount > 33) {
                // Button not found after ~10 s — give up.
                m_iconTimer->stop();
            }
        });
        m_iconTimer->start(300);
    }

    ~MiryuGrubConfigKcm() override
    {
        if (m_iconTimer) {
            m_iconTimer->stop();
        }
    }

    void load() override
    {
        m_panel->loadFromConfig();
        setNeedsSave(false);
        tryApplyAuthIcon();
    }

    void save() override
    {
        m_panel->applySettingsAsync();
        setNeedsSave(false);
        tryApplyAuthIcon();
    }

private:
    GrubConfigWidget *m_panel = nullptr;
    QTimer *m_iconTimer = nullptr;
    int m_iconRetryCount = 0;

    // Search every relevant QObject for the Apply button and set its icon to
    // dialog-password. Returns true when a matching button was found and its
    // icon was changed.
    //
    // Two hierarchies are searched:
    //   1. qApp->topLevelWidgets() — QWidget windows that may contain a
    //      QDialogButtonBox with a QPushButton for "Apply".
    //   2. QGuiApplication::topLevelWindows() — QWindow instances (including
    //      QQuickWindow / QQuickView) that do NOT appear in topLevelWidgets().
    //      In KDE Plasma 6 the System Settings main window may be a
    //      QQuickWindow, so the QML button bar is only reachable through
    //      this path.
    bool tryApplyAuthIcon()
    {
        const QIcon authIcon = QIcon::fromTheme(QStringLiteral("dialog-password"));
        const QString authIconName = QStringLiteral("dialog-password");

        // --- QWidget hierarchy -------------------------------------------------

        for (QWidget *top : qApp->topLevelWidgets()) {
            // Preferred path: use QDialogButtonBox::button(Apply) for
            // locale-independent discovery of the Apply button.
            const auto boxes = top->findChildren<QDialogButtonBox *>();
            for (QDialogButtonBox *box : boxes) {
                QPushButton *btn = box->button(QDialogButtonBox::Apply);
                if (btn) {
                    btn->setIcon(authIcon);
                    return true;
                }
            }

            // Fallback: property-based search (catches non-standard button
            // bars and individual QPushButtons).
            const auto objs = top->findChildren<QObject *>();
            for (QObject *obj : objs) {
                if (setAuthIconOnObject(obj, authIcon, authIconName)) {
                    return true;
                }
            }
        }

        // --- QWindow hierarchy (QML / QQuickWindow) ---------------------------

        for (QWindow *window : QGuiApplication::topLevelWindows()) {
            const auto objs = window->findChildren<QObject *>();
            for (QObject *obj : objs) {
                if (setAuthIconOnObject(obj, authIcon, authIconName)) {
                    return true;
                }
            }
        }

        return false;
    }
};

K_PLUGIN_CLASS_WITH_JSON(MiryuGrubConfigKcm, "kcm_miryu_grubconfig.json")

#include "kcm_miryu_grubconfig.moc"
