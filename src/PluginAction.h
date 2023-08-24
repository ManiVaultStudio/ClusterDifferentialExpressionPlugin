#pragma once

#include "actions/Actions.h"

class ClusterDifferentialExpressionPlugin;

class PluginAction : public hdps::gui::WidgetAction
{
public:
    PluginAction(QObject* parent, ClusterDifferentialExpressionPlugin* plugin, const QString& title);

protected:
    ClusterDifferentialExpressionPlugin*  _plugin;
};
