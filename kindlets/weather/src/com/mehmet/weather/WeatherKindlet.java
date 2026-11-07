package com.mehmet.weather;

import com.amazon.kindle.kindlet.AbstractKindlet;
import com.amazon.kindle.kindlet.KindletContext;
import java.awt.BorderLayout;
import java.awt.Container;
import java.awt.EventQueue;

public final class WeatherKindlet extends AbstractKindlet {
    private WeatherView view;
    private final NativeWeather helper = new NativeWeather();
    private volatile boolean active, refresh, maintenance;
    private volatile int generation, page;
    private Thread worker;
    private Weather weather;
    private Server server;
    private DeviceStatus device = new DeviceStatus();
    private String status = "Starting";
    public void create(KindletContext context) {
        view = new WeatherView(this);
        Container root = context.getRootContainer(); root.setLayout(new BorderLayout());
        root.add(view, BorderLayout.CENTER); root.validate();
    }
    public synchronized void start() {
        if (active) return;
        active = true; refresh = true; final int current = ++generation;
        view.requestFocus();
        worker = new Thread(new Runnable() { public void run() { runWorker(current); } });
        worker.setDaemon(true); worker.start();
    }
    public void refresh() { refresh = true; }
    public void pageChanged(int next) { page = next; }
    public void toggleMode() { maintenance = !maintenance; }
    private void publish(final int current) {
        EventQueue.invokeLater(new Runnable() {
            public void run() { if (active && generation == current && view != null) view.dashboard(weather, server, device, maintenance, status, System.currentTimeMillis()); }
        });
    }
    private void runWorker(int current) {
        synchronized (helper) {
            if (!active || generation != current) return;
            try {
                helper.acquire();
                try { weather = helper.cached(); } catch (Exception ignored) { }
                long nextWeather = 0, nextServer = 0, nextStatus = 0, lastWeather = 0, lastServer = 0, minute = -1;
                int lastPage = -1; boolean lastMode = false;
                while (active && generation == current) {
                    long now = System.currentTimeMillis(); int currentPage = page;
                    boolean changed = currentPage != lastPage || maintenance != lastMode;
                    helper.awake(maintenance || currentPage == 5);
                    if (now >= nextStatus || changed) {
                        try { device = DeviceStatus.parse(helper.status()); } catch (Exception ignored) { }
                        nextStatus = now + 300000L; changed = true;
                    }
                    boolean fetchServer = Polling.fetchServer(currentPage, now, nextServer, lastServer, refresh);
                    boolean fetchWeather = Polling.fetchWeather(currentPage, now, nextWeather, lastWeather, refresh);
                    if (fetchServer || fetchWeather) {
                        refresh = false; status = "Updating"; publish(current);
                        try {
                            helper.fetch(fetchServer);
                            if (fetchServer) server = helper.server(); else weather = helper.cached();
                            status = "Updated";
                        } catch (Exception error) { status = "Check Wi-Fi; R retries"; }
                        now = System.currentTimeMillis();
                        if (fetchServer) { lastServer = now; nextServer = now + 5000L; }
                        else { lastWeather = now; nextWeather = now + 3600000L; }
                        changed = true;
                    }
                    if (changed || now / 60000L != minute) { publish(current); minute = now / 60000L; }
                    lastPage = currentPage; lastMode = maintenance;
                    Thread.sleep(250);
                }
            } catch (InterruptedException stopped) { }
            catch (Exception error) { status = error.getMessage(); publish(current); }
            finally { helper.cancel(); helper.release(); }
        }
    }
    public synchronized void stop() {
        active = false; generation++; helper.cancel(); if (worker != null) worker.interrupt();
    }
    public void destroy() { stop(); view = null; }
}
