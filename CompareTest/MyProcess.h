#pragma once

#include "Tool_Helper.h"

#include "MyData.h"
#include <oneapi/tbb/parallel_sort.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/blocked_range.h>
#include "MyFileIterator.h"
#include "MyConvert.h"
#include "MyTestStatistcs.h"
#include "BlockingQueue.h"

#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>
#include <tuple>
#include <vector>
#include <map>
#include <algorithm>
 
#include <iterator>
#include <filesystem>
#include <future>
#include <ranges>
#include <concepts>
#include <source_location>

using namespace std::literals;
namespace fs = std::filesystem;

inline void Log(std::string const& msg, std::exception const& ex, std::source_location const& loc = std::source_location::current()) {
   std::cerr << std::format("{}: {} in file {} at line {}\n", msg, ex.what(), loc.file_name(), loc.line());
   }



template <typename ty>
   requires std::floating_point<ty>
inline void Reading(data_vector<ty>& vData, fs::path const& strFile) {
   static const func_vector_vw<ty> funcs_vw = {
      [](TDataNew<ty>& data, std::string_view val) { data.City(val); } ,
      [](TDataNew<ty>& data, std::string_view val) { data.Street(val); } ,
      [](TDataNew<ty>& data, std::string_view val) { data.StreetNumber(val); },
      [](TDataNew<ty>& data, std::string_view val) { data.ZipCode(val); },
      [](TDataNew<ty>& data, std::string_view val) { data.UrbanUnit(val); },
      [](TDataNew<ty>& data, std::string_view val) { data.UrbanUnitOld(val); },
      [](TDataNew<ty>& data, std::string_view val) { data.District(val); },
      [](TDataNew<ty>& data, std::string_view val) { data.Latitude(val); },
      [](TDataNew<ty>& data, std::string_view val) { data.Longitude(val); },
      [](TDataNew<ty>&, std::string_view) { throw std::runtime_error("unexpected number of elements.");  }
      };


   std::string strBuffer;
   
   my_line_count::reset();
   auto lines = my_lines{ GetContent(strFile, strBuffer) } | std::ranges::to<std::vector<my_line_count>>();
   vData.resize(lines.size());
   
   static constexpr std::string_view delim{ ";" };
   //static constexpr auto my_enumerate = [](std::ranges::viewable_range auto&& r) {
   //   return std::ranges::zip_view(std::views::iota(0), std::forward<decltype(r)>(r)); 
   //   };

  oneapi::tbb::parallel_for(oneapi::tbb::blocked_range<std::size_t>(0, lines.size()),
      [&vData, &lines](auto const& range) {
         for(std::size_t index = range.begin(); index != range.end(); ++index) {
            auto const& val = lines[index];
      //for(auto const& [iCount, data] : my_enumerate(std::ranges::split_view(val.view, delim))) {

          std::size_t iCount = 0;
          for (auto const& data : std::ranges::split_view(val.view, delim)) {
             if (iCount >= funcs_vw.size()) throw std::runtime_error("too many fields");
             funcs_vw[iCount++](vData[val.index].first, std::string_view{ data.begin(), data.end() });
            }
         }
      });
   return;
   }

template <typename ty>
   requires std::floating_point<ty>
inline void WritingPart(auto begin, auto end, fs::path const& strFile) {

   using itDataType = typename data_vector<ty>::const_iterator;
   using tplData = std::tuple<itDataType, itDataType, std::string>;

   static auto constexpr WritePart = [](itDataType begin, itDataType end, std::string& strBuffer) {
      std::ranges::for_each(std::ranges::subrange { begin, end }, [&strBuffer](auto const& val) {
      //std::for_each(begin, end, [&strBuffer](auto const& val) {
         std::string fill1(70 - val.first.City().length() - val.first.District().length(), ' ');
         std::string fill2(64 - val.first.Street().length() - val.first.StreetNumber().length(), ' ');
         std::format_to(std::back_inserter(strBuffer), "{} {} - {}, {}{} {}{}={}m / {}°\n",
            val.first.ZipCode(), val.first.City(), val.first.District(), fill1,
            val.first.Street(), val.first.StreetNumber(), fill2, my_Double_to_String_G(val.second.first, 3),
            my_Double_to_String_G(val.second.second, 3));
         });
      return true;
   };

   const size_t maxP = 8u;
   size_t pos = 0, count = std::distance(begin, end) / maxP;
   std::vector<std::pair<std::future<bool>, tplData>> procs(maxP);
   for (int i : std::ranges::iota_view{ 0u, maxP }) {
      std::get<0>(procs[i].second) = begin + i * count;
      std::get<1>(procs[i].second) = i < maxP - 1 ? begin + (i + 1) * count : end;
      if (i < maxP - 1) std::get<2>(procs[i].second).reserve(171 * count);
      else {
         size_t c = std::distance(std::get<0>(procs[i].second), std::get<1>(procs[i].second));
         std::get<2>(procs[i].second).reserve(171 * c);
      }
      procs[i].first = std::async(WritePart, std::get<0>(procs[i].second), std::get<1>(procs[i].second),
         std::ref(std::get<2>(procs[i].second)));
   }

   std::ofstream of(strFile);
   for (auto& part : procs) {
      bool test = part.first.get();
      of << std::get<2>(part.second);
   }

   }

/// !!!!!
template <typename ty>
requires std::floating_point<ty>
inline void Writing(data_vector<ty> const& vData, fs::path const& strFile) {
   //WritingPart(vData.cbegin(), vData.cend(), strFile);

   using itDataType = data_vector<ty>::const_iterator;
   using tplData = std::tuple<itDataType, itDataType, std::string>;

   static auto constexpr WritePart = [](itDataType begin, itDataType end, std::string& strBuffer) {
      std::for_each(begin, end, [&strBuffer](auto const& val) {
         std::format_to(std::back_inserter(strBuffer), "{0:s} {1:s} - {2:s}{3:<{4}s} {5:s} {6:<{7}s}={8:s}m / {9:s}°\n",
            val.first.ZipCode(), val.first.City(), val.first.District(), ","s, 71 - val.first.City().size() - val.first.District().size(),
            val.first.Street(), val.first.StreetNumber(), 64 - val.first.Street().size(),
            my_Double_to_String_G(val.second.first, 3), my_Double_to_String_G(val.second.second, 3));
         });
      return true;
   };

   const size_t maxP = 16u;
   size_t pos = 0, count = vData.size() / maxP;
   std::vector<std::pair<std::future<bool>, tplData>> procs(maxP);
   for (int i : std::ranges::iota_view{ 0u, maxP }) {
      std::get<0>(procs[i].second) = vData.cbegin() + i * count;
      std::get<1>(procs[i].second) = i < maxP - 1 ? vData.cbegin() + (i + 1) * count : vData.cend();
      if (i < maxP - 1) std::get<2>(procs[i].second).reserve(171 * count);
      else {
         size_t c = std::distance(std::get<0>(procs[i].second), std::get<1>(procs[i].second));
         std::get<2>(procs[i].second).reserve(171 * c);
      }
      procs[i].first = std::async(WritePart, std::get<0>(procs[i].second), std::get<1>(procs[i].second),
         std::ref(std::get<2>(procs[i].second)));
   }

   std::ofstream of(strFile);
   for (auto& part : procs) {
      bool test = part.first.get();
      of << std::get<2>(part.second);
   }
}


template <typename ty>
   requires std::floating_point<ty>
inline void Sorting(data_vector<ty>& vData) {
   static const std::string special_chars = "ßüöäÄÖÜáèéçñ"s;

   static const std::map<char, std::string> special_values = {
      { 'Ö', "Oe"s }, { 'ä', "ae"s }, { 'Ä', "Ae"s }, { 'ñ', "n"s  },
      { 'ü', "ue"s }, { 'ç', "c"s  }, { 'Ü', "Ue"s }, { 'è', "e"s  },
      { 'ß', "ss"s }, { 'ö', "oe"s }, { 'á', "a"s  }, { 'é', "e"s  }
      };

   static const auto normalize = [](std::string const& param, std::size_t pos) {
            std::string text = param;

            do {
               text.replace(pos, 1, special_values.find(text[pos])->second);
               }
            while ((pos = text.find_first_of(special_chars, pos)) != std::string::npos);

            return text;
      };

   static const auto compare_text = [](std::string const& left, std::string const& right) {
            if (auto const result = left <=> right; result == 0) [[likely]] {
               return result;
               }
            else {
               auto const left_pos = left.find_first_of(special_chars);
               auto const right_pos = right.find_first_of(special_chars);

               bool const left_special = left_pos != std::string::npos;

               bool const right_special = right_pos != std::string::npos;

               if (left_special && right_special) {
                  return normalize(left, left_pos) <=> normalize(right, right_pos);
                  }

               if (!left_special && right_special) {
                  return left <=> normalize(right, right_pos);
                  }

               if (left_special && !right_special) {
                  return normalize(left, left_pos) <=> right;
                  }

               return result;
               }
      };

   static constexpr auto compare_street_number = [](TDataNew<ty> const& lhs, TDataNew<ty> const& rhs) noexcept {
            if (auto const cmp = lhs.StreetNumberValue() <=> rhs.StreetNumberValue(); cmp != 0) {
               return cmp < 0;
               }

            return lhs.StreetNumberSuffix() < rhs.StreetNumberSuffix();
            };

   oneapi::tbb::parallel_sort( vData.begin(), vData.end(),
             [](auto const& lhs, auto const& rhs) {
                 auto const& left = lhs.first;
                 auto const& right = rhs.first;

                 if (auto const cmp = compare_text(left.City(), right.City()); cmp != 0) [[unlikely]] {
                    return cmp < 0;
                    }

                 if (auto const cmp = compare_text(left.UrbanUnit(), right.UrbanUnit()); cmp != 0) {
                    return cmp < 0;
                    }

                 if (auto const cmp = compare_text(left.District(), right.District()); cmp != 0) {
                    return cmp < 0;
                    }

                 if (auto const cmp = left.ZipCode() <=> right.ZipCode(); cmp != 0) {
                    return cmp < 0;
                    }

                 if (auto const cmp = compare_text(left.Street(), right.Street()); cmp != 0) {
                    return cmp < 0;
                    }

                 return compare_street_number(left, right);
                 });
}

inline void DeleteDirectories_with_ranges(std::string const& strPath) {
   static constexpr auto is_directory = [](auto const& d) { return fs::is_directory(d); };
   for (auto const& dir : fs::directory_iterator(strPath) | std::views::filter(is_directory)) 
      fs::remove_all(dir.path());
   }

inline void DeleteDirectories(fs::path const& strPath) {
   static std::function<void(fs::path const&, bool)> ClearDir = [](fs::path const& p, bool boDelete) {
      auto files = fs::directory_iterator { p } | std::ranges::to<std::vector>();
      auto it_dir = std::partition(files.begin(), files.end(), [](fs::path const& p) { return !fs::is_directory(p); });
      if (boDelete) std::for_each(files.begin(), it_dir, [](fs::path const& p) { fs::remove(p); });
      if(it_dir != files.end()) std::for_each(it_dir, files.end(), [](fs::path const& p2) { ClearDir(p2, true); } );
      if (boDelete) fs::remove(p);
      };
   ClearDir(strPath, false);
   }

template <typename ty>
   requires std::floating_point<ty>
inline void ReadFromDirectory(fs::path const& strPath, data_vector<ty>& vData) {
   static const std::vector<std::function<void(std::pair<TDataNew<ty>, Result<ty>>&, std::string_view)>> funcs_read_d = {
      [](std::pair<TDataNew<ty>, Result<ty>>& data, std::string_view val) { data.first.StreetNumber(val); },
      [](std::pair<TDataNew<ty>, Result<ty>>& data, std::string_view val) { data.first.ZipCode(val); },
      [](std::pair<TDataNew<ty>, Result<ty>>& data, std::string_view val) { data.first.UrbanUnitOld(val); },
      [](std::pair<TDataNew<ty>, Result<ty>>& data, std::string_view val) { data.first.Latitude(val); },
      [](std::pair<TDataNew<ty>, Result<ty>>& data, std::string_view val) { data.first.Longitude(val); },
      [](std::pair<TDataNew<ty>, Result<ty>>& data, std::string_view val) {
                                               fast_float::from_chars(val.data(), val.data() + val.size(), data.second.first); },
      [](std::pair<TDataNew<ty>, Result<ty>>& data, std::string_view val) {
                                               fast_float::from_chars(val.data(), val.data() + val.size(), data.second.second); },
      [](std::pair<TDataNew<ty>, Result<ty>>& data, std::string_view val) { throw std::runtime_error("file corruped"); }
   };

   static auto constexpr is_directory = [](fs::directory_entry const& entry) { return entry.is_directory();  };
   static auto constexpr is_file      = [](fs::directory_entry const& entry) { return entry.is_regular_file();  };

   using detail_data = std::tuple<std::string, std::string, std::string, std::string>;
   using data_to_read = std::tuple<fs::path, std::string, std::vector<my_line_count>, detail_data>;
   using to_read_vec = std::vector<data_to_read>;
   to_read_vec to_read;

   vData.clear();

   try {
      std::ranges::for_each(fs::directory_iterator{ strPath } | std::views::filter(is_directory), [&to_read](auto& d) {
         std::string&& strCities = d.path().stem().string();
         std::ranges::for_each(fs::directory_iterator{ d } | std::views::filter(is_directory), [&to_read, &strCities](auto& d) {
            std::string&& strUnit = d.path().stem().string();
            std::ranges::for_each(fs::directory_iterator{ d } | std::views::filter(is_directory), [&to_read, &strCities, &strUnit](auto& d) {
               std::string&& strDistrict = d.path().stem().string();
               std::ranges::for_each(fs::directory_iterator{ d } | std::views::filter(is_file), [&to_read, &strCities, &strUnit, &strDistrict](auto& d) {
                  std::string&& strStreet = d.path().stem().string();
                  detail_data data = { strCities, strUnit, strDistrict, strStreet };
                  to_read.emplace_back(std::move(d), std::string(), std::move(std::vector<my_line_count>()), std::move(data));
                  });
               });
            });
         });
   }
   catch(std::exception& ex) {
      Log("error while preparing reading of directories", ex);
      }
   my_line_count::reset();

   oneapi::tbb::parallel_for(
      oneapi::tbb::blocked_range<std::size_t>(0u, to_read.size()),
      [&to_read](auto const& range) {
         for (std::size_t i = range.begin(); i != range.end(); ++i) {
            auto& val = to_read[i];
            auto const test = GetContent(std::get<0>(val), std::get<1>(val));
            std::get<2>(val).reserve(std::ranges::count(test, '\n'));
            std::ranges::copy(my_lines { test }, std::back_inserter(std::get<2>(val)));
            }
         });

   vData.resize(my_line_count::GetCounter());

   try {
      static constexpr std::string_view delim{ ";" };


      oneapi::tbb::parallel_for(
      oneapi::tbb::blocked_range<std::size_t>(0u, to_read.size()),
      [&vData, &to_read](auto const& range) {
         for (std::size_t i = range.begin(); i != range.end(); ++i) {
            auto& val = to_read[i];
         auto& [strCity, strUnit, strDistrict, strStreet] = std::get<3>(val);
         std::for_each(std::get<2>(val).begin(), std::get<2>(val).end(), [&vData, &strCity, &strUnit, &strDistrict, &strStreet](auto& val_) mutable {
            auto& address = vData[val_.index];
            address.first.City(strCity);
            address.first.UrbanUnit(strUnit);
            address.first.District(strDistrict);
            address.first.Street(strStreet);

             std::size_t iCount = 0;
             for (auto const& data : std::ranges::split_view(val_.view, delim)) {
                if (iCount >= funcs_read_d.size()) throw std::runtime_error("too many fields");
                funcs_read_d[iCount++](address, std::string_view{ data.begin(), data.end() });
               }
            });
            }
         });
   }
   catch (std::exception& ex) {
      Log("error while parsing data", ex);
      }
   }


template <typename ty>
   requires std::floating_point<ty>
inline void WriteToDirectory(
   fs::path const& strPath,
   data_vector<ty>& vData) {
   using iterator_ty = typename data_vector<ty>::const_iterator;

   struct directory_group {
      iterator_ty first;
      iterator_ty last;
      fs::path file;
   };

   static constexpr auto compare = [](auto const& lhs, auto const& rhs) noexcept {
      auto const& left = lhs.first;
      auto const& right = rhs.first;

      if (auto const cmp = left.City() <=> right.City(); cmp != 0) [[unlikely]] {
         return cmp < 0;
         }

      if (auto const cmp = left.UrbanUnit() <=> right.UrbanUnit(); cmp != 0) [[unlikely]] {
         return cmp < 0;
         }

      if (auto const cmp = left.District() <=> right.District(); cmp != 0) [[unlikely]] {
         return cmp < 0;
         }

      if (auto const cmp = left.Street() <=> right.Street(); cmp != 0) {
         return cmp < 0;
         }

      if (auto const cmp = left.StreetNumberValue() <=> right.StreetNumberValue(); cmp != 0) {
         return cmp < 0;
         }

      if (auto const cmp = left.StreetNumberSuffix() <=> right.StreetNumberSuffix(); cmp != 0) {
         return cmp < 0;
         }
      return left.ZipCode() < right.ZipCode();
      };

   oneapi::tbb::parallel_sort( vData.begin(), vData.end(), compare);

   std::vector<directory_group> positions;

   positions.reserve(std::min<std::size_t>(vData.size(), 16'384u));

   fs::path oldPath;

   for (auto first = vData.cbegin();
      first != vData.cend();) {
      auto const& address = first->first;

      auto const last = std::find_if(
         std::next(first),
         vData.cend(),
         [&address](auto const& value) noexcept {
            auto const& other = value.first;

            return address.Street() != other.Street()
               || address.District() != other.District()
               || address.UrbanUnit() != other.UrbanUnit()
               || address.City() != other.City();
            });

      fs::path const currentPath = strPath / address.City() / address.UrbanUnit()
         / address.District();

      if (currentPath != oldPath) {
         fs::create_directories(currentPath);
         oldPath = currentPath;
         }

      positions.emplace_back(first, last, currentPath / (address.Street() + ".csv"s));
      first = last;
      }

   if (positions.empty()) {
      return;
      }

   struct directory_buffer {
      fs::path file;
      std::string content;
   };

   constexpr std::size_t producerCount =  20u;
   constexpr std::size_t queueCapacity = 256u;
   constexpr std::size_t writerCount   =   5u;

   BlockingQueue<directory_buffer> queue{ queueCapacity };

   std::exception_ptr pipelineException;
   std::mutex pipelineExceptionMutex;

   auto const storeException = [&pipelineException, &pipelineExceptionMutex] {
      std::lock_guard guard{ pipelineExceptionMutex };

      if (!pipelineException) {
         pipelineException = std::current_exception();
         }
      };

   auto const writeBuffers = [&queue, &storeException] {
      try {
         while (auto value = queue.Pop()) {
            std::ofstream output{ value->file };

            if (!output) {
               throw std::runtime_error(
                  std::format("Die Datei \"{}\" konnte nicht geöffnet werden.",
                     value->file.string()));
               }

            output.write(
               value->content.data(),
               static_cast<std::streamsize>(value->content.size()));

            if (!output) {
               throw std::runtime_error(
                  std::format(
                     "Die Datei \"{}\" konnte nicht vollständig geschrieben werden.",
                     value->file.string()));
               }
            }
         }
      catch (...) {
         storeException();
         queue.Close();
         }
      };

   std::vector<std::jthread> writers;
   writers.reserve(writerCount);

   for (std::size_t iWriter = 0u; iWriter < writerCount; ++iWriter) {
      writers.emplace_back(writeBuffers);
      }
   // for(auto& current : writers) current = std::jthread{ writeBuffers };
   
   using position_iterator_ty =
      typename std::vector<directory_group>::const_iterator;

   auto const createBuffers =
      [&queue, &storeException](
         position_iterator_ty first,
         position_iterator_ty const last) {
         try {
            for (; first != last; ++first) {
               auto const& group = *first;

               auto const count = static_cast<std::size_t>(
                  std::distance(group.first, group.last));

               std::string strBuffer;
               strBuffer.reserve(count * 68u);

               std::for_each(group.first, group.last,
                  [&strBuffer](auto const& value) {
                     auto const& [address, result] = value;
                     std::format_to(std::back_inserter(strBuffer), "{};{};{};{};{};{};{}\n",
                        address.StreetNumber(), address.ZipCode(),
                        address.UrbanUnitOld(),
                        my_Double_to_String_short(address.Latitude(), 9),
                        my_Double_to_String_short(address.Longitude(), 9),
                        my_Double_to_String_short(result.first, 3),
                        my_Double_to_String_short(result.second, 3));
                     });

               if (!queue.Push(
                     directory_buffer{
                        std::move(group.file),
                        std::move(strBuffer)
                        })) {
                  return;
                  }
               }
            }
         catch (...) {
            storeException();
            queue.Close();
            }
         };

   auto const effectiveProducerCount = std::min(
      producerCount,
      positions.size());

   std::vector<std::jthread> producers;
   producers.reserve(effectiveProducerCount);

   auto firstPosition = positions.cbegin();

   auto const baseCount =
      positions.size() / effectiveProducerCount;

   auto const remainder =
      positions.size() % effectiveProducerCount;

   for (std::size_t iProducer = 0u;
      iProducer < effectiveProducerCount;
      ++iProducer) {
      auto const count =
         baseCount + (iProducer < remainder ? 1u : 0u);

      auto const lastPosition = std::next(
         firstPosition,
         static_cast<std::ptrdiff_t>(count));

      producers.emplace_back(
         createBuffers,
         firstPosition,
         lastPosition);

      firstPosition = lastPosition;
      }

   for (auto& producer : producers) {
      producer.join();
      }

   queue.Close();

   for (auto& writer : writers) {
      writer.join();
      }

   if (pipelineException) {
      std::rethrow_exception(pipelineException);
      }
}

/*
template <typename ty> requires std::floating_point<ty>
inline void WriteToDirectory(fs::path const& strPath, data_vector<ty>& vData, std::size_t const workerCount) {
   using iterator_ty = typename data_vector<ty>::const_iterator;

   struct directory_group {
      iterator_ty first;
      iterator_ty last;
      fs::path file;
   };

   static constexpr auto compare = [](auto const& lhs, auto const& rhs) noexcept {
      auto const& left  = lhs.first;
      auto const& right = rhs.first;

      if (auto const cmp = left.City() <=> right.City(); cmp != 0) [[unlikely]] {
         return cmp < 0;
         }

      if (auto const cmp = left.UrbanUnit() <=> right.UrbanUnit(); cmp != 0) [[unlikely]] {
         return cmp < 0;
         }

      if (auto const cmp = left.District() <=> right.District(); cmp != 0) [[unlikely]] {
         return cmp < 0;
         }

      if (auto const cmp = left.Street() <=> right.Street(); cmp != 0) {
         return cmp < 0;
         }

      if (auto const cmp = left.StreetNumberValue() <=> right.StreetNumberValue(); cmp != 0) {
         return cmp < 0;
         }

      return left.StreetNumberSuffix() < right.StreetNumberSuffix();
      };

   oneapi::tbb::parallel_sort( vData.begin(), vData.end(), compare);

   std::vector<directory_group> positions;
   positions.reserve(std::min<std::size_t>(vData.size(), 16'384u));

   fs::path oldPath;

   for (auto first = vData.cbegin(); first != vData.cend();) {
      auto const& address = first->first;

      auto const last = std::find_if(
         std::next(first),
         vData.cend(),
         [&address](auto const& value) noexcept {
            auto const& other = value.first;

            return address.Street()    != other.Street()
                || address.District()  != other.District()
                || address.UrbanUnit() != other.UrbanUnit()
                || address.City()      != other.City();
            });

      fs::path const currentPath = strPath / address.City() / address.UrbanUnit()
                                   / address.District();

      if (currentPath != oldPath) {
         fs::create_directories(currentPath);
         oldPath = currentPath;
         }

      positions.emplace_back(first, last, currentPath / (address.Street() + ".csv"s));
      first = last;
      }

   if (positions.empty()) {
      return;
      }

   auto const write_group = [](directory_group const& group) {
      auto const count = static_cast<std::size_t>(
         std::distance(group.first, group.last));

      std::string strBuffer;
      strBuffer.reserve(count * 68u);

      std::for_each(
         group.first,
         group.last,
         [&strBuffer](auto const& value) {
            auto const& [address, result] = value;

            std::format_to(
               std::back_inserter(strBuffer),
               "{};{};{};{};{};{};{}\n",
               address.StreetNumber(),
               address.ZipCode(),
               address.UrbanUnitOld(),
               my_Double_to_String_short(address.Latitude(),  9),
               my_Double_to_String_short(address.Longitude(), 9),
               my_Double_to_String_short(result.first,        3),
               my_Double_to_String_short(result.second,       3));
            });

      std::ofstream output { group.file };

      output.write(
         strBuffer.data(),
         static_cast<std::streamsize>(strBuffer.size()));
      };

   auto const effectiveWorkerCount = std::min(
      std::max<std::size_t>(workerCount, 1u),
      positions.size());

   std::atomic_size_t nextPosition { 0u };

   std::vector<std::future<void>> workers;
   workers.reserve(effectiveWorkerCount);

   for (std::size_t worker = 0u; worker < effectiveWorkerCount; ++worker) {
      workers.emplace_back(
         std::async(
            std::launch::async,
            [&positions, &nextPosition, &write_group] {
               for (;;) {
                  auto const index = nextPosition.fetch_add(
                     1u,
                     std::memory_order_relaxed);

                  if (index >= positions.size()) {
                     return;
                     }

                  write_group(positions[index]);
                  }
               }));
      }

   for (auto& worker : workers) {
      worker.get();
      }
   }
*/

/*
template <typename ty>
   requires std::floating_point<ty>
inline void WriteToDirectory(
   fs::path const& strPath,
   data_vector<ty>& vData) {
   using iterator_ty = typename data_vector<ty>::const_iterator;

   struct directory_group {
      iterator_ty first;
      iterator_ty last;
      fs::path file;
   };

   static constexpr auto compare = [](auto const& lhs, auto const& rhs) noexcept {
      auto const& left = lhs.first;
      auto const& right = rhs.first;

      if (auto const cmp = left.City() <=> right.City(); cmp != 0) [[unlikely]] {
         return cmp < 0;
         }

      if (auto const cmp =left.UrbanUnit() <=> right.UrbanUnit(); cmp != 0) [[unlikely]] {
         return cmp < 0;
         }

      if (auto const cmp = left.District() <=> right.District(); cmp != 0) [[unlikely]] {
         return cmp < 0;
         }

      if (auto const cmp = left.Street() <=> right.Street(); cmp != 0) {
         return cmp < 0;
         }

      if (auto const cmp = left.StreetNumberValue() <=> right.StreetNumberValue(); cmp != 0) {
         return cmp < 0;
         }

      return left.StreetNumberSuffix() < right.StreetNumberSuffix();
      };

   oneapi::tbb::parallel_sort( vData.begin(), vData.end(), compare);

   std::vector<directory_group> positions;


   positions.reserve(std::min<std::size_t>(vData.size(), 16'384u));

   fs::path oldPath;

   for (auto first = vData.cbegin();
      first != vData.cend();) {
      auto const& address = first->first;

      auto const last = std::find_if(
         std::next(first),
         vData.cend(),
         [&address](auto const& value) noexcept {
            auto const& other = value.first;

            return address.Street() != other.Street()
                    || address.District() != other.District()
                    || address.UrbanUnit() != other.UrbanUnit()
                    || address.City() != other.City();
         });

      fs::path const currentPath = strPath / address.City() / address.UrbanUnit()
                                    / address.District();

      if (currentPath != oldPath) {
         fs::create_directories(currentPath);
         oldPath = currentPath;
         }

      positions.emplace_back(first, last, currentPath / (address.Street() + ".csv"s));
      first = last;
      }

   std::for_each(positions.cbegin(), positions.cend(),
      [](directory_group const& group) {
         auto const count = static_cast<std::size_t>(
            std::distance(group.first, group.last));

         std::string strBuffer;
         strBuffer.reserve(count * 68u);

         std::for_each(group.first, group.last,
                       [&strBuffer](auto const& value) {
                           auto const& [address, result] = value;
                           std::format_to(std::back_inserter(strBuffer), "{};{};{};{};{};{};{}\n",
                                          address.StreetNumber(), address.ZipCode(),
                                          address.UrbanUnitOld(),
                                          my_Double_to_String_short(address.Latitude(),  9),
                                          my_Double_to_String_short(address.Longitude(), 9),
                                          my_Double_to_String_short(result.first,        3),
                                          my_Double_to_String_short(result.second,       3));
                           });

         std::ofstream output{ group.file };

         output.write(strBuffer.data(), static_cast<std::streamsize>(strBuffer.size()));
      });
}
*/

// Prototype for test function
void Rechentest(fs::path const& directory, int iCount = 1);

