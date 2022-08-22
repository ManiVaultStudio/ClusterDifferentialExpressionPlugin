#include "SettingsAction.h"
#include "ClusterDifferentialExpressionPlugin.h"

#include <event/Event.h>



using namespace hdps;
using namespace hdps::gui;

SettingsAction::SettingsAction(ClusterDifferentialExpressionPlugin& dimensionsViewerPlugin) :
    WidgetAction(&dimensionsViewerPlugin),
    _differentialExpressionPlugin(dimensionsViewerPlugin),

    _spec(),
    _isLoading(false)
{
    _spec["modified"]           = 0;
    _spec["showDimensionNames"] = true;

   
}

ClusterDifferentialExpressionPlugin& SettingsAction::getDimensionsViewerPlugin()
{
    return _differentialExpressionPlugin;
}

QVariantMap SettingsAction::getSpec()
{
    

    return _spec;
}

SettingsAction::Widget::Widget(QWidget* parent, SettingsAction* settingsAction) :
    WidgetActionWidget(parent, settingsAction, State::Standard)
{
    setAutoFillBackground(true);

    auto layout = new QHBoxLayout();

    setLayout(layout);

    layout->setContentsMargins(3, 3, 3, 3);
    layout->setSpacing(3);

  
    layout->addStretch(1);
}