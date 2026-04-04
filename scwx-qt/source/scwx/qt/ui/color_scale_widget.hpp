#pragma once

#include <QWidget>
#include <memory>

namespace scwx::qt::ui
{

class ColorScaleWidgetImpl;

/// Interactive color scale bar shown below each radar pane.
///
/// Compact state  (~18 px): colour gradient strip + unit label.
/// Expanded state (~60 px): gradient + tick marks + plain-English category bands.
///
/// Left-click  → toggle compact / expanded.
/// Right-click → context menu (Always show labels / Intensity guide /
///               Edit colour palette).
///
/// Per-product expanded state is persisted in QSettings under
/// "colorScale/<productCode>/expanded".
class ColorScaleWidget : public QWidget
{
   Q_OBJECT

public:
   explicit ColorScaleWidget(QWidget* parent = nullptr);
   ~ColorScaleWidget();

public slots:
   /// Update the displayed product. Call whenever the active radar product
   /// changes (connect to MapWidget::RadarSweepUpdated + GetRadarProductName).
   void UpdateProduct(const QString& productCode);

protected:
   void paintEvent(QPaintEvent* event) override;
   void mousePressEvent(QMouseEvent* event) override;

private:
   std::unique_ptr<ColorScaleWidgetImpl> p;
};

} // namespace scwx::qt::ui
