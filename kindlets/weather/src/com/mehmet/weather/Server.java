package com.mehmet.weather;

import java.util.Hashtable;
import java.util.Vector;

/** Bounded view of the repository's /api/specs telemetry contract. */
public final class Server {
    public String hostname, uptime, load, updated;
    public double cpu, frequency, memory, swap;
    public String cpuTemperature, gpuTemperature, memoryUsed, memoryTotal, threads;
    public final Vector disks = new Vector(), processes = new Vector();
    public long fetchedAt;
    public static final class Disk { public String label, used, total, available; public double percent; }
    public static final class Process { public String command, user; public double cpu, memory; }
    private static Hashtable object(Object value) {
        if (!(value instanceof Hashtable)) throw new IllegalArgumentException("Missing telemetry object");
        return (Hashtable) value;
    }
    private static double required(Hashtable data, String field) {
        Object value = data.get(field);
        if (!(value instanceof Number)) throw new IllegalArgumentException("Missing telemetry: " + field);
        double n = ((Number) value).doubleValue();
        if (Double.isNaN(n) || Double.isInfinite(n) || n < 0) throw new IllegalArgumentException("Invalid telemetry: " + field);
        return n;
    }
    private static String string(Hashtable data, String field, String fallback, int limit) {
        Object value = data.get(field);
        if (!(value instanceof String)) return fallback;
        String text = (String) value; StringBuffer clean = new StringBuffer();
        for (int i = 0; i < text.length() && i < limit; i++) clean.append(text.charAt(i) < 32 ? ' ' : text.charAt(i));
        return clean.toString();
    }
    private static String temperature(Hashtable data) {
        if (data == null || !(data.get("temp_c") instanceof Number)) return "--";
        return Weather.decimal(((Number) data.get("temp_c")).doubleValue());
    }
    public static String gib(double bytes) { return Weather.decimal(bytes / 1073741824.0); }
    public static Server parse(String json) {
        Hashtable root = object(Json.parse(json));
        if (!Boolean.TRUE.equals(root.get("ok"))) throw new IllegalArgumentException("Server unavailable");
        Hashtable system = object(root.get("system")), cpu = object(root.get("cpu")), memory = object(root.get("memory"));
        Server s = new Server();
        s.hostname = string(system, "hostname", "Lenovo", 32); s.uptime = string(system, "uptime_human", "--", 30);
        s.updated = string(root, "timestamp", "", 40);
        if (s.updated.length() >= 19) s.updated = s.updated.substring(11, 19);
        s.load = "--";
        if (system.get("load_avg") instanceof Vector) {
            Vector load = (Vector) system.get("load_avg"); StringBuffer text = new StringBuffer();
            for (int i = 0; i < load.size() && i < 3; i++) {
                if (!(load.elementAt(i) instanceof Number)) throw new IllegalArgumentException("Invalid load");
                if (i != 0) text.append(" / "); text.append(load.elementAt(i).toString());
            }
            s.load = text.toString();
        }
        s.cpu = required(cpu, "usage_pct"); s.frequency = required(cpu, "avg_freq_mhz");
        s.threads = Weather.rounded(required(cpu, "threads")); s.cpuTemperature = temperature(cpu);
        s.gpuTemperature = temperature(root.get("gpu") instanceof Hashtable ? (Hashtable) root.get("gpu") : null);
        s.memory = required(memory, "used_pct"); s.swap = required(memory, "swap_used_pct");
        double used = required(memory, "used_bytes"), total = required(memory, "total_bytes");
        if (total <= 0 || used > total || s.memory > 100 || s.swap > 100) throw new IllegalArgumentException("Invalid memory");
        s.memoryUsed = gib(used); s.memoryTotal = gib(total);
        if (root.get("storage") instanceof Hashtable) {
            Object mounts = ((Hashtable) root.get("storage")).get("mounts");
            if (mounts instanceof Vector) for (int i = 0; i < ((Vector) mounts).size() && s.disks.size() < 2; i++) {
                Hashtable m = object(((Vector) mounts).elementAt(i)); String mount = string(m, "mountpoint", "", 100);
                if (mount.startsWith("/boot") || mount.length() == 0) continue;
                Disk d = new Disk(); d.percent = required(m, "use_pct");
                if (d.percent > 100) throw new IllegalArgumentException("Invalid storage percentage");
                d.used = gib(required(m, "used_bytes")); d.total = gib(required(m, "size_bytes")); d.available = gib(required(m, "avail_bytes"));
                d.label = "/".equals(mount) ? "SSD  /" : "HDD  " + mount.substring(mount.lastIndexOf('/') + 1);
                s.disks.addElement(d);
            }
        }
        if (root.get("processes") instanceof Vector) {
            Vector processes = (Vector) root.get("processes");
            for (int i = 0; i < processes.size() && i < 5; i++) {
                Hashtable p = object(processes.elementAt(i)); Process row = new Process();
                row.command = string(p, "command", "--", 22); row.user = string(p, "user", "--", 14);
                row.cpu = required(p, "cpu_pct"); row.memory = required(p, "mem_pct"); s.processes.addElement(row);
            }
        }
        s.fetchedAt = System.currentTimeMillis(); return s;
    }
}
