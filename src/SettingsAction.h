#pragma once

/*
#include "PluginAction.h"
#include "LoadedDatasetsAction.h"

#include "actions/WidgetActionStateWidget.h"


using namespace hdps::gui;

class ScatterplotPlugin;

class SettingsAction : public PluginAction
{
public:
    class SpacerWidget : public QWidget {
    public:
        enum class Type {
            Divider,
            Spacer
        };

    public:
        SpacerWidget(const Type& type = Type::Spacer);

        static Type getType(const WidgetActionWidget::State& widgetTypeLeft, const WidgetActionWidget::State& widgetTypeRight);
        static Type getType(const hdps::gui::WidgetActionStateWidget* stateWidgetLeft, const hdps::gui::WidgetActionStateWidget* stateWidgetRight);

        void setType(const Type& type);
        static std::int32_t getWidth(const Type& type);

    protected:
        Type            _type;
        QHBoxLayout*    _layout;
       // QFrame*         _verticalLine;
    };

protected: // Widget

    class Widget : public WidgetActionWidget {
    public:
        Widget(QWidget* parent, SettingsAction* settingsAction);

        bool eventFilter(QObject* object, QEvent* event);

    protected:
        void addStateWidget(WidgetAction* widgetAction, const std::int32_t& priority = 0);

    private:
        void updateLayout();

    protected:
        QHBoxLayout                         _layout;
        QWidget                             _toolBarWidget;
        QHBoxLayout                         _toolBarLayout;
        QVector<WidgetActionStateWidget*>   _stateWidgets;
        QVector<SpacerWidget*>              _spacerWidgets;

        friend class SettingsAction;
    };

    QWidget* getWidget(QWidget* parent, const std::int32_t& widgetFlags) override {
        return new Widget(parent, this);
    };

public:
    SettingsAction(ClusterDifferentialExpressionPlugin* plugin);

    QMenu* getContextMenu();

    void addAction(WidgetAction &action, qsizetype priority=0);

protected:
    QVector<WidgetAction*>   _actions;
    QVector<qsizetype> _priorities;
    
};
*/