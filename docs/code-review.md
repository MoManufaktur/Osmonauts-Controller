# Code-Review Version 5 und Prüfung Version 6

Osmonauts – OpenOsmosis Controller · Entwickler: Kai Mommsen & Elias Dörre · Stand: November 2026

## Ergebnis Version 6

- Alle Befunde aus dem Review von Version 5 (unten) sind behoben.
- **Build:** avr-gcc 14 ohne Warnungen für `config.h` und `config_elias.h` (Uno/Nano) sowie `config_kai.h` (Mega mit LED und OLED), jeweils auch im Testmodus `OPERATION_TESTING`.
- **RAM** auf dem Uno mit OLED: statisch 726 statt 1.789 Byte, dazu 512 Byte OLED-Puffer zur Laufzeit. Das passt in 2 KB.
- **Simulation** (`tests/`): 12 Szenarien mit 25 Prüfungen bestehen mit `config_elias.h` und `config_kai.h`.

| Test | Geprüft |
|---|---|
| tap | Vorspülung Membran → Standwasser → Filtern → Wartezeit → Nachspülung, mit allen Dauern |
| tap2 | zweites Zapfen innerhalb von 15 min ohne Vorspülung |
| stop | Stopp in der Vorspülung schließt alles; ein Stopp im Leerlauf beeinflusst den nächsten Zapfvorgang nicht |
| stopfilter | Stopp beim Filtern, die Nachspülung folgt trotzdem |
| disinfect | 2 Injektionen, 7 Pulse mit 10 min Pause, 3 Endspülungen (≈ 90 min) |
| busy | Zapfen wird während der Desinfektion abgelehnt |
| filterchange | 15 + 15 min |
| auto | Auto-Spülung nach 6 h, keine Wiederholung direkt danach |
| tapduringauto / tapduringpost | Zapfen unterbricht die Spülung sauber, nichts bleibt hängen |
| aquastop | Ventile zu nach ≈ 4,5 s, Sperre bleibt, keine Auto-Spülung |
| bounce | prellender Taster ergibt genau einen kurzen Druck |

**Gegenprobe mit Version 5** in derselben Simulation:
- Die Desinfektion ist nach der 2. Injektion in 4 s fertig.
- Der nächste Zapfvorgang überspringt danach die Membranspülung.
- „Stopp“ überspringt nur einen Schritt, statt abzubrechen.
- Wegen fehlender `return`-Anweisungen hängt bzw. stürzt Version 5 mit aktuellem GCC ab.

## Review Version 5 – Befunde

### Kritisch
1. **`wait()` hatte nur einen Timer (static) für alle Zustandsautomaten.** Liefen zwei Abläufe gleichzeitig oder wurde einer abgebrochen, übernahm der nächste Aufruf die alte Startzeit. → v6: eigener Timer pro Ablauf.
2. **Desinfektion, Schritte 4–13: Wartezeiten wurden ignoriert.** Der Zustand wurde bedingungslos erhöht, Pulse und Endspülungen liefen in Sekunden durch.
3. **„Stopp“ übersprang nur den aktuellen Schritt.** `stop_all` wurde nie ausgewertet. Ein Stopp im Leerlauf beendete später den nächsten Schritt sofort.
4. **Keine gegenseitige Sperre der Abläufe.** → v6: zentraler Modus (IDLE, TAPPING, AUTO_FLUSH, POST_FLUSH, FILTER_CHANGE, DISINFECTION, AQUASTOP).
5. **Eine durch Zapfen unterbrochene Nachspülung blieb hängen.**

### Mittel
6. Die Nachspülung wurde während der Auto-Spülung zweimal pro `loop()` aufgerufen.
7. Prellen beim Loslassen konnte einen Phantom-Druck erzeugen.
8. In den Desinfektionsschritten 1–3 liefen Relais- und Serial-Ausgaben in jeder `loop()` (jedes Mal ca. 0,8 s blockiert).
9. `bool`-Funktionen ohne `return` → undefiniertes Verhalten.
10. Der Default-Parameter von `beep()` stand doppelt (kompilierte nur mit `-fpermissive`).
11. Es wurde `intervalMembraneFlush` statt `intervalStandWaterFlush` geprüft.

### Konfiguration und Anzeige
12. `config_kai.h`: `uint8_t rst = 500` lief über → 244 ms.
13. Die alten `config.h` und `config_test.h` passten nicht mehr zum Code.
14. `FEEDBACK` ohne `LED` kompilierte nicht.
15. Ungenutzte Variablen und Funktionen; der Aquastop zeigte keine Displaymeldung.
16. Die Idle-Anzeige überschrieb jede Statusmeldung nach spätestens 1 s. Ein OLED-Fehler hielt den ganzen Controller an (inkl. Aquastop). Zu viel RAM durch Strings ohne `F()`.

### Doku
17. Kommentare zu Taster-Pins, Desinfektionstaster und Desinfektionsablauf passten nicht zum Code.
18. Die damals verteilte `.hex`/`.elf` stammte nicht aus dem Quellstand → Firmware immer aus dem Repo neu kompilieren.

## Offene Punkte

- Die Hydraulik im Blockschaltbild ist aus der Relaislogik abgeleitet. Sie muss mit der realen Anlage abgeglichen werden (Lage des Injektionsports, Durchflussbegrenzer).
- Das Umschalten der Relais blockiert weiterhin (3 × `rst`). In dieser Zeit werden keine Taster gelesen.
- Ein Lang-Druck wird erst beim Loslassen ausgewertet.
- Vor dem Einsatz: Test an der echten Anlage, zuerst mit `OPERATION_TESTING`.
