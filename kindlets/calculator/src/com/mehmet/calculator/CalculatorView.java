package com.mehmet.calculator;

import java.awt.Color;
import java.awt.Container;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;

/** A lightweight AWT surface, painted by the Kindle's own app framework. */
public final class CalculatorView extends Container implements KeyListener {
    private final Calculator calculator = new Calculator();
    public CalculatorView() {
        setBackground(Color.white); setForeground(Color.black);
        setFocusable(true); addKeyListener(this);
    }
    private void text(Graphics g, String text, int size, int style, int x, int y) {
        g.setFont(new Font("SansSerif", style, size)); g.drawString(text, x, y);
    }
    public void paint(Graphics g) {
        int width = getWidth(), height = getHeight();
        g.setColor(Color.white); g.fillRect(0, 0, width, height); g.setColor(Color.black);
        int left = 24, right = width - 24;
        text(g, "CALCULATOR", 32, Font.BOLD, left, 48);
        text(g, "Kindle Keyboard", 17, Font.PLAIN, left, 78);
        g.drawLine(left, 96, right, 96);
        text(g, calculator.pending(), 24, Font.PLAIN, left, 143);
        String value = calculator.display(); int size = 64;
        while (size > 24) {
            g.setFont(new Font("SansSerif", Font.BOLD, size));
            if (g.getFontMetrics().stringWidth(value) <= right - left) break;
            size -= 2;
        }
        g.drawString(value, left, 228);
        if (calculator.error() != null) text(g, calculator.error(), 23, Font.BOLD, left, 275);
        else text(g, "Four decimal places", 17, Font.PLAIN, left, 275);
        int y = 325;
        g.drawLine(left, y - 16, right, y - 16);
        text(g, "KEYBOARD", 21, Font.BOLD, left, y + 16);
        String letters = "QWERTYUIOP", digits = "1234567890";
        int step = (width - 48) / 10;
        for (int i = 0; i < 10; i++) {
            int x = left + i * step;
            text(g, letters.substring(i, i + 1), 22, Font.BOLD, x, y + 64);
            text(g, digits.substring(i, i + 1), 24, Font.PLAIN, x, y + 99);
        }
        text(g, "A  +       S  -       F  x       D  /", 23, Font.PLAIN, left, y + 153);
        text(g, "Enter  =       .  decimal       N  +/-", 21, Font.PLAIN, left, y + 199);
        text(g, "Del  delete digit       C  clear", 21, Font.PLAIN, left, y + 241);
        int footer = Math.max(y + 285, height - 48);
        g.drawLine(left, footer - 20, right, footer - 20);
        text(g, "Home returns to your library", 18, Font.PLAIN, left, footer + 8);
        text(g, "Calculates left to right", 16, Font.PLAIN, left, footer + 34);
    }
    public void update(Graphics g) { paint(g); }
    public void keyPressed(KeyEvent event) {
        char c = event.getKeyChar();
        if (event.getKeyCode() == KeyEvent.VK_ENTER) c = '=';
        else if (event.getKeyCode() == KeyEvent.VK_BACK_SPACE || event.getKeyCode() == KeyEvent.VK_DELETE) c = '\b';
        c = Calculator.mapKey(c);
        if ((c >= '0' && c <= '9') || "+-*/=.nc\b".indexOf(c) >= 0) {
            calculator.input(c); repaint(); event.consume();
        }
    }
    public void keyTyped(KeyEvent event) { }
    public void keyReleased(KeyEvent event) { }
}
