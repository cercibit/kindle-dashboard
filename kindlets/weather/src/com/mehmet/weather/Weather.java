package com.mehmet.weather;

import java.util.Hashtable;
import java.util.Vector;
import java.util.Calendar;
import java.util.SimpleTimeZone;

public final class Weather {
    public long time;
    public double temperature, feels, humidity, wind;
    public int code;
    public final Day[] days = new Day[3];
    public int dayCount;
    public static final class Day {
        public long time;
        public double high, low, rain, wind;
        public int code;
    }
    private static Hashtable object(Object o) {
        if (!(o instanceof Hashtable)) throw new IllegalArgumentException("Missing weather object");
        return (Hashtable) o;
    }
    private static double number(Object o) {
        if (!(o instanceof Number)) throw new IllegalArgumentException("Missing weather value");
        double n = ((Number) o).doubleValue();
        if (Double.isNaN(n) || Double.isInfinite(n)) throw new IllegalArgumentException("Invalid weather value");
        return n;
    }
    private static double field(Hashtable o, String key) { return number(o.get(key)); }
    private static double element(Hashtable o, String key, int i) {
        Object array = o.get(key);
        if (!(array instanceof Vector) || ((Vector) array).size() <= i) throw new IllegalArgumentException("Missing forecast");
        return number(((Vector) array).elementAt(i));
    }
    public static Weather parse(String json) {
        Hashtable root = object(Json.parse(json)), current = object(root.get("current"));
        Weather w = new Weather();
        w.time = (long) field(current, "time") * 1000L;
        w.temperature = field(current, "temperature_2m"); w.feels = field(current, "apparent_temperature");
        w.humidity = field(current, "relative_humidity_2m"); w.wind = field(current, "wind_speed_10m");
        w.code = (int) field(current, "weather_code");
        if (w.time < 946684800000L || w.humidity < 0 || w.humidity > 100 || w.wind < 0)
            throw new IllegalArgumentException("Weather outside valid range");
        Hashtable units = object(root.get("current_units"));
        if (!"\u00b0F".equals(units.get("temperature_2m"))) throw new IllegalArgumentException("Expected Fahrenheit");
        if (root.get("daily") instanceof Hashtable) {
            Hashtable daily = (Hashtable) root.get("daily");
            Object times = daily.get("time");
            if (times instanceof Vector) {
                int count = Math.min(3, ((Vector) times).size());
                for (int i = 0; i < count; i++) {
                    Day day = new Day(); day.time = (long) element(daily, "time", i) * 1000L;
                    day.high = element(daily, "temperature_2m_max", i); day.low = element(daily, "temperature_2m_min", i);
                    day.code = (int) element(daily, "weather_code", i); day.rain = element(daily, "precipitation_probability_max", i);
                    day.wind = element(daily, "wind_speed_10m_max", i);
                    if (day.high < day.low || day.rain < 0 || day.rain > 100 || day.wind < 0)
                        throw new IllegalArgumentException("Invalid forecast range");
                    w.days[w.dayCount++] = day;
                }
            }
        }
        return w;
    }
    public static Calendar eastern(long time) {
        SimpleTimeZone zone = new SimpleTimeZone(-5 * 3600000, "America/New_York",
            Calendar.MARCH, 2, Calendar.SUNDAY, 2 * 3600000,
            Calendar.NOVEMBER, 1, Calendar.SUNDAY, 2 * 3600000);
        Calendar date = Calendar.getInstance(zone); date.setTime(new java.util.Date(time)); return date;
    }
    public static String clock(long time) {
        Calendar c = eastern(time); int hour = c.get(Calendar.HOUR); if (hour == 0) hour = 12;
        int minute = c.get(Calendar.MINUTE);
        return hour + ":" + (minute < 10 ? "0" : "") + minute + (c.get(Calendar.AM_PM) == Calendar.AM ? " AM" : " PM");
    }
    public static String date(long time) {
        Calendar c = eastern(time);
        String[] weekdays = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        String[] months = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        return weekdays[c.get(Calendar.DAY_OF_WEEK) - 1] + ", " + months[c.get(Calendar.MONTH)] + " " + c.get(Calendar.DAY_OF_MONTH);
    }
    public static String condition(int code) {
        if (code == 0) return "Clear sky";
        if (code == 1) return "Mostly clear";
        if (code == 2) return "Partly cloudy";
        if (code == 3) return "Overcast";
        if (code == 45 || code == 48) return "Fog";
        if (code >= 51 && code <= 57) return "Drizzle";
        if (code >= 61 && code <= 67) return "Rain";
        if (code >= 71 && code <= 77) return "Snow";
        if (code >= 80 && code <= 82) return "Rain showers";
        if (code == 85 || code == 86) return "Snow showers";
        if (code >= 95 && code <= 99) return "Thunderstorm";
        return "Conditions unavailable";
    }
    public boolean stale(long now) { return now - time > 3600000L || time - now > 900000L; }
    public static String rounded(double value) { return Long.toString(Math.round(value)); }
    public static String decimal(double value) {
        long tenths = Math.round(Math.abs(value) * 10);
        return (value < 0 && tenths != 0 ? "-" : "") + tenths / 10 + "." + tenths % 10;
    }
    public static String longDate(long time) {
        Calendar c = eastern(time);
        String[] weekdays = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
        String[] months = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
        return weekdays[c.get(Calendar.DAY_OF_WEEK) - 1] + ", " + months[c.get(Calendar.MONTH)] + " " + c.get(Calendar.DAY_OF_MONTH);
    }
    public static int dayDifference(long from, long to) {
        Calendar a = eastern(from), b = eastern(to);
        int yearA = a.get(Calendar.YEAR), yearB = b.get(Calendar.YEAR);
        if (yearA == yearB) return b.get(Calendar.DAY_OF_YEAR) - a.get(Calendar.DAY_OF_YEAR);
        // A timestamp shifted by each date's actual UTC offset gives a local civil day.
        long dayA = (from + a.get(Calendar.ZONE_OFFSET) + a.get(Calendar.DST_OFFSET)) / 86400000L;
        long dayB = (to + b.get(Calendar.ZONE_OFFSET) + b.get(Calendar.DST_OFFSET)) / 86400000L;
        return (int) (dayB - dayA);
    }
}
