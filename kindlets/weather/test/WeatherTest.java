import com.mehmet.weather.*;
import java.io.*;
import java.util.Calendar;
import java.awt.image.BufferedImage;
import java.awt.Graphics;
import java.awt.event.KeyEvent;
import javax.imageio.ImageIO;

public final class WeatherTest {
    private static int count;
    private static void check(boolean condition) { if (!condition) throw new RuntimeException("Test " + count + " failed"); count++; }
    private static String read(String name) throws Exception {
        InputStream in = new FileInputStream(name); ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] block = new byte[1024]; int n; while ((n = in.read(block)) != -1) out.write(block, 0, n);
        in.close(); return new String(out.toByteArray(), "UTF-8");
    }
    private static void invalid(String text) {
        try { Weather.parse(text); } catch (IllegalArgumentException expected) { count++; return; }
        throw new RuntimeException("Accepted invalid data");
    }
    public static void main(String[] args) throws Exception {
        String live = read("test/live-weather.json"); Weather weather = Weather.parse(live);
        check(weather.dayCount == 3); check(weather.days[0].high >= weather.days[0].low);
        check(!weather.stale(weather.time + 3600000)); check(weather.stale(weather.time + 3600001));
        check(weather.stale(weather.time - 900001));
        check("54.2".equals(Weather.decimal(54.2)));
        check("-0.1".equals(Weather.decimal(-0.1)));
        check("0.0".equals(Weather.decimal(-0.01)));
        check(Weather.dayDifference(1793516400000L, 1793606400000L) == 1);
        check(Weather.dayDifference(1798765200000L, 1798851600000L) == 1);
        Server server = Server.parse(read("test/server.json"));
        check(server.disks.size() == 2); check(server.processes.size() == 5);
        check("1.9".equals(server.memoryUsed)); check("7.7".equals(server.memoryTotal));
        check("SSD  /".equals(((Server.Disk) server.disks.elementAt(0)).label));
        check("HDD  Lenovo_HDD".equals(((Server.Disk) server.disks.elementAt(1)).label));
        check(server.cpu == 12.4); check("09:30:15".equals(server.updated));
        boolean rejectedServer = false;
        try { Server.parse(read("test/server.json").replace("\"ok\": true", "\"ok\": false")); }
        catch (IllegalArgumentException expected) { rejectedServer = true; }
        check(rejectedServer);
        check("--".equals(Server.parse(read("test/server.json").replace("\"gpu\": {", "\"unused_gpu\": {")).gpuTemperature));
        for (int p = 0; p < 6; p++) {
            check(Polling.fetchServer(p, 10000, 5000, 0, false) == (p == 5));
            check(Polling.fetchWeather(p, 10000, 5000, 0, false) == (p != 5));
            check(!Polling.fetchWeather(p, 10000, 60000, 0, true));
        }
        check(!Polling.fetchServer(5, 10000, 20000, 9000, true));
        check(Polling.fetchServer(5, 15000, 20000, 9000, true));
        check(Quotes.forDay(weather.time).equals(Quotes.forDay(weather.time + 31L * 86400000)));
        check(!Quotes.forDay(weather.time).equals(Quotes.forDay(weather.time + 86400000)));
        check("BATTERY 80%".equals(DeviceStatus.parse("CONNECTED\n80\n-100\n800\n").battery));
        check("Charging".equals(DeviceStatus.parse("CONNECTED\n80\n100\n800\n").runtime));
        invalid(live.substring(0, live.length() / 2)); invalid(live + "bad"); invalid("{}");
        invalid(live.replaceAll("temperature_2m\"", "missing_temperature\""));
        invalid(live.replaceAll("\u00b0F", "\u00b0C"));
        String[] badJson = {"[1,]", "{\"x\":01}", "{\"x\":1.}", "{\"x\":1e}", "{\"x\":\"\\q\"}", "{\"x\":true,}"};
        for (int i = 0; i < badJson.length; i++) {
            boolean rejected = false; try { Json.parse(badJson[i]); } catch (IllegalArgumentException expected) { rejected = true; }
            check(rejected);
        }
        BufferedReader input = new BufferedReader(new FileReader("test/eastern.csv")); String line;
        while ((line = input.readLine()) != null) {
            java.util.StringTokenizer tokens = new java.util.StringTokenizer(line, ",");
            Calendar c = Weather.eastern(Long.parseLong(tokens.nextToken()));
            check(c.get(Calendar.YEAR) == Integer.parseInt(tokens.nextToken()));
            check(c.get(Calendar.MONTH) + 1 == Integer.parseInt(tokens.nextToken()));
            check(c.get(Calendar.DAY_OF_MONTH) == Integer.parseInt(tokens.nextToken()));
            check(c.get(Calendar.HOUR_OF_DAY) == Integer.parseInt(tokens.nextToken()));
            check(c.get(Calendar.MINUTE) == Integer.parseInt(tokens.nextToken()));
        }
        input.close();
        WeatherView view = new WeatherView(null); view.setSize(600, 730);
        server.fetchedAt = weather.time;
        view.dashboard(weather, server, DeviceStatus.parse("CONNECTED\n80\n-100\n800\n"), false, "Updated", weather.time);
        java.util.Hashtable hashes = new java.util.Hashtable();
        for (int i = 0; i < 6; i++) {
            view.setPage(i);
            BufferedImage image = new BufferedImage(600, 730, BufferedImage.TYPE_BYTE_GRAY);
            Graphics g = image.getGraphics(); view.paint(g); g.dispose();
            ImageIO.write(image, "png", new File("build/preview-" + i + ".png"));
            java.util.zip.CRC32 checksum = new java.util.zip.CRC32();
            for (int y = 0; y < 730; y++) for (int x = 0; x < 600; x++) checksum.update(image.getRGB(x, y));
            hashes.put(new Long(checksum.getValue()), Boolean.TRUE);
        }
        check(hashes.size() == 6);
        view.setSize(600, 700);
        view.setPage(0);
        BufferedImage compact = new BufferedImage(600, 700, BufferedImage.TYPE_BYTE_GRAY);
        Graphics cg = compact.getGraphics(); view.paint(cg); cg.dispose();
        ImageIO.write(compact, "png", new File("build/preview-compact.png"));
        view.setPage(0);
        view.keyPressed(new KeyEvent(view, KeyEvent.KEY_PRESSED, 0, 0, KeyEvent.VK_PAGE_UP, KeyEvent.CHAR_UNDEFINED)); check(view.page() == 5);
        view.keyPressed(new KeyEvent(view, KeyEvent.KEY_PRESSED, 0, 0, KeyEvent.VK_PAGE_DOWN, KeyEvent.CHAR_UNDEFINED)); check(view.page() == 0);
        view.keyPressed(new KeyEvent(view, KeyEvent.KEY_PRESSED, 0, 0, KeyEvent.VK_F, 'f')); check(view.page() == 1);
        System.out.println(count + " parser, server, quote, polling, DST and navigation checks passed; six distinct views rendered.");
    }
}
