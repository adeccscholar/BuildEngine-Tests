#pragma once

#include "MyAddress.h"
#include "MyLocation.h"
#include "Tool_Helper.h"

#include <iomanip>
#include <concepts>
#include <algorithm>
#include <charconv>
#include <fast_float/fast_float.h>
#include <vector>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/blocked_range.h>
#include <numbers>

#include <cassert>


template <typename ty> requires std::floating_point<ty>
class TData : public TAddress {
   friend void swap(TData& lhs, TData& rhs) noexcept { lhs.swap(rhs); }
private:
   Location<ty> mLoc;
public:
   TData(void) : TAddress() { _init(); }
   TData(TData const& ref) : TAddress(ref) { _copy(ref); }
   TData(TAddress const& ref) : TAddress(ref) { _init(); }
   TData(TData&& ref) noexcept : TAddress(std::forward<TAddress>(ref)) { _swap(ref); }

   virtual ~TData(void) override { }

   TData& operator = (TData const& ref) {
      copy(ref);
      return *this;
   }

   TData& operator = (TData&& ref) noexcept {
      swap(ref);
      return *this;
   }

   void swap(TData& ref) {
      TAddress::swap(static_cast<TAddress&>(ref));
      _swap(ref);
      }

   virtual TAddress* create() override { return new TData<ty>; }
   virtual void init(void) override { TAddress::init();  _init(); };

   virtual void copy(TAddress const& ref) override {
      TAddress::copy(ref);
      try {
         _copy(dynamic_cast<TData const&>(ref));
         }
      catch (std::bad_cast&) { ; }
   }

   operator Location<ty>& () { return mLoc; }
   operator Location<ty> const& () const { return mLoc; }

   ty const& Latitude(void) const { return mLoc.first; }
   ty const& Longitude(void) const { return mLoc.second; }


   void Latitude(ty const& newVal) { mLoc.first = newVal; }
   void Longitude(ty const& newVal) { mLoc.second = newVal; }

   void Latitude(std::string const& newVal) { auto result = fast_float::from_chars(newVal.data(), newVal.data() + newVal.size(), mLoc.first); }  // mLoc.first = std::stod(newVal); }
   void Longitude(std::string const& newVal) { auto result = fast_float::from_chars(newVal.data(), newVal.data() + newVal.size(), mLoc.second); } //mLoc.second = std::stod(newVal); }

   void Latitude(std::string&& newVal) { auto result = fast_float::from_chars(newVal.data(), newVal.data() + newVal.size(), mLoc.first); }  // mLoc.first = std::stod(newVal); }
   void Longitude(std::string&& newVal) { auto result = fast_float::from_chars(newVal.data(), newVal.data() + newVal.size(), mLoc.second); } //mLoc.second = std::stod(newVal); }

   /*
   void Latitude(std::string_view const& newVal) { auto result = fast_float::from_chars(newVal.data(), newVal.data() + newVal.size(), mLoc.first); }  // mLoc.first = std::stod(newVal); }
   void Longitude(std::string_view const& newVal) { auto result = fast_float::from_chars(newVal.data(), newVal.data() + newVal.size(), mLoc.second); } //mLoc.second = std::stod(newVal); }
   */
   void Latitude(std::string_view newVal) { auto result = fast_float::from_chars(newVal.data(), newVal.data() + newVal.size(), mLoc.first); }  // mLoc.first = std::stod(newVal); }
   void Longitude(std::string_view newVal) { auto result = fast_float::from_chars(newVal.data(), newVal.data() + newVal.size(), mLoc.second); } //mLoc.second = std::stod(newVal); }

private:
   void _init(void) { mLoc = Location<ty> { 0.0, 0.0 }; }
   void _copy(TData const& ref) { mLoc = ref.mLoc; }
   void _swap(TData& ref) noexcept { std::swap(mLoc, ref.mLoc); }
};



template <typename ty>
   requires std::floating_point<ty>
class TDataNew {
   friend std::ostream& operator<<(
      std::ostream& out,
      TDataNew const& aData) {
      if (!aData.strZipCode.empty()) {
         out << aData.strZipCode << ' ';
      }

      out << aData.strCity << ", " << aData.strStreet;

      if (!aData.strStreetNumber.empty()) {
         out << ' ' << aData.strStreetNumber;
      }

      return out;
   }

private:
   std::string strCity;
   std::string strStreet;
   std::string strStreetNumber;
   std::string strZipCode;
   std::string strUrbanUnit;
   std::string strUrbanUnitOld;
   std::string strDistrict;

   Location<ty> aLocation{
      ty { 0.0 },
      ty { 0.0 }
   };

   int iStreetNumber = 0;
   char chStreetNumberSuffix = '\0';

public:
   [[nodiscard]]
   std::string const& City() const noexcept {
      return strCity;
   }

   [[nodiscard]]
   std::string const& Street() const noexcept {
      return strStreet;
   }

   [[nodiscard]]
   std::string const& StreetNumber() const noexcept {
      return strStreetNumber;
   }

   [[nodiscard]]
   int StreetNumberValue() const noexcept {
      return iStreetNumber;
   }

   [[nodiscard]]
   char StreetNumberSuffix() const noexcept {
      return chStreetNumberSuffix;
   }

   [[nodiscard]]
   std::string const& ZipCode() const noexcept {
      return strZipCode;
   }

   [[nodiscard]]
   std::string const& UrbanUnit() const noexcept {
      return strUrbanUnit;
   }

   [[nodiscard]]
   std::string const& UrbanUnitOld() const noexcept {
      return strUrbanUnitOld;
   }

   [[nodiscard]]
   std::string const& District() const noexcept {
      return strDistrict;
   }

   [[nodiscard]]
   Location<ty> const& Position() const noexcept {
      return aLocation;
   }

   [[nodiscard]]
   ty Latitude() const noexcept {
      return aLocation.first;
   }

   [[nodiscard]]
   ty Longitude() const noexcept {
      return aLocation.second;
   }

   void City(std::string_view const svNewValue) {
      strCity.assign(svNewValue);
   }

   void Street(std::string_view const svNewValue) {
      strStreet.assign(svNewValue);
      }

   void StreetNumber(std::string_view const svNewValue) {
      TStreetNumberKey const aKey = ParseStreetNumber(svNewValue);

      strStreetNumber.assign(svNewValue);
      iStreetNumber = aKey.iNumber;
      chStreetNumberSuffix = aKey.chSuffix;
      }

   void ZipCode(std::string_view const svNewValue) {
      strZipCode.assign(svNewValue);
      }

   void UrbanUnit(std::string_view const svNewValue) {
      strUrbanUnit.assign(svNewValue);
      }

   void UrbanUnitOld(std::string_view const svNewValue) {
      strUrbanUnitOld.assign(svNewValue);
      }

   void District(std::string_view const svNewValue) {
      strDistrict.assign(svNewValue);
      }

   void Latitude(ty const flNewValue) noexcept {
      aLocation.first = flNewValue;
      }

   void Longitude(ty const flNewValue) noexcept {
      aLocation.second = flNewValue;
      }

   void Latitude(std::string_view const svNewValue) noexcept {
      aLocation.first = ParseCoordinate(svNewValue);
      }

   void Longitude(std::string_view const svNewValue) noexcept {
      aLocation.second = ParseCoordinate(svNewValue);
      }

private:
   struct TStreetNumberKey {
      int iNumber = 0;
      char chSuffix = '\0';
      };

   [[nodiscard]]
   static TStreetNumberKey ParseStreetNumber(
      std::string_view const svStreetNumber) noexcept {
      int iNumber = 0;

      char const* const pBegin = svStreetNumber.data();
      char const* const pEnd = pBegin + svStreetNumber.size();

      std::from_chars_result const aResult =
         std::from_chars(pBegin, pEnd, iNumber);

      assert(aResult.ec == std::errc{});
      assert(aResult.ptr != pBegin);

      return TStreetNumberKey{
         .iNumber = iNumber,
         .chSuffix = aResult.ptr != pEnd
            ? ToLowerAscii(*aResult.ptr)
            : '\0'
      };
   }

   [[nodiscard]]
   static ty ParseCoordinate(
      std::string_view const svValue) noexcept {
      ty flValue = ty{ 0.0 };

      char const* const pBegin = svValue.data();
      char const* const pEnd = pBegin + svValue.size();

      auto const aResult =
         fast_float::from_chars(
            pBegin,
            pEnd,
            flValue,
            fast_float::chars_format::general);

      assert(aResult.ec == std::errc{});
      assert(aResult.ptr == pEnd);

      return flValue;
   }

   [[nodiscard]]
   static constexpr char ToLowerAscii(
      char const chValue) noexcept {
      if (chValue >= 'A' && chValue <= 'Z') {
         return static_cast<char>(
            chValue + ('a' - 'A'));
      }

      return chValue;
   }
};





template <typename ty>
requires std::floating_point<ty>
using data_vector = std::vector<std::pair<TDataNew<ty>, Result<ty>>>;

template <typename ty>
requires std::floating_point<ty>
using func_vector = std::vector<std::function<void(TDataNew<ty>&, std::string const&)>>;

template <typename ty>
requires std::floating_point<ty>
using func_vector_vw = std::vector<std::function<void(TDataNew<ty>&, std::string_view)>>;


template< typename ty>
requires std::floating_point<ty>
inline void CalculateRange(Location<ty> const& pointA, typename data_vector<ty>::iterator begin, typename data_vector<ty>::iterator end) {
//inline void CalculateRange(Location<ty> const& pointA, it_type begin, it_type end) {
   static constexpr auto my_round = [](ty const& val) {
      return std::round(val * 1000.0) / 1000.0;
   };

   static const ty r  = 6371000.785;                     //< mean radius of the earth
   static const ty w1 = static_cast<ty>(3.141592653589793238462643383279502884L) / 180.0;
   static const ty w2 = 180.0 / static_cast<ty>(3.141592653589793238462643383279502884L);

   const ty phiA    = pointA.first  * w1; /// 180.0 * M_PI;
   const ty lambdaA = pointA.second * w1; /// 180.0 * M_PI;
   const ty sin_phiA = std::sin(phiA);
   const ty cos_phiA = std::cos(phiA);

   auto const count = static_cast<std::size_t>(end - begin);
   oneapi::tbb::parallel_for(oneapi::tbb::blocked_range<std::size_t>(0, count), [&](auto const& range) {
      for (auto index = range.begin(); index != range.end(); ++index) {
         auto& val = *(begin + index);
      ty phiB    = val.first.Latitude() * w1;  /// 180.0 * M_PI;
      ty lambdaB = val.first.Longitude() * w1; /// 180.0 * M_PI;
      ty zeta = std::acos(sin_phiA * std::sin(phiB) + cos_phiA * std::cos(phiB) * std::cos(lambdaB - lambdaA));
      ty tmp = (std::sin(phiB) - sin_phiA * std::cos(zeta)) / (cos_phiA * std::sin(zeta));
      ty alpha = tmp < -1 ? 180.0 : tmp > 1 ? 0 : std::acos(tmp) * w2;

      /*
      double alpha = std::acos((std::sin(phiB) - std::sin(phiA) * std::cos(zeta)) / (std::cos(phiA) * std::sin(zeta)));
      if (std::isnan(alpha)) {
         alpha = (std::sin(phiB) - std::sin(phiA) * std::cos(zeta)) / (std::cos(phiA) * std::sin(zeta)) < -1.0 ? 180.0 : 0.0;
         }
      else alpha = my_round(alpha * w2);
      */
      val.second = std::make_pair(my_round(zeta * r), 
                                  lambdaA>lambdaB ? phiA>0 ? 360.0-alpha : 180.0+alpha : phiA>0 ? alpha : 180.0-alpha);
         }
   });
}


