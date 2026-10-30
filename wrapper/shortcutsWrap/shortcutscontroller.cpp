#include "shortcutscontroller.h"

#include <QKeySequence>

namespace {

QStringList toQt(const std::vector<std::string>& keys)
{
    QStringList list;
    for (const std::string& key : keys) list << QString::fromStdString(key);
    return list;
}

} // namespace

ShortcutsController::ShortcutsController(QObject* parent)
    : QObject(parent)
{
    m_settings.beginGroup("shortcuts");
    for (const QString& id : m_settings.childKeys()) {
        std::vector<std::string> keys;
        for (const QString& key : m_settings.value(id).toStringList()) keys.push_back(key.toStdString());
        m_map.restore(id.toStdString(), keys);
    }
    m_settings.endGroup();
}

QVariantMap ShortcutsController::keys() const
{
    QVariantMap map;
    for (const ShortcutAction& action : m_map.actions())
        map.insert(QString::fromStdString(action.id), toQt(m_map.keysOf(action.id)));
    return map;
}

QVariantList ShortcutsController::actions() const
{
    QVariantList list;
    for (const ShortcutAction& action : m_map.actions()) {
        list.append(QVariantMap {
            {"id", QString::fromStdString(action.id)},
            {"title", QString::fromStdString(action.title)},
            {"keys", toQt(m_map.keysOf(action.id)).join(QStringLiteral(", "))},
        });
    }
    return list;
}

QString ShortcutsController::label(const QString& id) const
{
    const auto& keys = m_map.keysOf(id.toStdString());
    return keys.empty() ? QString() : QString::fromStdString(keys.front());
}

void ShortcutsController::save()
{
    m_settings.remove("shortcuts");
    m_settings.beginGroup("shortcuts");
    for (const auto& [id, keys] : m_map.custom()) m_settings.setValue(QString::fromStdString(id), toQt(keys));
    m_settings.endGroup();
    emit changed();
}

void ShortcutsController::set(const QString& id, const QString& key)
{
    m_map.set(id.toStdString(), key.toStdString());
    save();
}

void ShortcutsController::resetAll()
{
    m_map.resetAll();
    save();
}

QString ShortcutsController::sequenceFor(int key, int modifiers) const
{
    if (key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt || key == Qt::Key_Meta
        || key == Qt::Key_AltGr || key == 0 || key == Qt::Key_unknown) return {};
    return QKeySequence(key | (modifiers & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)))
        .toString(QKeySequence::PortableText);
}
