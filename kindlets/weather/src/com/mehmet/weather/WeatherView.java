package com.mehmet.weather;

import java.awt.Color;
import java.awt.Container;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.util.Calendar;

public final class WeatherView extends Container implements KeyListener {
    private Weather weather;
    private String status = "Waiting for weather";
    private int page;
    private Server server;
    private DeviceStatus device = new DeviceStatus();
    private boolean maintenance;
    private long now = System.currentTimeMillis();
    private final WeatherKindlet owner;
    private final java.util.Hashtable pageCodes = new java.util.Hashtable();
    public WeatherView(WeatherKindlet owner) {
        this.owner = owner; setFocusable(true); addKeyListener(this); setBackground(Color.white);
        try {
            // Read the device SDK's actual constants instead of guessing evdev/KDK mappings.
            Class keys = Class.forName("com.amazon.kindle.kindlet.event.KindleKeyCodes");
            java.lang.reflect.Field[] fields = keys.getFields();
            for (int i = 0; i < fields.length; i++) {
                String name = fields[i].getName().toUpperCase();
                if (fields[i].getType() != Integer.TYPE || name.indexOf("PAGE") < 0) continue;
                int step = 0;
                if (name.indexOf("NEXT") >= 0 || name.indexOf("DOWN") >= 0 || name.indexOf("FORWARD") >= 0) step = 1;
                if (name.indexOf("PREV") >= 0 || name.indexOf("UP") >= 0 || name.indexOf("BACK") >= 0) step = -1;
                if (step != 0) pageCodes.put(new Integer(fields[i].getInt(null)), new Integer(step));
            }
        } catch (Exception unavailable) { /* Standard page keys, arrows and F/B remain usable. */ }
    }
    public void update(Weather weather, String status, long now) {
        this.weather = weather; this.status = status; this.now = now; repaint();
    }
    public void dashboard(Weather weather, Server server, DeviceStatus device, boolean maintenance, String status, long now) {
        this.server = server; this.device = device; this.maintenance = maintenance; update(weather, status, now);
    }
    public void setPage(int page) { this.page = (page + 6) % 6; if (owner != null) owner.pageChanged(this.page); repaint(); }
    public int page() { return page; }
    private int sx(int x) { return x * getWidth() / 600; }
    private int sy(int y) { return y * getHeight() / 800; }
    private int fontSize(int size) { return Math.max(10, size * getHeight() / 800); }
    private void font(Graphics g, String family, int size, int style) {
        g.setFont(new Font(family, style, fontSize(size)));
    }
    private void text(Graphics g, String value, int size, int style, int x, int y) {
        font(g, "SansSerif", size, style); g.drawString(value, sx(x), sy(y));
    }
    private void centered(Graphics g, String value, int size, String family, int y) {
        font(g, family, size, Font.PLAIN);
        while (g.getFontMetrics().stringWidth(value) > sx(540) && size > 12) font(g, family, --size, Font.PLAIN);
        g.drawString(value, (getWidth() - g.getFontMetrics().stringWidth(value)) / 2, sy(y));
    }
    private void right(Graphics g, String value, int size, int x, int y) {
        font(g, "SansSerif", size, Font.PLAIN);
        g.drawString(value, sx(x) - g.getFontMetrics().stringWidth(value), sy(y));
    }
    private void fit(Graphics g, String value, int size, int x, int y, int width) {
        font(g, "SansSerif", size, Font.PLAIN);
        while (g.getFontMetrics().stringWidth(value) > sx(width) && size > 10) font(g, "SansSerif", --size, Font.PLAIN);
        g.drawString(value, sx(x), sy(y));
    }
    private void rule(Graphics g, int y) {
        g.setColor(new Color(120, 120, 120)); g.drawLine(sx(32), sy(y), sx(568), sy(y)); g.setColor(Color.black);
    }
    private void analog(Graphics g, int logicalY, int logicalRadius) {
        int x = getWidth() / 2, y = sy(logicalY);
        int radius = Math.min(sx(logicalRadius), sy(logicalRadius));
        g.drawOval(x - radius, y - radius, radius * 2, radius * 2);
        for (int i = 0; i < 60; i++) {
            double angle = i * Math.PI / 30;
            int inner = radius - (i % 5 == 0 ? Math.max(8, sy(13)) : Math.max(3, sy(5)));
            int x1 = x + (int) (inner * Math.sin(angle)), y1 = y - (int) (inner * Math.cos(angle));
            int x2 = x + (int) (radius * Math.sin(angle)), y2 = y - (int) (radius * Math.cos(angle));
            g.drawLine(x1, y1, x2, y2);
            if (i % 5 == 0) g.drawLine(x1 + 1, y1, x2 + 1, y2);
        }
        font(g, "SansSerif", logicalRadius > 150 ? 25 : 19, Font.PLAIN);
        String[] digits = page == 4 ? new String[] {"12", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"} : new String[] {"12", "3", "6", "9"};
        if (page == 4) font(g, "SansSerif", 40, Font.BOLD);
        for (int i = 0; i < digits.length; i++) {
            double a = i * 2 * Math.PI / digits.length; int inset = Math.max(sy(25), g.getFontMetrics().getHeight());
            int tx = x + (int) ((radius - inset) * Math.sin(a));
            int ty = y - (int) ((radius - inset) * Math.cos(a));
            g.drawString(digits[i], tx - g.getFontMetrics().stringWidth(digits[i]) / 2,
                ty + (g.getFontMetrics().getAscent() - g.getFontMetrics().getDescent()) / 2);
        }
        Calendar time = Weather.eastern(now);
        double minute = time.get(Calendar.MINUTE), hour = time.get(Calendar.HOUR) + minute / 60;
        hand(g, x, y, radius * 0.55, hour * Math.PI / 6, logicalRadius > 150 ? 4 : 3);
        hand(g, x, y, radius * 0.78, minute * Math.PI / 30, 2);
        g.fillOval(x - 5, y - 5, 10, 10);
    }
    private void hand(Graphics g, int x, int y, double length, double angle, int thickness) {
        int tailX = x - (int) (length * 0.10 * Math.sin(angle)), tailY = y + (int) (length * 0.10 * Math.cos(angle));
        int endX = x + (int) (length * Math.sin(angle)), endY = y - (int) (length * Math.cos(angle));
        for (int i = -thickness; i <= thickness; i++) {
            g.drawLine(tailX + i, tailY, endX + i, endY); g.drawLine(tailX, tailY + i, endX, endY + i);
        }
    }
    private void icon(Graphics g, int code, int cx, int cy) {
        int x = sx(cx), y = sy(cy), r = Math.min(sx(20), sy(20));
        if (code <= 2) {
            g.drawOval(x - r, y - r, r * 2, r * 2);
            for (int i = 0; i < 8; i++) {
                double a = i * Math.PI / 4;
                g.drawLine(x + (int) (r * 1.4 * Math.sin(a)), y + (int) (r * 1.4 * Math.cos(a)),
                    x + (int) (r * 1.8 * Math.sin(a)), y + (int) (r * 1.8 * Math.cos(a)));
            }
        }
        if (code > 0) {
            // White cloud fill hides the sun behind a partly cloudy outline.
            g.setColor(Color.white);
            g.fillOval(sx(cx - 39), sy(cy - 10), sx(37), sy(35));
            g.fillOval(sx(cx - 22), sy(cy - 31), sx(48), sy(55));
            g.fillOval(sx(cx + 12), sy(cy - 12), sx(34), sy(37));
            g.fillRect(sx(cx - 20), sy(cy), sx(47), sy(25));
            g.setColor(Color.black);
            g.drawArc(sx(cx - 39), sy(cy - 10), sx(37), sy(35), 90, 180);
            g.drawArc(sx(cx - 22), sy(cy - 31), sx(48), sy(55), 0, 180);
            g.drawArc(sx(cx + 12), sy(cy - 12), sx(34), sy(37), 270, 180);
            g.drawLine(sx(cx - 20), sy(cy + 25), sx(cx + 29), sy(cy + 25));
        }
        if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82) || code >= 95) {
            for (int i = -1; i <= 1; i++) g.drawLine(sx(cx + i * 18), sy(cy + 31), sx(cx + i * 18 - 6), sy(cy + 43));
        } else if ((code >= 71 && code <= 77) || code == 85 || code == 86) {
            for (int i = -1; i <= 1; i++) {
                g.drawLine(sx(cx + i * 18), sy(cy + 31), sx(cx + i * 18), sy(cy + 43));
                g.drawLine(sx(cx + i * 18 - 6), sy(cy + 37), sx(cx + i * 18 + 6), sy(cy + 37));
            }
        } else if (code == 45 || code == 48) {
            g.drawLine(sx(cx - 30), sy(cy + 34), sx(cx + 34), sy(cy + 34));
            g.drawLine(sx(cx - 23), sy(cy + 43), sx(cx + 25), sy(cy + 43));
        }
    }
    private String zone() {
        return Weather.eastern(now).get(Calendar.DST_OFFSET) == 0 ? "EASTERN STANDARD TIME" : "EASTERN DAYLIGHT TIME";
    }
    public void paint(Graphics g) {
        g.setColor(Color.white); g.fillRect(0, 0, getWidth(), getHeight()); g.setColor(Color.black);
        text(g, maintenance ? "SSH MODE" : "NORMAL MODE", 24, Font.PLAIN, 32, 29);
        text(g, "Wi-Fi " + device.wifi, 14, Font.PLAIN, 32, 61);
        right(g, device.battery, 14, 568, 28); right(g, device.runtime, 14, 568, 55); rule(g, 78);
        centered(g, Weather.longDate(now), 30, "Serif", 120);
        if (page == 0) overview(g); else if (page == 1) clock(g); else if (page == 2) forecast(g);
        else if (page == 3) quote(g); else if (page == 4) easyRead(g); else server(g);
        rule(g, 746);
        String label = "Updating".equals(status) ? "Updating..." : weather == null ? status : "Updated " + Weather.clock(weather.time);
        if (weather != null && weather.stale(now)) label += " / STALE";
        if (weather != null && !"Updated".equals(status) && !"Updating".equals(status)) label = "Offline / last " + Weather.clock(weather.time);
        if (page == 5) label = server == null ? "Server unavailable" : "Server " + server.updated + (now - server.fetchedAt > 30000 ? " / STALE" : " (5s live)");
        fit(g, label, 14, 32, 764, 270);
        right(g, "M MODE   R REFRESH   HOME EXIT", 12, 568, 764);
        String[] titles = {"1 / 6  OVERVIEW", "2 / 6  CLOCK", "3 / 6  FORECAST", "4 / 6  DAILY QUOTE", "5 / 6  EASY READ", "6 / 6  LENOVO SERVER"};
        centered(g, "< PAGE / B     " + titles[page] + "     F / PAGE >", 14, "SansSerif", 789);
    }
    private void overview(Graphics g) {
        analog(g, 285, 140);
        centered(g, Weather.clock(now), 25, "SansSerif", 462);
        centered(g, zone(), 14, "SansSerif", 490); rule(g, 516);
        if (weather == null) {
            centered(g, "Weather unavailable", 28, "SansSerif", 576);
            centered(g, "Connect Wi-Fi and press R", 20, "SansSerif", 620); return;
        }
        icon(g, weather.code, 110, 567);
        text(g, Weather.decimal(weather.temperature) + "\u00b0F", 65, Font.PLAIN, 218, 586);
        fit(g, Weather.condition(weather.code).toUpperCase(), 19, 225, 625, 342);
        text(g, "Feels like " + Weather.decimal(weather.feels) + " F", 14, Font.PLAIN, 225, 650);
        rule(g, 677);
        text(g, "WIND", 14, Font.PLAIN, 32, 707);
        text(g, Weather.decimal(weather.wind) + " mph", 19, Font.PLAIN, 32, 731);
        text(g, "HUMIDITY", 14, Font.PLAIN, 215, 707);
        text(g, Weather.rounded(weather.humidity) + "%", 19, Font.PLAIN, 215, 731);
        text(g, "HIGH / LOW", 14, Font.PLAIN, 402, 707);
        if (weather.dayCount > 0) fit(g, Weather.decimal(weather.days[0].high) + " / " + Weather.decimal(weather.days[0].low), 18, 402, 731, 166);
        else text(g, "-- / --", 18, Font.PLAIN, 402, 731);
    }
    private void clock(Graphics g) {
        analog(g, 359, 224);
        centered(g, Weather.clock(now), 32, "Serif", 624);
        centered(g, zone(), 14, "SansSerif", 660); rule(g, 682);
        String summary = weather == null ? "Weather unavailable - R to refresh" : Weather.decimal(weather.temperature) + " F   /   " + Weather.condition(weather.code).toUpperCase();
        centered(g, summary, 19, "SansSerif", 718);
    }
    private void forecast(Graphics g) {
        if (weather != null) {
            text(g, "RIGHT NOW", 14, Font.PLAIN, 32, 160);
            text(g, Weather.decimal(weather.temperature) + " F", 65, Font.PLAIN, 28, 231);
            icon(g, weather.code, 487, 221);
            text(g, Weather.condition(weather.code).toUpperCase(), 19, Font.PLAIN, 32, 278);
            fit(g, "Feels like " + Weather.decimal(weather.feels) + " F   /   Humidity " + Weather.rounded(weather.humidity) + "%", 15, 32, 313, 536);
            String highLow = weather.dayCount > 0 ? "   /   Today " + Weather.decimal(weather.days[0].high) + " / " + Weather.decimal(weather.days[0].low) + " F" : "";
            fit(g, "Wind " + Weather.decimal(weather.wind) + " mph" + highLow, 15, 32, 339, 536);
        } else {
            centered(g, "Weather unavailable", 28, "SansSerif", 230);
            centered(g, "Connect Wi-Fi and press R", 20, "SansSerif", 278);
        }
        rule(g, 370); text(g, "THE NEXT TWO DAYS", 14, Font.PLAIN, 32, 398);
        int shown = 0;
        if (weather != null) for (int i = 0; i < weather.dayCount && shown < 2; i++) {
            Weather.Day day = weather.days[i];
            int gap = Weather.dayDifference(now, day.time);
            if (gap < 1 || gap > 2) continue;
            int y = 416 + shown * 162;
            fit(g, Weather.longDate(day.time), 25, 32, y + 24, 536);
            icon(g, day.code, 83, y + 81);
            fit(g, Weather.decimal(day.high) + " / " + Weather.decimal(day.low) + " F", 30, 161, y + 71, 407);
            fit(g, Weather.condition(day.code).toUpperCase(), 19, 161, y + 105, 407);
            fit(g, "Rain " + Weather.rounded(day.rain) + "%   /   Wind up to " + Weather.decimal(day.wind) + " mph", 14, 161, y + 131, 407);
            if (shown == 0) rule(g, y + 151); shown++;
        }
        if (shown == 0) {
            centered(g, "Forecast not available", 25, "SansSerif", 513);
            centered(g, "Press R to download the next two days", 14, "SansSerif", 551);
        }
    }
    private void easyRead(Graphics g) {
        analog(g, 394, 242);
        centered(g, Weather.clock(now), 46, "SansSerif", 691);
        centered(g, zone(), 14, "SansSerif", 720);
    }
    private void quote(Graphics g) {
        centered(g, "DAILY QUOTE", 25, "SansSerif", 211); rule(g, 238);
        font(g, "Serif", 28, Font.PLAIN);
        java.util.Vector lines = new java.util.Vector();
        java.util.StringTokenizer words = new java.util.StringTokenizer(Quotes.forDay(now), " ");
        String line = "";
        while (words.hasMoreTokens()) {
            String word = words.nextToken(), candidate = line.length() == 0 ? word : line + " " + word;
            if (line.length() > 0 && g.getFontMetrics().stringWidth(candidate) > sx(480)) { lines.addElement(line); line = word; }
            else line = candidate;
        }
        if (line.length() > 0) lines.addElement(line);
        int top = 424 - lines.size() * 24;
        for (int i = 0; i < lines.size(); i++) centered(g, (String) lines.elementAt(i), 28, "Serif", top + i * 48);
        rule(g, 610); centered(g, "A fresh thought for each new day", 19, "SansSerif", 662);
        centered(g, "Changes at midnight Eastern time", 14, "SansSerif", 692);
    }
    private void gauge(Graphics g, int x, int y, int width, double percentage) {
        int percent = Math.max(0, Math.min(100, (int) Math.round(percentage)));
        g.drawRect(sx(x), sy(y), sx(width), sy(18));
        g.fillRect(sx(x + 1), sy(y + 1), Math.max(0, sx(width * percent / 100) - 1), Math.max(1, sy(16)));
        for (int i = 1; i < 4; i++) { int tick = sx(x + width * i / 4); g.drawLine(tick, sy(y + 18), tick, sy(y + 23)); }
    }
    private void server(Graphics g) {
        text(g, "LENOVO SERVER", 24, Font.PLAIN, 32, 160);
        if (server == null) {
            rule(g, 198); centered(g, "Server telemetry unavailable", 26, "SansSerif", 380);
            centered(g, "Connect Wi-Fi and press R", 19, "SansSerif", 425); return;
        }
        right(g, "UP " + server.uptime, 18, 568, 160);
        fit(g, server.hostname, 14, 32, 185, 220); right(g, "LOAD " + server.load, 14, 568, 185); rule(g, 198);
        text(g, "CPU", 19, Font.PLAIN, 32, 226);
        right(g, Weather.decimal(server.cpu) + "%  (" + Weather.rounded(server.frequency) + " MHz / " + server.threads + "T)", 14, 568, 226);
        gauge(g, 32, 234, 536, server.cpu);
        text(g, "MEMORY", 19, Font.PLAIN, 32, 284);
        right(g, Weather.decimal(server.memory) + "%  (" + server.memoryUsed + " / " + server.memoryTotal + " GiB  Swap " + Weather.decimal(server.swap) + "%)", 13, 568, 284);
        gauge(g, 32, 292, 536, server.memory);
        text(g, "CPU TEMP", 18, Font.PLAIN, 32, 342); right(g, server.cpuTemperature + " C", 18, 284, 342);
        text(g, "GPU TEMP", 18, Font.PLAIN, 316, 342); right(g, server.gpuTemperature + " C", 18, 568, 342);
        gauge(g, 32, 350, 252, temperatureValue(server.cpuTemperature)); gauge(g, 316, 350, 252, temperatureValue(server.gpuTemperature));
        for (int i = 0; i < server.disks.size(); i++) {
            Server.Disk disk = (Server.Disk) server.disks.elementAt(i); int y = 380 + i * 58;
            fit(g, disk.label, 17, 32, y + 20, 170);
            right(g, Weather.rounded(disk.percent) + "%  (" + disk.used + " / " + disk.total + " GiB, " + disk.available + " GiB free)", 13, 568, y + 20);
            gauge(g, 32, y + 28, 536, disk.percent);
        }
        rule(g, 506); text(g, "TOP PROCESSES", 14, Font.PLAIN, 32, 530);
        right(g, "USER", 14, 355, 530); right(g, "CPU%", 14, 462, 530); right(g, "MEM%", 14, 568, 530);
        for (int i = 0; i < server.processes.size(); i++) {
            Server.Process p = (Server.Process) server.processes.elementAt(i); int y = 562 + i * 38;
            fit(g, p.command, 18, 32, y, 225); right(g, p.user, 13, 355, y);
            right(g, Weather.decimal(p.cpu) + "%", 18, 462, y); right(g, Weather.decimal(p.memory) + "%", 18, 568, y);
        }
    }
    private static double temperatureValue(String text) { try { return Double.parseDouble(text); } catch (Exception missing) { return 0; } }
    public void update(Graphics g) { paint(g); }
    public void keyPressed(KeyEvent event) {
        int code = event.getKeyCode(); char key = Character.toLowerCase(event.getKeyChar());
        Integer step = (Integer) pageCodes.get(new Integer(code));
        if (step != null) setPage(page + step.intValue());
        else if (code == KeyEvent.VK_PAGE_DOWN || code == KeyEvent.VK_RIGHT || key == 'f') setPage(page + 1);
        else if (code == KeyEvent.VK_PAGE_UP || code == KeyEvent.VK_LEFT || key == 'b') setPage(page - 1);
        else if (key == 'r') { if (owner != null) owner.refresh(); }
        else if (key == 'm') { if (owner != null) owner.toggleMode(); }
        else return;
        event.consume();
    }
    public void keyTyped(KeyEvent event) { }
    public void keyReleased(KeyEvent event) { }
}
