package com.amazon.kindle.kindlet;
/** Compile-only signatures; supplied by the device, excluded from the app. */
public abstract class AbstractKindlet {
    public void create(KindletContext context) { }
    public void start() { }
    public void stop() { }
    public void destroy() { }
}
