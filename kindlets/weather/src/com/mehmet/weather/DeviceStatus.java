package com.mehmet.weather;

public final class DeviceStatus {
    public String wifi = "UNKNOWN", battery = "Battery unavailable", runtime = "";
    public static DeviceStatus parse(String text) {
        DeviceStatus s = new DeviceStatus();
        java.util.StringTokenizer lines = new java.util.StringTokenizer(text, "\r\n");
        if (lines.hasMoreTokens()) s.wifi = lines.nextToken();
        try {
            int percent = Integer.parseInt(lines.nextToken().trim());
            int current = Integer.parseInt(lines.nextToken().trim());
            int capacity = Integer.parseInt(lines.nextToken().trim());
            if (percent < 0 || percent > 100 || capacity < 0 || capacity > 5000 || current < -2000 || current > 2000) return s;
            s.battery = "BATTERY " + percent + "%";
            if (current > 0) s.runtime = "Charging";
            else if (current < 0) {
                int minutes = capacity * 60 / -current;
                s.runtime = "~ " + minutes / 60 + "h " + minutes % 60 + "m remaining";
            } else s.runtime = "Estimating runtime...";
        } catch (Exception unavailable) { }
        return s;
    }
}
