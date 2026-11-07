package com.mehmet.calculator;

/** Four decimal fixed-point arithmetic; no floating-point rounding surprises. */
public final class Calculator {
    private static final long SCALE = 10000L, LIMIT = 999999999L;
    private String entry = "";
    private long accumulator;
    private char operation;
    private boolean fresh;
    private String error;

    public void clear() {
        entry = ""; accumulator = 0; operation = 0; fresh = false; error = null;
    }
    public String display() { return entry.length() > 0 ? entry : format(accumulator); }
    public String error() { return error; }
    public String pending() { return operation == 0 ? "" : format(accumulator) + " " + operation; }
    private static long number(String s) {
        boolean negative = s.startsWith("-");
        long value = 0; int fractional = -1;
        for (int i = negative ? 1 : 0; i < s.length(); i++) {
            char c = s.charAt(i);
            if (c == '.') fractional = 0;
            else { value = value * 10 + c - '0'; if (fractional >= 0) fractional++; }
        }
        if (fractional < 0) fractional = 0;
        while (fractional++ < 4) value *= 10;
        return negative ? -value : value;
    }
    private static String format(long value) {
        boolean negative = value < 0;
        if (negative) value = -value;
        String text = Long.toString(value / SCALE);
        long remainder = value % SCALE;
        if (remainder != 0) {
            String fraction = Long.toString(SCALE + remainder).substring(1);
            while (fraction.endsWith("0")) fraction = fraction.substring(0, fraction.length() - 1);
            text += "." + fraction;
        }
        return negative ? "-" + text : text;
    }
    private void fail(String message) { error = message; operation = 0; fresh = true; }
    private void calculate(char next) {
        if (error != null) return;
        if (entry.length() == 0) { if (fresh || operation != 0) operation = next; return; }
        long value = number(entry), result = value;
        if (operation == '+') result = accumulator + value;
        if (operation == '-') result = accumulator - value;
        if (operation == '*') result = accumulator * value / SCALE;
        if (operation == '/') {
            if (value == 0) { fail("Cannot divide by zero"); return; }
            result = accumulator * SCALE / value;
        }
        if (result > LIMIT || result < -LIMIT) { fail("Result out of range"); return; }
        accumulator = result; operation = next; fresh = true; entry = "";
    }
    public void input(char c) {
        if (c == 'c') { clear(); return; }
        if (c == '+' || c == '-' || c == '*' || c == '/' || c == '=') {
            calculate(c == '=' ? (char) 0 : c); return;
        }
        if (c == '\b') {
            if (error != null) { clear(); return; }
            if (fresh) { entry = format(accumulator); fresh = false; }
            if (entry.length() > 0) entry = entry.substring(0, entry.length() - 1);
            return;
        }
        if (c == 'n') {
            if (error != null) return;
            if (fresh) { accumulator = -accumulator; return; }
            if (entry.startsWith("-")) entry = entry.substring(1);
            else if (entry.length() > 0) entry = "-" + entry;
            return;
        }
        if ((c < '0' || c > '9') && c != '.') return;
        if (error != null) clear();
        if (fresh) { entry = ""; fresh = false; if (operation == 0) accumulator = 0; }
        int dot = entry.indexOf('.');
        int whole = (dot < 0 ? entry.length() : dot) - (entry.startsWith("-") ? 1 : 0);
        if (c == '.') {
            if (dot >= 0) return;
            if (whole == 0) entry += "0";
        } else {
            if (dot >= 0 && entry.length() - dot - 1 >= 4) return;
            if (dot < 0 && whole >= 5) return;
        }
        entry += c;
    }
    public static char mapKey(char key) {
        key = Character.toLowerCase(key);
        String row = "qwertyuiop";
        int index = row.indexOf(key);
        if (index >= 0) return "1234567890".charAt(index);
        if (key == 'a') return '+';
        if (key == 's') return '-';
        if (key == 'f') return '*';
        if (key == 'd') return '/';
        if (key == '\n' || key == '\r') return '=';
        return key;
    }
}
