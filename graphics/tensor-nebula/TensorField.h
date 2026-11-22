#pragma once

#include <xtensor/containers/xarray.hpp>
#include <xtensor/core/xmath.hpp>
#include <xtl/xoptional.hpp>
#include <xsimd/xsimd.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace tensor_nebula {

enum class FieldMode : int {
   twin = 0,
   trinity = 1,
   standing = 2,
   pulse = 3
};

struct FieldSettings {
   int iWidth {512};
   int iHeight {288};
   FieldMode theMode {FieldMode::twin};
   float flPhase {0.0F};
   float flFrequency {9.0F};
   float flSeparation {2.45F};
   float flEnvelope {0.75F};
};

struct FieldMetrics {
   float flMinimum {};
   float flMaximum {};
   float flMean {};
   float flEnergy {};
   std::size_t uSamples {};
};

/**
\brief Holds the physical scalar field as a genuine xtensor array.
\details The CPU field is the sole source for all GPU visualizations. SIMD
   reduction evaluates numerical observables, not synthetic presentation data.
\tparam T Float type used for the xtensor intensity array.
\note The class does not depend on SDL, a graphics driver, or a sound device.
*/
template<class T = float>
class TensorField final {
   xt::xarray<T> theIntensity;
   FieldMetrics theMetrics;

public:
   /**
   \brief Computes normalized intensity for a configured wave experiment.
   \details The four modes use coherent superposition or a bounded Gaussian pulse.
      Source locations are in the mathematical coordinate plane [-3.2,3.2] x [-1.8,1.8].
   \param theSettings Physical field parameters and numerical resolution.
   \returns Nothing. Updates the owned array and SIMD-derived metrics.
   \throw std::invalid_argument If resolution or parameters are outside safe bounds.
   \throw std::bad_alloc On allocation failure.
   \pre Width and height are at least two.
   \post All intensity samples are finite and in the interval [0,1].
   */
   void Compute(FieldSettings const& theSettings) {
      if(theSettings.iWidth < 2 || theSettings.iHeight < 2 ||
         theSettings.iWidth > 2048 || theSettings.iHeight > 1152 ||
         !std::isfinite(theSettings.flPhase) ||
         !std::isfinite(theSettings.flFrequency) ||
         !std::isfinite(theSettings.flSeparation) ||
         !std::isfinite(theSettings.flEnvelope) ||
         theSettings.flFrequency <= 0.0F ||
         theSettings.flSeparation <= 0.0F)
         throw std::invalid_argument {"Tensor Nebula field settings are invalid"};

      theIntensity = xt::zeros<T>({
         static_cast<std::size_t>(theSettings.iHeight),
         static_cast<std::size_t>(theSettings.iWidth)
      });
      float const flPi = std::numbers::pi_v<float>;
      float const flXScale = 6.4F / static_cast<float>(theSettings.iWidth - 1);
      float const flYScale = 3.6F / static_cast<float>(theSettings.iHeight - 1);
      for(int iY = 0; iY < theSettings.iHeight; ++iY) {
         float const flY = -1.8F + static_cast<float>(iY) * flYScale;
         for(int iX = 0; iX < theSettings.iWidth; ++iX) {
            float const flX = -3.2F + static_cast<float>(iX) * flXScale;
            float const flHalf = theSettings.flSeparation * .5F;
            float const flD1 = std::hypot(flX + flHalf, flY);
            float const flD2 = std::hypot(flX - flHalf, flY);
            float flValue {};
            switch(theSettings.theMode) {
               case FieldMode::twin: {
                  float const flA = std::cos(theSettings.flFrequency * flD1);
                  float const flB = std::cos(theSettings.flFrequency * flD2 + theSettings.flPhase);
                  flValue = .25F * (flA + flB) * (flA + flB);
                  break;
               }
               case FieldMode::trinity: {
                  float const flA = std::cos(theSettings.flFrequency * flD1);
                  float const flB = std::cos(theSettings.flFrequency * flD2 + theSettings.flPhase);
                  float const flC = std::cos(theSettings.flFrequency *
                     std::hypot(flX, flY - 1.4F) - .5F * theSettings.flPhase);
                  flValue = (flA + flB + flC) * (flA + flB + flC) / 9.0F;
                  break;
               }
               case FieldMode::standing: {
                  float const flA = std::sin(theSettings.flFrequency * flX);
                  float const flB = std::sin(theSettings.flFrequency * flY +
                     theSettings.flPhase);
                  flValue = flA * flA * flB * flB;
                  break;
               }
               case FieldMode::pulse: {
                  float const flSigma = std::max(.2F, theSettings.flEnvelope);
                  float const flRadius2 = flX * flX + flY * flY;
                  float const flWave = std::cos(theSettings.flFrequency *
                     std::sqrt(flRadius2) - theSettings.flPhase);
                  float const flGaussian = std::exp(-flRadius2 /
                     (2.0F * flSigma * flSigma));
                  flValue = flGaussian * flWave * flWave;
                  break;
               }
            }
            theIntensity(iY, iX) = static_cast<T>(std::clamp(flValue, 0.0F, 1.0F));
         }
      }
      RecalculateMetrics();
   }

   /**
   \brief Calculates global metrics with actual installed xsimd batch arithmetic.
   \details SIMD handles complete native-width blocks and a scalar tail.
      No duplicate staging vector is allocated; the native xtensor data buffer is used.\n      The accumulation uses multiple floating-point partial sums; no timing
      or speedup claim follows from this numerical API exercise.
   \returns Nothing. Updates the metric values from tensor contents.
   \throw std::runtime_error If a nonfinite sample is encountered.
   \post Metrics describe the current array, not a separately generated field.
   */
   void RecalculateMetrics() {
      theMetrics = {};
      if(theIntensity.size() == 0U) return;
      std::size_t const uCount = theIntensity.size();
      float const* const pSamples = theIntensity.data();
      for(std::size_t uIndex = 0; uIndex < uCount; ++uIndex)
         if(!std::isfinite(pSamples[uIndex]))
            throw std::runtime_error {"Nonfinite xtensor sample"};

      theMetrics.uSamples = uCount;
      auto const [pMin, pMax] = std::minmax_element(
         pSamples, pSamples + uCount);
      theMetrics.flMinimum = *pMin;
      theMetrics.flMaximum = *pMax;

      using batch_ty = xsimd::batch<float>;
      constexpr std::size_t uLanes = batch_ty::size;
      batch_ty theSum {0.0F};
      batch_ty theEnergy {0.0F};
      std::size_t uIndex {};
      for(; uIndex + uLanes <= uCount; uIndex += uLanes) {
         batch_ty const theBlock = batch_ty::load_unaligned(pSamples + uIndex);
         theSum += theBlock;
         theEnergy += theBlock * theBlock;
      }
      double flSum = static_cast<double>(xsimd::reduce_add(theSum));
      double flEnergy = static_cast<double>(xsimd::reduce_add(theEnergy));
      for(; uIndex < uCount; ++uIndex) {
         double const flValue = pSamples[uIndex];
         flSum += flValue;
         flEnergy += flValue * flValue;
      }
      theMetrics.flMean = static_cast<float>(flSum / uCount);
      theMetrics.flEnergy = static_cast<float>(flEnergy / uCount);
   }

   /**
   \brief Returns a nullable selected intensity without inventing off-grid data.
   \param iX Integer sample column.
   \param iY Integer sample row.
   \returns xtl optional value, or missing outside the grid.
   \throw Nothing intentionally.
   */
   [[nodiscard]] xtl::xoptional<T> Sample(int const iX, int const iY) const {
      if(iX < 0 || iY < 0 ||
         static_cast<std::size_t>(iX) >= theIntensity.shape()[1] ||
         static_cast<std::size_t>(iY) >= theIntensity.shape()[0])
         return xtl::missing<T>();
      return xtl::xoptional<T> {theIntensity(iY, iX)};
   }

   /**
   \brief Exposes read-only numerical source evidence.
   \returns The actual owned xtensor field.
   \throw Nothing intentionally.
   */
   [[nodiscard]] xt::xarray<T> const& Values() const noexcept {
      return theIntensity;
   }

   /**
   \brief Exposes metrics derived from the same tensor.
   \returns Minimum, maximum, mean and mean-square energy.
   \throw Nothing intentionally.
   */
   [[nodiscard]] FieldMetrics const& Metrics() const noexcept {
      return theMetrics;
   }
};
}
