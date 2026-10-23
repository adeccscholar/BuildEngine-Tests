# GeoSpatial Skia / OpenGL — Adressdichte und virtuelle Hügellandschaft

> Eigenständiges C++23-Testprogramm auf Basis des **CompareTest**-Datenbestands. Implementierung für BCC64X / BuildEngine; nativer Test mit den Berliner Daten steht aus.

## Zielsetzung

Zwei alternative Eingabequellen und drei Darstellungen: (1) Adresspunkte auf einer Skia-Zeichenfläche, (2) Skia-Dichtekarte, (3) eine **gleitend geglättete Dichtelandschaft mit Bergen und Tälern** als 2D-Relief und interaktive OpenGL-3D-Oberfläche. Die künstlichen Erhebungen zeigen die Anzahl der Adressen im Umkreis, **keine Geländehöhen und keine Einwohnerzahlen**.

## Quellen und Herkunft

Die Daten und die beiden Formate stammen aus [CompareTest](../CompareTest/), konkret den Routinen \`Reading<T>\`, \`WriteToDirectory<T>\` und \`ReadFromDirectory<T>\` in \`MyProcess.h\` sowie \`Test_Read.cpp\`. Die neue Anwendung ist unabhängig kompilierbar und verändert CompareTest nicht.

**A — flache Eingabedatei \`berlin_infos.dat\`:**

\`\`\`text
City;Street;Number;ZipCode;UrbanUnit;UrbanUnitOld;District;Latitude;Longitude
\`\`\`

**B — hierarchische Eingabe, in der die Verzeichnisstruktur selbst Information enthält:**

\`\`\`text
Wurzel/
  Stadt/
    Stadtbezirk/
      Ortsteil/
        Straßenname.csv
\`\`\`

Die sieben CSV-Spalten sind genau die beim CompareTest-Verzeichnisschreiben verbleibenden Felder:

\`\`\`text
StreetNumber;ZipCode;UrbanUnitOld;Latitude;Longitude;Distance;Bearing
\`\`\`

Der neue Reader liest die drei Verzeichnisebenen unterhalb der Stadt, sortiert die Dateien stabil und kann diese mit oneTBB parallel verarbeiten. Im derzeitigen Renderpfad werden die GPS-Werte verwendet; die Namen aus der Hierarchie sind Teil des Eingabeformats, werden aber noch nicht in Bezirksfiltern genutzt. Fehlende/ungültige Koordinaten werden gezählt und nicht dargestellt. Wir verwenden keine automatische Geokodierung.

\`\`\`mermaid
flowchart TD
 A[CompareTest Adressdaten] --> B[berlin_infos.dat]
 A --> C[Stadt / Stadtbezirk / Ortsteil / Strasse.csv]
 B --> D[Sequentieller oder oneTBB-Zeilenparser]
 C --> E[Sequentieller oder oneTBB-Dateiparser]
 D --> F[Validierte GPS-Punkte]
 E --> F
 F --> G[Bounding Box und lokale Projektion]
 G --> H[Skia-Punktansicht]
 G --> I[Skia-Zellendichte]
 G --> J[OpenMP / oneTBB Gauß-Glättung]
 J --> K[Skia 2D-Relief]
 J --> L[OpenGL Dreiecksnetz als 3D-Dichtelandschaft]
\`\`\`

## Kartografische Projektion

Für Berlin wird eine lokale equirektanguläre Näherung verwendet. Mit Längengrad \(\lambda\), Breitengrad \(\varphi\) und Bezugsbreite \(\varphi_0\) gilt:

$$
x=R(\lambda-\lambda_0)\cos(\varphi_0),\qquad
y=R(\varphi-\varphi_0).
$$

In der Software wird der gemeinsame Faktor \(R\) beim Einpassen in die Zeichenfläche weggelassen. Längen und Breiten müssen mit einem gemeinsamen Bezugssystem verarbeitet werden; der Referenzbreitengrad wird aus den Daten gewonnen.

Das Anzeigenrechteck ergibt sich aus \((x_{\min},y_{\min},x_{\max},y_{\max})\), mit 3 % Rand auf jeder Seite. Seitenverhältnisse bleiben erhalten:

$$
s=\min\left(\frac{0.94 W}{x_{\max}-x_{\min}},
            \frac{0.94 H}{y_{\max}-y_{\min}}\right).
$$

Diese lokale Näherung ist nicht für weltweite Daten mit Datumsgrenze oder polarer Nähe bestimmt.

## Dichte, Glättung und Höhe

Sei \(H(x,y)\) die Anzahl der Adressen in der Rasterzelle. Die diskrete Dichtekarte kann anhand einer logarithmischen Skala eingefärbt werden:

$$
c(x,y)=\frac{\log(1+H(x,y))}{\log(1+H_{\max})}.
$$

Für eine **kontinuierlich wirkende Topologie** verwenden wir einen Gauß-Kern:

$$
G_\sigma(u,v)=\frac{1}{2\pi\sigma^2}
\exp\left(-\frac{u^2+v^2}{2\sigma^2}\right).
$$

Die Dichteoberfläche ist eine Faltung:

$$
D(x,y)=(H*G_\sigma)(x,y).
$$

Wegen der Separierbarkeit \(G_\sigma(u,v)=g_\sigma(u)g_\sigma(v)\) lässt sich die Glättung durch einen horizontalen und einen vertikalen Durchlauf effizient ausführen. Das ermöglicht disjunkte Zeilenarbeit ohne konkurrierende Schreibzugriffe.

Die für den OpenGL-Viewer verwendete künstliche Höhe wird durch

$$
z(x,y)=z_{\max}\sqrt{\frac{D(x,y)}{D_{\max}}}
$$

gebildet. Die Wurzel macht auch kleinere Hügel sichtbar. Bereiche geringerer Adressdichte werden zu Tälern; größere Konzentrationen bilden Erhebungen. Das ist eine **synthetische Adressdichte-Topografie** und darf nicht als physikalische Höhenkarte, Gebäudezahl pro Grundstück oder Einwohnerverteilung ausgegeben werden.

**Grenze:** Die erste Implementation zählt Zellen in Pixelkoordinaten; bei unterschiedlichen Auflösungen ändert sich die effektive Zellfläche. Vergleichbare absolute Adressen/km² erfordern später einen metrisch definierten Rastermaßstab.

\`\`\`mermaid
sequenceDiagram
 participant Read as CompareTest Input
 participant TBB as oneTBB
 participant OMP as OpenMP
 participant Skia as Skia 2D
 participant GL as OpenGL 3D
 Read->>TBB: Datei- und Zeilenparsing
 TBB-->>OMP: Koordinatenfolge
 OMP->>OMP: Projektion und Histogramm
 OMP->>OMP: Gauß horizontal + vertikal
 OMP-->>Skia: Rasterpunkte / Dichte / Relief
 OMP-->>GL: Gitterpositionen und Höhen
 GL->>GL: Perspektive, Mausrotation, Zoom
\`\`\`

## Parallelisierung

| Arbeitsschritt | Sequenziell | oneTBB | OpenMP |
|---|---|---|---|
| Flache Datei lesen | Ja | Zeilen | — |
| Hierarchie lesen | Ja | Dateien | — |
| Begrenzungsrechteck | Ja | parallel_reduce | — |
| GPS auf Pixel projizieren | Ja | parallel_for | parallel for |
| Histogramm bilden | Ja | Projektion | Threadlokale Histogramme mit deterministischer Reduktion |
| Gauß-Glättung | Ja | Zeilen | Zeilen |
| Skia/OpenGL zeichnen | GUI-Thread | — | — |

\`\`\`cpp
#pragma omp parallel for num_threads(threads) schedule(static)
for(int row = 0; row < height; ++row) {
   // Only write disjoint destination rows.
   // Read exclusively from the immutable previous convolution pass.
}
\`\`\`

Weder Skia noch OpenGL werden gleichzeitig aus mehreren Worker-Threads auf demselben Canvas/Context verwendet. Bei zu großen lokalen OpenMP-Histogrammen greift derzeit eine sequenzielle, speicherschonende Reduktion.

## Bedienung

| Programm | Zweck |
|---|---|
| \`geospatial_viewer.exe\` | Skia-Punkte, Zellendichte, glattes 2D-Relief |
| \`geospatial_terrain.exe\` | OpenGL 3D-Hügellandschaft aus dem geglätteten Raster |
| \`geospatial_benchmark.exe\` | Vergleich zwischen serieller und paralleler Verarbeitung |

**Skia-Steuerung:** Strg+O öffnet die flache Datei; Strg+D öffnet eine Straßen-CSV und leitet daraus die Hierarchiewurzel ab. Ein Hierarchieverzeichnis kann alternativ beim Programmstart als Argument übergeben werden. Tasten 1/2/3: Punkte/Zelldichte/geglättetes Relief; S/T/P: seriell/oneTBB/OpenMP; +/-: Zellgröße.

**3D-Steuerung:** Linke Maustaste ziehen dreht die Dichteoberfläche, Mausrad zoomt; 1/2/3 stellen Drauf-, Seiten- und Schrägansicht ein; Esc beendet. Beide Quellformate sind als Programmargument vorgesehen. Der OpenGL-Renderer verwendet die System-GPU; keine lokale Mesa-\`opengl32.dll\`.

## Bauen mit der BuildEngine

Aus \`D:/local/embarcadero/test_v3/tests\`:

\`\`\`bat
git pull
cmake -S geospatial-skia -B D:/local/embarcadero/test_v3/build/test/geospatial-skia/Release/cmake -G Ninja -C bootstrap/BuildEngineInit.cmake -DCMAKE_BUILD_TYPE=Release -DBUILDENGINE_CONFIGURATION=Release -DCMAKE_INSTALL_PREFIX=D:/local/embarcadero/test_v3/build/test/geospatial-skia/Release
cmake --build D:/local/embarcadero/test_v3/build/test/geospatial-skia/Release/cmake
cmake --install D:/local/embarcadero/test_v3/build/test/geospatial-skia/Release/cmake
\`\`\`

Beispielaufrufe:

\`\`\`bat
geospatial_viewer.exe D:/Test/berlin_infos.dat
geospatial_terrain.exe D:/Test/berlin_infos.dat
geospatial_terrain.exe D:/Test/Verzeichnisstruktur
geospatial_benchmark.exe D:/Test/berlin_infos.dat
\`\`\`

Verwendet werden die aktiven XML-Versionen von Skia, oneTBB, LLVM OpenMP, fast_float, SDL2 und GLEW sowie die dynamischen BuildEngine-Runtime-DLLs. Intern wird ein CMake-OBJECT-Target statt einer statischen Drittanbieterbibliothek verwendet.

## Dokumentationsserver-Regression

Diese Markdown-Datei testet **Mermaid-Flowchart und Sequence-Diagram**, MathJax mit Inline- und Blockformeln, GFM-Tabellen und **C++-Syntaxhighlighting**. In der Druckansicht sollten Formeln und Diagramme gerendert werden, ohne unvollständige Grafiken oder leere Platzhalter.

## Offene Verifikation

Der tatsächliche Build und die realen Berliner Adressdaten wurden in dieser Umgebung noch nicht ausgeführt. Die ersten Messwerte dürfen deshalb erst nach BCC64X-Consumer-Test und visueller QA angegeben werden. Weitere geplante Erweiterungen: Bezirksfilter aus Pfadmetadaten, metrische Dichte je km², einstellbare Gauß-Bandbreite, Höhen-Skalierung, Normalenbeleuchtung und numerische Testdaten mit bekannten Gipfelpositionen.
