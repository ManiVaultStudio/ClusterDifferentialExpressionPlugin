#include "PluginAction.h"

#include "ClusterDifferentialExpressionPlugin.h"

PluginAction::PluginAction(QObject* parent, ClusterDifferentialExpressionPlugin* plugin, const QString& title) :
    WidgetAction(parent, title),
    _plugin(plugin)
{
    _plugin->getWidget().addAction(this);

    setText(title);
    setToolTip(title);
}
