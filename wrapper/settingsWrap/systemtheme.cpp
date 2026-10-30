#include "systemtheme.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QGuiApplication>
#include <QStyleHints>

namespace {

const auto kService = QStringLiteral("org.freedesktop.portal.Desktop");
const auto kPath = QStringLiteral("/org/freedesktop/portal/desktop");
const auto kInterface = QStringLiteral("org.freedesktop.portal.Settings");
const auto kGroup = QStringLiteral("org.freedesktop.appearance");
const auto kKey = QStringLiteral("color-scheme");

constexpr uint kPrefersDark = 1;
constexpr uint kPrefersLight = 2;

} // namespace

SystemTheme::SystemTheme(QObject* parent)
    : QObject(parent)
{
    applyQtValue();
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, &SystemTheme::applyQtValue);

    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.connect(kService, kPath, kInterface, QStringLiteral("SettingChanged"), this,
                SLOT(settingChanged(QString,QString,QDBusVariant)));

    QDBusMessage call = QDBusMessage::createMethodCall(kService, kPath, kInterface, QStringLiteral("ReadOne"));
    call << kGroup << kKey;
    auto* watcher = new QDBusPendingCallWatcher(bus.asyncCall(call), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
        const QDBusPendingReply<QVariant> reply = *watcher;
        if (!reply.isError()) applyPortalValue(reply.value());
        watcher->deleteLater();
    });
}

void SystemTheme::settingChanged(const QString& group, const QString& key, const QDBusVariant& value)
{
    if (group == kGroup && key == kKey) applyPortalValue(value.variant());
}

void SystemTheme::applyPortalValue(const QVariant& value)
{
    QVariant inner = value;
    while (inner.canConvert<QDBusVariant>()) inner = inner.value<QDBusVariant>().variant();

    const uint scheme = inner.toUInt();
    m_portalAnswered = scheme == kPrefersDark || scheme == kPrefersLight;
    if (m_portalAnswered) setDark(scheme == kPrefersDark);
    else applyQtValue();
}

void SystemTheme::applyQtValue()
{
    if (m_portalAnswered) return;
    setDark(QGuiApplication::styleHints()->colorScheme() != Qt::ColorScheme::Light);
}

void SystemTheme::setDark(bool dark)
{
    if (dark == m_dark) return;
    m_dark = dark;
    emit changed();
}
