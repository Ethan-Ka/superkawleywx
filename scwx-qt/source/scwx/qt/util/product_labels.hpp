#pragma once

#include <QString>
#include <QMap>

namespace scwx::qt::util
{

/// Returns the static product-code → label lookup map.
/// One instance lives in the binary (Meyers singleton via inline function).
inline const QMap<QString, QString>& ProductLabels()
{
   // clang-format off
   static const QMap<QString, QString> labels {
      // Reflectivity
      {"N0B", "Base Refl. (Hi-res)"},
      {"N1B", "Base Refl. (Hi-res)"},
      {"N2B", "Base Refl. (Hi-res)"},
      {"N3B", "Base Refl. (Hi-res)"},
      {"N0Q", "Base Reflectivity"},
      {"N1Q", "Base Reflectivity"},
      {"N2Q", "Base Reflectivity"},
      {"N3Q", "Base Reflectivity"},
      {"NCR", "Composite Refl."},
      {"NET", "Echo Tops"},
      {"EET", "Enhanced Echo Tops"},
      {"NVL", "Vert. Integrated Liquid"},
      {"DVL", "Vert. Integrated Liquid"},
      {"NLA", "Low Layer Composite Refl."},
      {"NML", "Mid Layer Composite Refl."},
      {"NHL", "High Layer Composite Refl."},
      {"DHR", "Digital Hybrid Scan Refl."},
      // Velocity
      {"N0G", "Base Velocity (Hi-res)"},
      {"N1G", "Base Velocity (Hi-res)"},
      {"NAG", "Base Velocity (Hi-res)"},
      {"N0U", "Base Velocity"},
      {"N1U", "Base Velocity"},
      {"N2U", "Base Velocity"},
      {"N3U", "Base Velocity"},
      {"N0S", "Storm-Relative Vel."},
      {"N1S", "Storm-Relative Vel."},
      {"N2S", "Storm-Relative Vel."},
      {"N3S", "Storm-Relative Vel."},
      {"NSW", "Spectrum Width"},
      // Dual-pol
      {"N0X", "Differential Refl. (ZDR)"},
      {"N1X", "Differential Refl. (ZDR)"},
      {"N2X", "Differential Refl. (ZDR)"},
      {"N3X", "Differential Refl. (ZDR)"},
      {"N0C", "Correlation Coeff. (CC)"},
      {"N1C", "Correlation Coeff. (CC)"},
      {"N2C", "Correlation Coeff. (CC)"},
      {"N3C", "Correlation Coeff. (CC)"},
      {"N0K", "Spec. Diff. Phase (KDP)"},
      {"N1K", "Spec. Diff. Phase (KDP)"},
      {"N2K", "Spec. Diff. Phase (KDP)"},
      {"N3K", "Spec. Diff. Phase (KDP)"},
      {"N0H", "Hydrometeor Class"},
      {"N1H", "Hydrometeor Class"},
      {"N2H", "Hydrometeor Class"},
      {"N3H", "Hydrometeor Class"},
      // Derived / alerts
      {"NMD", "Mesocyclone Detection"},
      {"NTV", "Tornado Vortex Sig."},
      {"NHI", "Hail Index"},
      {"NST", "Storm Tracking"},
      {"DPA", "Precip. Array"},
      {"N0F", "Power Removed Control"},
   };
   // clang-format on
   return labels;
}

/// Returns the human-readable label for a product code.
/// Falls back to the raw code if not found — never crashes on unknown codes.
inline QString ProductLabel(const QString& code)
{
   return ProductLabels().value(code, code);
}

} // namespace scwx::qt::util
