package com.mehmet.weather;

import ixtab.jailbreak.Jailbreak;
import java.io.FileInputStream;
import java.io.FilePermission;
import java.io.InputStream;
import java.io.ByteArrayOutputStream;

/** Uses MKK's installed API only for the fixed HTTPS helper and cache path. */
public final class NativeWeather {
    private Jailbreak jailbreak;
    private boolean owned;
    private volatile Process process;
    private boolean awake;
    public void acquire() throws Exception {
        jailbreak = new Jailbreak();
        if (!jailbreak.isAvailable()) throw new Exception("MKK is required for HTTPS updates");
        if (!jailbreak.isEnabled()) { if (!jailbreak.enable()) throw new Exception("MKK permission setup failed"); owned = true; }
        if (!jailbreak.getContext().requestPermission(new FilePermission("/bin/sh", "execute")) ||
            !jailbreak.getContext().requestPermission(new FilePermission("/mnt/us/extensions/weather-clock/weather.json", "read")) ||
            !jailbreak.getContext().requestPermission(new FilePermission("/mnt/us/extensions/weather-clock/server.json", "read")))
            throw new Exception("Weather helper access denied");
    }
    public Weather cached() throws Exception {
        return Weather.parse(read("weather.json"));
    }
    public Server server() throws Exception { return Server.parse(read("server.json")); }
    private String read(String file) throws Exception {
        InputStream stream = new FileInputStream("/mnt/us/extensions/weather-clock/" + file);
        try {
            ByteArrayOutputStream bytes = new ByteArrayOutputStream(); byte[] block = new byte[2048]; int count;
            while ((count = stream.read(block)) != -1) {
                if (bytes.size() + count > 16384) throw new Exception("Weather cache too large");
                bytes.write(block, 0, count);
            }
            return new String(bytes.toByteArray(), "UTF-8");
        } finally { stream.close(); }
    }
    public void fetch(boolean server) throws Exception { command(server ? "server" : "weather"); }
    public String status() throws Exception { return command("status"); }
    public void awake(boolean value) throws Exception {
        if (awake == value) return;
        command(value ? "awake" : "sleep"); awake = value;
    }
    private String command(String argument) throws Exception {
        Process p = Runtime.getRuntime().exec(new String[] {"/bin/sh", "/mnt/us/extensions/weather-clock/kindlet-helper.sh", argument});
        process = p;
        try {
            InputStream stream = p.getInputStream(); byte[] block = new byte[512];
            ByteArrayOutputStream result = new ByteArrayOutputStream(); int count;
            try { while ((count = stream.read(block)) != -1) { if (result.size() + count <= 2048) result.write(block, 0, count); } } finally { stream.close(); }
            if (p.waitFor() != 0) throw new Exception("Update failed; check Wi-Fi and device date");
            return new String(result.toByteArray(), "UTF-8");
        } finally { p.destroy(); if (process == p) process = null; }
    }
    public void cancel() { Process p = process; if (p != null) p.destroy(); }
    public void release() {
        try { awake(false); } catch (Exception ignored) { }
        if (owned && jailbreak != null) { jailbreak.disable(); owned = false; }
    }
}
