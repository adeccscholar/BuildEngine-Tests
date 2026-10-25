#pragma once

#include "database.h"
#include "system_data_persistent.h"

#include "extented_test_types.h"

#include <set>

namespace extented_test {

   template <adecc::db::persistent_system_data_type data_ty,
             std::ranges::input_range range_ty>
        requires adecc::db::persistent_system_data_tuple_range<range_ty, data_ty>
   [[nodiscard]] auto ToPersistentSet(range_ty&& rngValues) {
      return std::forward<range_ty>(rngValues)
                   | std::views::transform([](auto&& val) {
                           return data_ty { std::forward<decltype(val)>(val) };
                           })
                   | std::ranges::to<std::set<data_ty>>();
      }

   template <adecc::db::persistent_system_data_type data_ty,
             adecc::db::logical_database_type db_ty>
   auto LoadValues(db_ty& db) {
      return data_ty::types_list::invoke([&]<class... Ts>() {
         return ToPersistentSet<data_ty>(
            db.template Execute<Ts...>(data_ty::CreateSelectFromSql(), {} )
            );
         });
      }

   template <std::size_t uKeyIndex,std::size_t uValueIndex,
             std::ranges::input_range range_ty>
      requires adecc::tuple_index_available<std::ranges::range_value_t<range_ty>, uKeyIndex> &&
               adecc::tuple_index_available<std::ranges::range_value_t<range_ty>, uValueIndex>
   [[nodiscard]] auto BuildBridge(range_ty&& rngValues) {
      using entry_ty = std::remove_cvref_t<std::ranges::range_value_t<range_ty>>;
      using key_ty = std::remove_cvref_t<std::tuple_element_t<uKeyIndex, entry_ty>>;
      using value_ty = std::remove_cvref_t<std::tuple_element_t<uValueIndex, entry_ty>>;

      return std::forward<range_ty>(rngValues) 
               | std::views::transform([](entry_ty const& entry) {
                    return std::pair<key_ty, value_ty> {
                                     std::get<uKeyIndex>(entry),
                                     std::get<uValueIndex>(entry)
                                     };
                    })
               | std::ranges::to<std::map<key_ty, value_ty>>();
      }


   /**
     \brief Sucht einen Schlüssel in einer Bridge Map.
     \details
        Die Funktion liefert den zugeordneten Wert zurück oder wirft eine Exception mit
        formatierter Fehlermeldung, wenn der Schlüssel nicht existiert.
     \tparam key_ty Schlüsseltyp.
     \tparam value_ty Werttyp.
     \tparam compare_ty Vergleichstyp der Map.
     \tparam allocator_ty Allocator Typ der Map.
     \param mpBridge Die Bridge Map.
     \param theKey Der gesuchte Schlüssel.
     \returns Der zugeordnete Wert.
     \throw std::out_of_range Wenn der Schlüssel nicht gefunden wurde.
   */
   template <typename key_ty, typename value_ty, typename compare_ty, typename allocator_ty>
   [[nodiscard]] value_ty BridgeValue(std::map<key_ty, value_ty, compare_ty, allocator_ty> const& mpBridge,
                                      key_ty const& theKey ) {
      if (auto const it = mpBridge.find(theKey); it != mpBridge.end()) [[likely]] {
         return it->second;
         }
      else {
         throw std::out_of_range { std::format("Bridge key '{}' not found.", theKey) };
         }
      }

   template <typename key_ty, typename value_ty, typename compare_ty, typename allocator_ty>
   [[nodiscard]] std::optional<value_ty> BridgeValue(std::map<key_ty, value_ty, compare_ty, allocator_ty> const& mpBridge,
                                                     std::optional<key_ty> const& optKey) {
      if (!optKey) {
         return std::nullopt;
         }
      else {
         if (auto const it = mpBridge.find(*optKey); it != mpBridge.end()) [[likely]] {
            return it->second;
            }
         else {
            throw std::out_of_range { std::format("Bridge key '{}' not found.", *optKey) };
            }
         }
      }

   template <typename value_ty, typename compare_ty, typename allocator_ty, typename key_ty>
   [[nodiscard]] value_ty const& FindInSet(std::set<value_ty, compare_ty, allocator_ty> const& Values,
                                           key_ty const& Value ) {
      if(auto const it = Values.find(value_ty(Value)); it == Values.end()) {
         throw std::out_of_range{ std::format("Value '{}' not found in set.", Value) };
         }
      else return *it;
      }


} // end of namespace extented_test