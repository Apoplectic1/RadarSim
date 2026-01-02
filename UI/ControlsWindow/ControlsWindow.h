// ---- ControlsWindow.h ----
// Floating window for radar/target/RCS plane controls

#pragma once

#include <QWidget>

class QVBoxLayout;

class ControlsWindow : public QWidget {
    Q_OBJECT

public:
    explicit ControlsWindow(QWidget* parent = nullptr);
    ~ControlsWindow() override = default;

    // Set the content widget (transfers the controls panel)
    void setContent(QWidget* content);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUI();

    QVBoxLayout* mainLayout_ = nullptr;
};
