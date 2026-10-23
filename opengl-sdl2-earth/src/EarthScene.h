#pragma once

#include <SDL.h>

#include <memory>

/**
 \brief Waehlt die fuer die Erde verwendete Oberflaechentextur.
 \details Day verwendet die Tagkarte, Night die Nachtkarte. Beide Varianten verwenden denselben Sternenhintergrund.
 */
enum class EarthTextureMode {
   Day,
   Night
};

/**
 \brief Kapselt Darstellung und Steuerung der Earth Szene.
 \details Die Klasse besitzt Kugelmeshs, Texturen, Shader, Lichtzustand und Kameradistanz.
 */
class EarthScene {
public:
   /**
    \brief Erzeugt eine noch nicht initialisierte Earth Szene.
    \details OpenGL Ressourcen und Texturen werden erst durch Initialize erzeugt.
    \returns Nichts.
    \throw Nichts.
    */
   EarthScene() noexcept;

   /**
    \brief Zerstoert die Earth Szene und ihre OpenGL Ressourcen.
    \details Die Ressourcen werden ueber RAII Wrapper automatisch freigegeben.
    \returns Nichts.
    \throw Nichts.
    \pre Der OpenGL Kontext ist waehrend der Zerstoerung noch gueltig.
    */
   ~EarthScene() noexcept;

   EarthScene(EarthScene const&) = delete;
   EarthScene& operator=(EarthScene const&) = delete;
   EarthScene(EarthScene&&) noexcept;
   EarthScene& operator=(EarthScene&&) noexcept;

   /**
    \brief Initialisiert Shader, Kugelmeshs und die drei Earth Texturen.
    \details Geladen werden 2k_earth_daymap.jpg, 2k_earth_nightmap.jpg und 2k_stars_milky_way.jpg aus graphics.
    \returns true bei erfolgreicher Initialisierung, sonst false.
    \throw Nichts.
    */
   [[nodiscard]] bool Initialize() noexcept;

   /**
    \brief Waehlt Tag- oder Nachttextur fuer die Erde.
    \details Der Szenenzustand und die aktuelle Steuerposition bleiben beim Umschalten erhalten.
    \param aMode Gewuenschte Earth Texturvariante.
    \returns Nichts.
    \throw Nichts.
    */
   void SetMode(EarthTextureMode aMode) noexcept;

   /**
    \brief Verarbeitet kontinuierliche Tastatureingaben fuer die Earth Szene.
    \details Pfeiltasten bewegen die Lichtquelle um die Erde. Plus vergroessert und Minus verkleinert den Kameraabstand.
    \param flDeltaTime Vergangene Zeit seit dem letzten Frame in Sekunden.
    \returns Nichts.
    \throw Nichts.
    */
   void ProcessInput(float flDeltaTime) noexcept;

   /**
    \brief Verarbeitet ereignisbasierte Eingaben fuer die Earth Szene.
    \details Das Mausrad veraendert den Kameraabstand; nach oben wird herangezoomt, nach unten herausgezoomt.
    \param aEvent Zu verarbeitendes SDL Ereignis.
    \returns Nichts.
    \throw Nichts.
    */
   void ProcessEvent(SDL_Event const& aEvent) noexcept;

   /**
    \brief Aktualisiert die langsame Eigenrotation der Erde.
    \details Die Rotation ist zeitbasiert und damit unabhaengig von der Bildrate.
    \param flDeltaTime Vergangene Zeit seit dem letzten Frame in Sekunden.
    \returns Nichts.
    \throw Nichts.
    */
   void Update(float flDeltaTime) noexcept;

   /**
    \brief Rendert Sternenhintergrund und texturierte Erde.
    \details Die aktive Tag- oder Nachttextur wird mit der beweglichen Lichtquelle beleuchtet.
    \param iDrawableWidth Breite des OpenGL Drawable Bereichs in Pixeln.
    \param iDrawableHeight Hoehe des OpenGL Drawable Bereichs in Pixeln.
    \returns Nichts.
    \throw Nichts.
    */
   void Render(int iDrawableWidth, int iDrawableHeight) noexcept;

   /**
    \brief Gibt die OpenGL Ressourcen der Earth Szene vorzeitig frei.
    \details Normalerweise uebernimmt der Destruktor die Freigabe automatisch.
    \returns Nichts.
    \throw Nichts.
    \pre Der OpenGL Kontext ist noch gueltig.
    */
   void Shutdown() noexcept;

private:
   struct Implementation;
   std::unique_ptr<Implementation> pImplementation;
};
