// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/QuoteScraper.java) ---
package org.example;

import org.jsoup.Jsoup;
import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;
import org.jsoup.select.Elements;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.concurrent.ThreadLocalRandom;

public class QuoteScraper {

    private static final String BASE_URL = "https://quotes.toscrape.com";
    private static final String START_URL = BASE_URL + "/page/1/";
    private static final String OUTPUT_CSV = "quotes.csv";

    // Fetch page content
    public static String getPage(String urlStr) {
        StringBuilder content = new StringBuilder();
        HttpURLConnection conn = null;
        try {
            URL url = new URL(urlStr);
            conn = (HttpURLConnection) url.openConnection();
            conn.setConnectTimeout(10000);
            conn.setReadTimeout(10000);

            try (BufferedReader reader = new BufferedReader(
                    new InputStreamReader(conn.getInputStream(), StandardCharsets.UTF_8))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    content.append(line).append("\n");
                }
            }
        } catch (IOException e) {
            System.err.println("[ERROR] Failed to fetch: " + urlStr + " - " + e.getMessage());
            return "";
        } finally {
            if (conn != null) conn.disconnect();
        }
        return content.toString();
    }

    // Parse quotes from the HTML
    public static List<Quote> parseQuotes(Document doc) {
        List<Quote> quotes = new ArrayList<>();
        Elements quoteDivs = doc.select("div.quote");

        for (Element quoteDiv : quoteDivs) {
            String text = quoteDiv.selectFirst("span.text") != null
                    ? quoteDiv.selectFirst("span.text").text()
                    : "";
            String author = quoteDiv.selectFirst("small.author") != null
                    ? quoteDiv.selectFirst("small.author").text()
                    : "";
            List<String> tags = new ArrayList<>();
            for (Element tag : quoteDiv.select("a.tag")) {
                tags.add(tag.text());
            }
            quotes.add(new Quote(text, author, String.join(", ", tags)));
        }
        return quotes;
    }

    // Find the "next" page link
    public static String getNextPage(Document doc) {
        Element nextLi = doc.selectFirst("li.next > a");
        if (nextLi != null) {
            return BASE_URL + nextLi.attr("href");
        }
        return "";
    }

    // Save quotes to CSV file
    public static void saveToCsv(List<Quote> quotes, String filename) {
        try (PrintWriter pw = new PrintWriter(new OutputStreamWriter(
                new FileOutputStream(filename), StandardCharsets.UTF_8))) {
            pw.println("text,author,tags");
            for (Quote q : quotes) {
                pw.printf("\"%s\",\"%s\",\"%s\"%n",
                        q.text.replace("\"", "\"\""),
                        q.author.replace("\"", "\"\""),
                        q.tags.replace("\"", "\"\""));
            }
        } catch (IOException e) {
            System.err.println("[ERROR] Unable to save CSV: " + e.getMessage());
        }
    }

    public static void main(String[] args) {
        String url = START_URL;
        List<Quote> allQuotes = new ArrayList<>();

        while (!url.isEmpty()) {
            System.out.println("[INFO] Fetching: " + url);

            String html = getPage(url);
            if (html.isEmpty()) break;

            Document doc = Jsoup.parse(html);
            allQuotes.addAll(parseQuotes(doc));

            url = getNextPage(doc);

            // Random delay between 1 and 3 seconds
            try {
                Thread.sleep(ThreadLocalRandom.current().nextInt(1000, 3001));
            } catch (InterruptedException ignored) {}
        }

        if (!allQuotes.isEmpty()) {
            saveToCsv(allQuotes, OUTPUT_CSV);
            System.out.println("[INFO] Saved " + allQuotes.size() + " quotes to file: " + OUTPUT_CSV);
        } else {
            System.err.println("[WARNING] No quotes found.");
        }
    }

    // Simple data holder for quotes
    static class Quote {
        String text;
        String author;
        String tags;

        Quote(String text, String author, String tags) {
            this.text = text;
            this.author = author;
            this.tags = tags;
        }
    }
}
