# Standalone Home-screen apps

These Java 1.4 Kindlets open from the Kindle Keyboard's Home library without
opening KUAL. Jailbreak, MKK and the 2025 community developer certificates must
already be installed. No Amazon developer account is needed.

Installable builds are included at [Calculator.azw2](package/Calculator.azw2)
and [WeatherDashboard.azw2](package/WeatherDashboard.azw2). Copy them into
`documents/`; the weather app also needs the native extension and `helper.sh`
as described under Deploy. Their checksums are in [SHA256SUMS](package/SHA256SUMS).

## Apps

- **Calculator**: fixed-point arithmetic with four decimal places, matching the
  native calculator's keyboard layout. Q–P enter 1–0, A/S/F/D select +/−/×/÷,
  Enter calculates, period enters decimals, N changes sign, Del edits and C clears.
  Calculations proceed left to right within ±99999.9999 and truncate fractional
  results toward zero. Home returns to the library.
- **Weather Dashboard**: the native dashboard's six views—Overview, Clock,
  Forecast, Daily Quote, Easy Read and Lenovo Server—with numbered analog dials,
  serif dates and quotes, decimal temperatures, server gauges and process rows.
  Page keys, arrows or F/B cycle all six views. R refreshes the active data source;
  M toggles maintenance mode to keep Wi-Fi and the screen awake; Home exits.

Weather is for Billerica, MA, in Fahrenheit and US Eastern time. The 31 offline
quotes share the exact native app's day cycle. Weather updates hourly; server
telemetry is fetched at five-second intervals **only while page six is active**.
Its default LAN URL is `http://10.0.0.159:8080/api/specs`, matching the native
app. Change it in `weather/helper.sh` to target another server. All displayed
telemetry comes from that endpoint; the JSON fixture is used only by tests.

The Java app draws through the Kindle's own framework. The native HTTPS helper
is used for modern TLS, with the same verified certificates and atomic cache
replacement as the KUAL app. Requests are bounded; failed requests retain the
last valid weather or server reading, and stale data is labeled. Battery and
Wi-Fi status appear in the header. Battery runtime is a rough current-based
estimate. Outside server/maintenance mode the Kindlet follows the stock Kindle
sleep behavior; the native app's direct RTC suspend and radio-off battery-saving
loop remains specific to the KUAL version. The two apps share data caches.

The MKK frontend in `weather/src/ixtab/` is attributed upstream WTFPL code.
Only execution of `/bin/sh` and reads of the two fixed cache files are requested.
The app restores its MKK policy wrapper and screensaver hold when it stops.
`stubs/` contains compile-only API signatures, excluded from both `.azw2` files.

## Build and test

Install a **JDK 8**, Python 3.9+ (with zoneinfo), Pillow is not required by the Java
preview renderer. Set `JAVA8_HOME` if Java 8 is not in the default Linux location.

```sh
python3 kindlets/fetch-deps.py
JAVA8_HOME=/path/to/jdk8 python3 kindlets/build.py all
```

`fetch-deps.py` fetches ECJ 3.21.0 and the **public community test keystore** used
by KUAL, checks their pinned SHA-256 hashes, and stores them in ignored `.tools/`.
No personal keys or credentials belong in this repository. Alternatively set
`ECJ_JAR`, `KEYSTORE` and `STOREPASS`. For an extracted JDK tools.jar plus a Java
8 JRE, set `JARSIGNER_TOOLS_JAR` to that tools.jar. The default store password is
the public test keystore password, `password`.

The build runs arithmetic, parser, forecast, server, battery, offline-quote,
polling and navigation tests. Time conversion is checked against Python zoneinfo
around six years of US DST transitions. It renders six distinct weather previews
and a calculator preview. All application classes are checked for class version
48, and signatures are verified. Outputs go to ignored `kindlets/dist/`; previews
go to each app's ignored `build/`. Desktop tests do not prove device launch or
physical key behavior; verify those after installation.

## Deploy

Build/test the native dashboard first using the root instructions, then build
the Kindlets. Connect the Kindle in USB Drive Mode and run:

```sh
python3 kindlets/deploy.py /media/mehmet/Kindle
```

This deploys both `.azw2` files, the complete six-page native KUAL extension and
the Kindlet helper. It backs up replaced files on the host and verifies all
copied bytes and checksums. Existing weather/server caches are preserved. Safely
eject and unplug USB, then open **Calculator** or **Weather Dashboard** from Home,
or **Weather Clock – Billerica** from KUAL. Do not launch the native app while a
Kindlet update is in progress. Press M in the native app to keep Wi-Fi available
for SSH; its battery-saving mode intentionally turns Wi-Fi off between updates.

Check Q W A E Enter gives 15; all six weather views wrap; R updates weather;
server values update on page six and polling stops on exit; and Home returns
normally. Existing KUAL and local SSH settings are not changed by deployment.
