package com.mehmet.weather;

/** Server polling must stop immediately when its page is no longer selected. */
public final class Polling {
    public static boolean fetchServer(int page, long now, long next, long last, boolean refresh) {
        return page == 5 && (now >= next || (refresh && now - last >= 5000));
    }
    public static boolean fetchWeather(int page, long now, long next, long last, boolean refresh) {
        return page != 5 && (now >= next || (refresh && now - last >= 30000));
    }
}
