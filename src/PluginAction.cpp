#include "PluginAction.h"

#include "ClusterDifferentialExpressionPlugin.h"


using namespace hdps::gui;

PluginAction::PluginAction(QObject* parent, ClusterDifferentialExpressionPlugin* plugin, const QString& title) :
    WidgetAction(parent),
    _plugin(plugin)
{
    _plugin->getWidget().addAction(this);

    setText(title);
    setToolTip(title);
}
