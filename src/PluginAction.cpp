#include "PluginAction.h"

#include "ClusterDifferentialExpressionPlugin.h"
#include "ClusterDifferentialExpressionWidget.h"

using namespace hdps::gui;

PluginAction::PluginAction(QObject* parent, ClusterDifferentialExpressionPlugin* plugin, const QString& title) :
    WidgetAction(parent),
    _plugin(plugin)
{
    _plugin->getClusterDifferentialExpressionWidget().addAction(this);

    setText(title);
    setToolTip(title);
}

ClusterDifferentialExpressionWidget& PluginAction::getClusterDifferentialExpressionWidget()
{
    Q_ASSERT(_plugin != nullptr);

    return _plugin->getClusterDifferentialExpressionWidget();
}