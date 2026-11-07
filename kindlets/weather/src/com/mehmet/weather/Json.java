package com.mehmet.weather;

import java.util.Hashtable;
import java.util.Vector;

/** Small bounded JSON reader. Rejects truncated, trailing and malformed data. */
public final class Json {
    private final String text;
    private int position, tokens;
    private Json(String text) { this.text = text; }
    public static Object parse(String text) {
        if (text.length() > 16384) throw new IllegalArgumentException("Weather response too large");
        Json parser = new Json(text); Object result = parser.value(0); parser.space();
        if (parser.position != text.length()) parser.fail();
        return result;
    }
    private void fail() { throw new IllegalArgumentException("Invalid weather JSON at " + position); }
    private void space() {
        while (position < text.length() && " \r\n\t".indexOf(text.charAt(position)) >= 0) position++;
    }
    private char take() { if (position >= text.length()) fail(); return text.charAt(position++); }
    private void expect(char c) { space(); if (take() != c) fail(); }
    private Object value(int depth) {
        if (depth > 20 || ++tokens > 1024) fail();
        space(); if (position >= text.length()) fail();
        char c = text.charAt(position);
        if (c == '"') return string();
        if (c == '{') {
            position++; Hashtable result = new Hashtable(); space();
            if (position < text.length() && text.charAt(position) == '}') { position++; return result; }
            while (true) {
                space(); String key = string(); expect(':'); Object item = value(depth + 1);
                result.put(key, item); space(); c = take();
                if (c == '}') return result;
                if (c != ',') fail();
            }
        }
        if (c == '[') {
            position++; Vector result = new Vector(); space();
            if (position < text.length() && text.charAt(position) == ']') { position++; return result; }
            while (true) {
                result.addElement(value(depth + 1)); space(); c = take();
                if (c == ']') return result;
                if (c != ',') fail();
            }
        }
        if (text.startsWith("null", position)) { position += 4; return Null.VALUE; }
        if (text.startsWith("true", position)) { position += 4; return Boolean.TRUE; }
        if (text.startsWith("false", position)) { position += 5; return Boolean.FALSE; }
        int start = position;
        if (c == '-') position++;
        int digits = position;
        while (position < text.length() && digit(text.charAt(position))) position++;
        if (position == digits || (position - digits > 1 && text.charAt(digits) == '0')) fail();
        if (position < text.length() && text.charAt(position) == '.') {
            position++; digits = position;
            while (position < text.length() && digit(text.charAt(position))) position++;
            if (position == digits) fail();
        }
        if (position < text.length() && (text.charAt(position) == 'e' || text.charAt(position) == 'E')) {
            position++;
            if (position < text.length() && (text.charAt(position) == '+' || text.charAt(position) == '-')) position++;
            digits = position;
            while (position < text.length() && digit(text.charAt(position))) position++;
            if (position == digits) fail();
        }
        return Double.valueOf(text.substring(start, position));
    }
    private static boolean digit(char c) { return c >= '0' && c <= '9'; }
    private String string() {
        if (take() != '"') fail();
        StringBuffer result = new StringBuffer();
        while (true) {
            char c = take(); if (c == '"') return result.toString();
            if (c < 32) fail();
            if (c == '\\') {
                c = take();
                if (c == 'u') {
                    int number = 0;
                    for (int i = 0; i < 4; i++) { int d = Character.digit(take(), 16); if (d < 0) fail(); number = number * 16 + d; }
                    c = (char) number;
                } else {
                    String escapes = "\"\\/bfnrt", replacements = "\"\\/\b\f\n\r\t";
                    int index = escapes.indexOf(c); if (index < 0) fail(); c = replacements.charAt(index);
                }
            }
            result.append(c);
        }
    }
    private static final class Null { static final Null VALUE = new Null(); }
}
