#pragma once

#include <actions/WidgetAction.h>

using namespace hdps;
using namespace hdps::gui;

class ClusterDifferentialExpressionPlugin;

class SettingsAction : public WidgetAction
{
protected:

    class Widget : public hdps::gui::WidgetActionWidget {
    public:
        Widget(QWidget* parent, SettingsAction* settingsAction);
    };

    QWidget* getWidget(QWidget* parent, const std::int32_t& widgetFlags) override {
        return new SettingsAction::Widget(parent, this);
    };

public:
    SettingsAction(ClusterDifferentialExpressionPlugin& differentialExpressionPlugin);

    ClusterDifferentialExpressionPlugin& getDimensionsViewerPlugin();

    QVariantMap getSpec();

    std::int32_t getModified() const { return _spec["modified"].toInt(); }
    void setModified() { _spec["modified"] = _spec["modified"].toInt() + 1; }

public: // Action getters

 
protected:
    ClusterDifferentialExpressionPlugin&    _differentialExpressionPlugin;
    
    QVariantMap                     _spec;
    bool                            _isLoading;

    //friend class ChannelAction;
};