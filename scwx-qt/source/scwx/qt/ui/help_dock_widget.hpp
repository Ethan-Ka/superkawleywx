#pragma once

#include <QDockWidget>
#include <memory>

namespace scwx::qt::ui
{

class HelpDockWidgetImpl;

/// Collapsible sidebar help panel.
///
/// - Shows plain-English product guides indexed by product code.
/// - Search field filters content in real time.
/// - ShowProductHelp() navigates to the article for the given product code.
///   Used by the "What am I looking at?" action in the pane context menu.
class HelpDockWidget : public QDockWidget
{
   Q_OBJECT

public:
   explicit HelpDockWidget(QWidget* parent = nullptr);
   ~HelpDockWidget();

public slots:
   /// Navigate to the help article for productCode.
   /// Shows and raises the dock if it is hidden.
   void ShowProductHelp(const QString& productCode);

private:
   std::unique_ptr<HelpDockWidgetImpl> p;
};

} // namespace scwx::qt::ui
