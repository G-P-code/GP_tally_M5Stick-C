# GP Tally M5StickC — User Manual / Lietošanas instrukcija

---

## English

### First-time setup

1. Power on the device. It will create a Wi-Fi access point named **m5StickC-plus-xxxxxx** (last 6 characters are unique to your device).
2. Connect to that network from your phone or computer (no password required).
3. Open a browser and go to **192.168.6.1**.
4. Open **Setup**, enter the **Tally Arbiter server IP address and port** (default port: 4455) and click Save. The device restarts and the access point comes back.
5. Open **Configure WiFi**, choose your network, enter its password and click Save. The device connects automatically.

If the saved Wi-Fi network is not available, the device keeps retrying it and opens the same access point after about 20 seconds. The access point closes by itself as soon as the network is back. A long press on the M5 button on the Settings screen opens it without erasing anything, and closes it again.

### Controls

| Control | Action |
|---|---|
| **M5 button** — single click | Toggle between Tally screen and Settings screen |
| **M5 button** — double click | Toggle external LEDs on / off |
| **M5 button** — long press (~1 s) | On the Settings screen: open the setup access point (Wi-Fi settings are kept; closes after 2 min). Any screen: close it if open |
| **Action button** — single click | Cycle display brightness |
| **Action button** — long press | Run LED sequential check (Program → Preview → Aux → All) |
| **Grove pin G33** — short press | Open the setup access point (closes after 2 min; tally keeps running) |
| **Grove pin G33** — hold 3 s | Erase all settings and restart |

### Tally screen

- Background colour shows the current tally state as defined in Tally Arbiter (red = Program, green = Preview, custom colours for Aux and others).
- Device name is shown in the centre.
- When external LEDs are disabled, a small **LED OFF** indicator appears just above the device name.
- The screen rotates automatically when the device is turned upside down.

### Settings screen

Shown automatically on disconnect. Displays:
- Connected Wi-Fi network name
- Device IP address
- Tally Arbiter server address and port
- Connection status (green = connected, red = not connected)
- Battery level
- Firmware version

Whenever the device is on the network, the configuration web portal is available at its IP address (shown on the Settings screen): change the Tally Arbiter server, see device info or upload new firmware.

### External LEDs (optional)

Connect LEDs to the following GPIO pins:

| Signal | Pin |
|---|---|
| Program | G0 (G19 on Plus2) |
| Preview | G26 |
| Aux | G25 |

LED brightness follows the display brightness setting.

### Over-the-air (OTA) firmware update

The device supports OTA updates via PlatformIO (`-ota` environments) or the web portal. Hostname: **m5StickC-plus-xxxxxx**. Password: **tallyarbiter**.

---

## Latviešu

### Pirmreizējā iestatīšana

1. Ieslēdz ierīci. Tā izveidos Wi-Fi piekļuves punktu ar nosaukumu **m5StickC-plus-xxxxxx** (pēdējie 6 simboli ir unikāli katrai ierīcei).
2. Pieslēdzies šim tīklam no telefona vai datora (parole nav nepieciešama).
3. Atver pārlūkprogrammu un dodies uz **192.168.6.1**.
4. Atver **Setup**, ievadi **Tally Arbiter servera IP adresi un portu** (noklusējuma ports: 4455) un nospied Save. Ierīce restartēsies, un piekļuves punkts parādīsies atkal.
5. Atver **Configure WiFi**, izvēlies savu tīklu, ievadi paroli un nospied Save. Ierīce savienosies automātiski.

Ja saglabātais Wi-Fi tīkls nav pieejams, ierīce turpina to meklēt un pēc ~20 sekundēm atver to pašu piekļuves punktu. Tiklīdz tīkls atkal ir pieejams, piekļuves punkts aizveras pats. Ilgs M5 pogas spiediens Iestatījumu ekrānā to atver, neko nedzēšot, un atkal aizver.

### Vadība

| Vadība | Darbība |
|---|---|
| **M5 poga** — viens klikšķis | Pārslēgties starp Tally ekrānu un Iestatījumu ekrānu |
| **M5 poga** — dubultklikšķis | Ieslēgt / izslēgt ārējos LED |
| **M5 poga** — ilgs spiediens (~1 s) | Iestatījumu ekrānā: atvērt iestatīšanas piekļuves punktu (Wi-Fi iestatījumi saglabājas; aizveras pēc 2 min). Jebkurā ekrānā: aizvērt to, ja atvērts |
| **Action poga** — viens klikšķis | Mainīt ekrāna spilgtumu |
| **Action poga** — garš spiediens | Palaist LED secīgo pārbaudi (Program → Preview → Aux → Visi) |
| **Grove kontakts G33** — īss spiediens | Atvērt iestatīšanas piekļuves punktu (aizveras pēc 2 min; tally turpina strādāt) |
| **Grove kontakts G33** — turēt 3 s | Dzēst visus iestatījumus un restartēt |

### Tally ekrāns

- Fona krāsa norāda pašreizējo tally stāvokli, kā definēts Tally Arbiter (sarkans = Program, zaļš = Preview, pielāgotas krāsas Aux un citiem).
- Ekrāna centrā redzams ierīces nosaukums.
- Kad ārējie LED ir izslēgti, tieši virs ierīces nosaukuma parādās mazs indikators **LED OFF**.
- Ekrāns automātiski rotē, kad ierīce tiek apgriezta otrādi.

### Iestatījumu ekrāns

Parādās automātiski, zaudējot savienojumu. Rāda:
- Pieslēgtā Wi-Fi tīkla nosaukumu
- Ierīces IP adresi
- Tally Arbiter servera adresi un portu
- Savienojuma statusu (zaļš = savienots, sarkans = nav savienojuma)
- Akumulatora uzlādes līmeni
- Programmatūras versiju

Kamēr ierīce ir tīklā, konfigurācijas web portāls vienmēr pieejams tās IP adresē (redzama Iestatījumu ekrānā): var mainīt Tally Arbiter serveri, skatīt ierīces informāciju vai augšupielādēt jaunu firmware.

### Ārējie LED (pēc izvēles)

Pieslēdz LED pie šādiem GPIO kontaktiem:

| Signāls | Kontakts |
|---|---|
| Program | G0 (G19 uz Plus2) |
| Preview | G26 |
| Aux | G25 |

LED spilgtums seko ekrāna spilgtuma iestatījumam.

### Programmatūras atjaunināšana pa gaisu (OTA)

Ierīce atbalsta OTA atjaunināšanu caur PlatformIO (`-ota` vides) vai tīmekļa portālu. Resursdatora nosaukums: **m5StickC-plus-xxxxxx**. Parole: **tallyarbiter**.
