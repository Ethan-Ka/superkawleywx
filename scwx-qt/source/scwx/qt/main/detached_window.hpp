#pragma once

#include <scwx/qt/gl/gl_context.hpp>
#include <scwx/qt/map/map_widget.hpp>

#include <memory>

#include <qmaplibre.hpp>

#include <QMainWindow>

namespace scwx
{
namespace qt
{
namespace main
{

class DetachedWindowImpl;

/**
 * @brief A fully independent radar window duplicated from a source pane.
 *
 * Opened via right-click → "Duplicate in new window". Initialized with the
 * source pane's site, product, tilt, zoom, and pan at the moment of
 * duplication. After opening it is fully decoupled — no shared state.
 */
class DetachedWindow : public QMainWindow
{
   Q_OBJECT

public:
   /**
    * @param sourcePane  The pane to duplicate. State is read at construction.
    * @param settings    QMapLibre settings (map provider, API keys, etc.)
    * @param glContext   Shared OpenGL context from the main window.
    * @param parent      Optional Qt parent.
    */
   DetachedWindow(map::MapWidget*                sourcePane,
                  const QMapLibre::Settings&     settings,
                  std::shared_ptr<gl::GlContext> glContext,
                  QWidget*                       parent = nullptr);
   ~DetachedWindow();

   void showEvent(QShowEvent* event) override;
   void closeEvent(QCloseEvent* event) override;

private:
   std::unique_ptr<DetachedWindowImpl> p;
};

} // namespace main
} // namespace qt
} // namespace scwx
