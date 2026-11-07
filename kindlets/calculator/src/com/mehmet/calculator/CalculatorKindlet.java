package com.mehmet.calculator;

import com.amazon.kindle.kindlet.AbstractKindlet;
import com.amazon.kindle.kindlet.KindletContext;
import java.awt.BorderLayout;
import java.awt.Container;
import java.awt.EventQueue;

public final class CalculatorKindlet extends AbstractKindlet {
    private CalculatorView view;
    public void create(KindletContext context) {
        view = new CalculatorView();
        Container root = context.getRootContainer();
        root.setLayout(new BorderLayout());
        root.add(view, BorderLayout.CENTER);
        root.validate();
    }
    public void start() {
        EventQueue.invokeLater(new Runnable() {
            public void run() { if (view != null) { view.requestFocus(); view.repaint(); } }
        });
    }
    public void stop() { }
    public void destroy() { view = null; }
}
