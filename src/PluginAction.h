#pragma once

#include "actions/Actions.h"

class ClusterDifferentialExpressionPlugin;
class ClusterDifferentialExpressionWidget;

class PluginAction : public hdps::gui::WidgetAction
{
public:
    PluginAction(QObject* parent, ClusterDifferentialExpressionPlugin* plugin, const QString& title);

    ClusterDifferentialExpressionWidget& getClusterDifferentialExpressionWidget();

protected:
    ClusterDifferentialExpressionPlugin*  _plugin;
};