#pragma once

#include "TensorField.h"

#include <oneapi/tbb/task_arena.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>
#include <utility>

namespace tensor_nebula {

/**
\brief Calculates immutable xtensor frames independently from the UI and GL threads.
\details A single std::jthread coalesces incoming requests: when the user moves
   a slider or animation advances, only the most recent pending FieldSettings are
   retained. The oneTBB arena bounds CPU parallelism to leave cores for SDL and
   software Mesa. Completed frames are published as immutable shared ownership.
   Neither the worker nor oneTBB threads ever call OpenGL or ImGui.
\note The worker is joined by RAII before the OpenGL context is destroyed.
*/
class AsyncField final {
   struct Request {
      FieldSettings theSettings;
      std::uint64_t uRevision {};
   };

   mutable std::mutex theMutex;
   std::condition_variable_any theWake;
   std::optional<Request> thePending;
   std::shared_ptr<TensorField<float> const> pCompleted;
   std::exception_ptr pFailure;
   std::uint64_t uRequested {};
   std::uint64_t uCompleted {};
   std::jthread theWorker;

   /**
   \brief Computes the newest pending request within a bounded oneTBB task arena.
   \param theStop Stop token owned by the worker.
   \returns Nothing.
   \throw Nothing; exceptions are relayed to the UI from a short locked section.
   */
   void Run(std::stop_token const theStop) noexcept {
      // Reserve execution capacity for the responsive UI and the Mesa renderer.
      unsigned const uHardware = std::max(1U,std::thread::hardware_concurrency());
      int const iParallelism=std::max(1,std::min(6,static_cast<int>(uHardware > 2 ? uHardware-2 : 1)));
      oneapi::tbb::task_arena theArena {iParallelism};
      while(!theStop.stop_requested()) {
         Request theRequest;
         {
            std::unique_lock theLock {theMutex};
            bool const bReady=theWake.wait(theLock,theStop,[&] {
               return thePending.has_value();
            });
            if(!bReady)break;
            theRequest=*thePending;
            thePending.reset();
         }
         try {
            auto pNew=std::make_shared<TensorField<float>>();
            theArena.execute([&] {
               pNew->Compute(theRequest.theSettings);
            });
            {
               std::scoped_lock theLock {theMutex};
               if(theRequest.uRevision==uRequested && !theStop.stop_requested()) {
                  pCompleted=std::move(pNew);
                  uCompleted=theRequest.uRevision;
                  pFailure=nullptr;
               }
            }
         }
         catch(...) {
            std::scoped_lock theLock {theMutex};
            if(theRequest.uRevision==uRequested) {
               pFailure=std::current_exception();
               uCompleted=theRequest.uRevision;
            }
         }
      }
   }

public:
   /**
   \brief Starts a dedicated worker that can never enter ImGui or OpenGL.
   \details The worker is idle until an initial FieldSettings request arrives.
   \throw std::system_error When the system cannot create a worker thread.
   */
   AsyncField() : theWorker {[this](std::stop_token const theStop) {
      Run(theStop);
   }} {}

   AsyncField(AsyncField const&)=delete;
   AsyncField& operator=(AsyncField const&)=delete;
   ~AsyncField() {
      theWorker.request_stop();
      theWake.notify_all();
      if(theWorker.joinable())theWorker.join();
   }

   /**
   \brief Replaces any queued obsolete request without blocking numerical work.
   \param theSettings Complete value copy of current settings.
   \returns Monotonically increasing request revision.
   \throw std::system_error On an unexpected mutex failure.
   \post Only the newest unstarted request is retained.
   */
   [[nodiscard]] std::uint64_t RequestFrame(FieldSettings const& theSettings) {
      std::uint64_t uRevision {};
      {
         std::scoped_lock theLock {theMutex};
         uRevision=++uRequested;
         thePending=Request {theSettings,uRevision};
      }
      theWake.notify_one();
      return uRevision;
   }

   /**
   \brief Takes a newly completed immutable CPU tensor snapshot.
   \details No GPU call occurs under the mutex, so the expensive GL upload
      always happens after the lock is released on the presentation thread.
   \returns A new complete field, or null when nothing newer is ready.
   \throw std::exception Re-throws current worker errors without data races.
   */
   [[nodiscard]] std::shared_ptr<TensorField<float> const> TakeCompleted() {
      std::exception_ptr pError;
      std::shared_ptr<TensorField<float> const> pResult;
      {
         std::scoped_lock theLock {theMutex};
         pError=std::exchange(pFailure,nullptr);
         pResult=std::move(pCompleted);
      }
      if(pError)std::rethrow_exception(pError);
      return pResult;
   }

   /**
   \brief Returns current request/finished revisions for optional diagnostics.
   \returns Pair of revision numbers from one short synchronized snapshot.
   \throw std::system_error On an unexpected mutex failure.
   */
   [[nodiscard]] std::pair<std::uint64_t,std::uint64_t> Revisions() const {
      std::scoped_lock theLock {theMutex};
      return {uRequested,uCompleted};
   }
};
}
