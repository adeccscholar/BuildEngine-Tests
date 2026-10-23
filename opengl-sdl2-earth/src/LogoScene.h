#pragma once

#include <SDL.h>

#include <memory>

/**
 \brief Kapselt die komplette Darstellung und Steuerung der adecc Logo Szene.
 \details Die Klasse besitzt alle OpenGL Ressourcen, Shader, Meshdaten sowie Kamera und Lichtsteuerung,
          die ausschliesslich fuer die Logo Variante benoetigt werden.
 */
class LogoScene {
public:
   /**
    \brief Erzeugt eine noch nicht initialisierte Logo Szene.
    \details OpenGL Ressourcen werden erst durch Initialize erzeugt.
    \returns Nichts.
    \throw Nichts.
    */
   LogoScene() noexcept;

   /**
    \brief Zerstoert die Logo Szene und ihre OpenGL Ressourcen.
    \details Die Ressourcen werden ueber RAII Wrapper automatisch freigegeben.
    \returns Nichts.
    \throw Nichts.
    \pre Der OpenGL Kontext ist waehrend der Zerstoerung noch gueltig.
    */
   ~LogoScene() noexcept;

   LogoScene(LogoScene const&) = delete;
   LogoScene& operator=(LogoScene const&) = delete;
   LogoScene(LogoScene&&) noexcept;
   LogoScene& operator=(LogoScene&&) noexcept;

   /**
    \brief Initialisiert Mesh, Shader, Boden und Szenenzustand.
    \details Das adecc OBJ Mesh wird aus dem graphics Verzeichnis geladen und fuer die Darstellung vorbereitet.
    \returns true bei erfolgreicher Initialisierung, sonst false.
    \throw Nichts.
    */
   bool Initialize() noexcept;

   /**
    \brief Verarbeitet die fuer das Logo gueltige kontinuierliche Tastatursteuerung.
    \details Pfeiltasten bewegen die Kamera. Home und End rotieren den Scheinwerfer,
             PageUp und PageDown aendern seine Hoehe.
    \param flDeltaTime Vergangene Zeit seit dem letzten Frame in Sekunden.
    \returns Nichts.
    \throw Nichts.
    */
   void ProcessInput(float flDeltaTime) noexcept;

   /**
    \brief Verarbeitet ereignisbasierte Eingaben fuer die Logo Szene.
    \details Das Mausrad veraendert den Kameraabstand; nach oben wird herangezoomt, nach unten herausgezoomt.
    \param aEvent Zu verarbeitendes SDL Ereignis.
    \returns Nichts.
    \throw Nichts.
    */
   void ProcessEvent(SDL_Event const& aEvent) noexcept;

   /**
    \brief Rendert die komplette Logo Szene.
    \details Zeichnet Boden, planaren Schatten und das metallische Logo mit gerichtetem Scheinwerfer.
    \param iDrawableWidth Breite des OpenGL Drawable Bereichs in Pixeln.
    \param iDrawableHeight Hoehe des OpenGL Drawable Bereichs in Pixeln.
    \returns Nichts.
    \throw Nichts.
    */
   void Render(int iDrawableWidth, int iDrawableHeight) noexcept;

   /**
    \brief Gibt die OpenGL Ressourcen der Logo Szene vorzeitig frei.
    \details Normalerweise uebernimmt der Destruktor die Freigabe automatisch. Die Methode bleibt fuer einen expliziten Szenenwechsel verfuegbar.
    \returns Nichts.
    \throw Nichts.
    \pre Der OpenGL Kontext ist noch gueltig.
    */
   void Shutdown() noexcept;

private:
   struct Implementation;
   std::unique_ptr<Implementation> pImplementation;
};
