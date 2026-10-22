//
#include <condition_variable>
#include <mutex>
#include <deque>
#include <vector>
#include <cstddef>
#include <optional>
#include <utility>


/*!
\brief Gebundene Blocking-Queue für Producer–Consumer
\details \n
- \ref Push blockiert bei vollem Puffer, liefert \c false wenn während des Wartens geschlossen wurde. \n
- \ref Pop blockiert bis ein Element verfügbar ist oder die Queue geschlossen und leer ist, dann \c std::nullopt. \n
- \ref Emplace konstruiert \e in-place. \n
\pre \c uCapacity > 0
\tparam ty Elementtyp (muss move-konstruktibel sein)
*/
template <typename ty>
class BlockingQueue {
public:
   explicit BlockingQueue(std::size_t const uCapacity)
      : uCap{ uCapacity } {
      // \pre
      if (uCap == 0U) { throw std::invalid_argument("BlockingQueue capacity must be > 0"); }
   }

   /*!
   \brief Fügt ein Element ein; blockiert bei vollem Puffer
   \details Liefert \c false, wenn die Queue während des Wartens geschlossen wurde
   \param aVal zu übernehmender Wert (wird ggf. gemoved)
   \returns \c true bei Erfolg, sonst \c false (geschlossen)
   \throw keine (außerfalls ty-Move/Kopie wirft)
   */
   bool Push(ty aVal) {
      std::unique_lock theLk(mtx);
      cvNotFull.wait(theLk, [&] { return isClosed || deq.size() < uCap; });
      if (isClosed) { return false; }
      deq.emplace_back(std::move(aVal));
      theLk.unlock();
      cvNotEmpty.notify_one();
      return true;
   }

   /*!
   \brief Konstruiert ein Element in-place; blockiert bei vollem Puffer
   \details Liefert \c false, wenn die Queue während des Wartens geschlossen wurde
   \tparam Args Konstruktionsargumente
   */
   template <typename... Args>
   bool Emplace(Args&&... theArgs) {
      std::unique_lock theLk(mtx);
      cvNotFull.wait(theLk, [&] { return isClosed || deq.size() < uCap; });
      if (isClosed) { return false; }
      deq.emplace_back(std::forward<Args>(theArgs)...);
      theLk.unlock();
      cvNotEmpty.notify_one();
      return true;
   }

   /*!
   \brief Holt ein Element; blockiert bis verfügbar oder geschlossen+leer
   \returns \c std::optional<ty>{value} bei Erfolg, sonst \c std::nullopt
   */
   std::optional<ty> Pop() {
      std::unique_lock theLk(mtx);
      cvNotEmpty.wait(theLk, [&] { return isClosed || !deq.empty(); });
      if (deq.empty()) { return std::nullopt; }               // geschlossen & leer
      ty aVal = std::move(deq.front());
      deq.pop_front();
      theLk.unlock();
      cvNotFull.notify_one();
      return aVal;
   }

   /*!
   \brief Schließt die Queue idempotent
   \details Weckt alle Wartenden; weitere Push/Emplace schlagen fehl
   */
   void Close() {
      std::lock_guard theG(mtx);
      if (isClosed) { return; }
      isClosed = true;
      cvNotEmpty.notify_all();
      cvNotFull.notify_all();
   }

   /*!
   \brief Aktueller Füllstand (nur ein Schnappschuss)
   */
   std::size_t Size() const {
      std::lock_guard theG(mtx);
      return deq.size();
   }

   /*!
   \brief Prüft, ob die Queue geschlossen ist
   */
   bool IsClosed() const {
      std::lock_guard theG(mtx);
      return isClosed;
   }

private:
   mutable std::mutex mtx;
   std::condition_variable cvNotEmpty;
   std::condition_variable cvNotFull;
   std::deque<ty> deq;
   std::size_t uCap;
   bool isClosed{ false };
};