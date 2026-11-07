import com.mehmet.calculator.Calculator;
import com.mehmet.calculator.CalculatorView;
import java.awt.Graphics;
import java.awt.image.BufferedImage;
import java.awt.event.KeyEvent;
import java.io.File;
import javax.imageio.ImageIO;

public final class CalculatorTest {
    private static int count;
    private static void check(String keys, String value, String error) {
        Calculator c = new Calculator();
        for (int i = 0; i < keys.length(); i++) c.input(keys.charAt(i));
        if (!value.equals(c.display()) || (error == null ? c.error() != null : !error.equals(c.error())))
            throw new RuntimeException(keys + " expected " + value + ", got " + c.display() + " / " + c.error());
        count++;
    }
    public static void main(String[] args) throws Exception {
        check("12+3=", "15", null);
        check("12.5*2=", "25", null);
        check("1/3=", "0.3333", null);
        check("2-5=", "-3", null);
        check("2+3*4=", "20", null);
        check("99999.9999+0.0001=", "0.0001", "Result out of range");
        check("12/0=", "0", "Cannot divide by zero");
        check("12/0=7", "7", null);
        check("12/0=c", "0", null);
        check(".5*2=", "1", null);
        check("1..25", "1.25", null);
        check("1.234567", "1.2345", null);
        check("1234567", "12345", null);
        check("12\b", "1", null);
        check("12+3=\b", "1", null);
        check("12n+3=", "-9", null);
        check("12+3=n", "-15", null);
        check("12nn", "12", null);
        check("12+-3=", "9", null);
        check("5=+3=", "8", null);
        check("5=3", "3", null);
        check("1/0=c2+3=", "5", null);
        check("99999*99999=", "99999", "Result out of range");
        check("1n/3=", "-0.3333", null);
        String keys = "QWERTYUIOPasfd\n", values = "1234567890+-*/=";
        for (int i = 0; i < keys.length(); i++) {
            if (Calculator.mapKey(keys.charAt(i)) != values.charAt(i)) throw new RuntimeException("key mapping");
            count++;
        }
        CalculatorView view = new CalculatorView(); view.setSize(600, 730);
        String example = "qwae";
        for (int i = 0; i < example.length(); i++)
            view.keyPressed(new KeyEvent(view, KeyEvent.KEY_PRESSED, 0, 0, KeyEvent.VK_UNDEFINED, example.charAt(i)));
        view.keyPressed(new KeyEvent(view, KeyEvent.KEY_PRESSED, 0, 0, KeyEvent.VK_ENTER, '\n'));
        BufferedImage image = new BufferedImage(600, 730, BufferedImage.TYPE_BYTE_GRAY);
        Graphics g = image.getGraphics(); view.paint(g); g.dispose();
        ImageIO.write(image, "png", new File("build/preview.png"));
        System.out.println(count + " arithmetic and keyboard checks passed; UI preview rendered.");
    }
}
