// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/QuoteScraper.java) ---
package org.example;

import org.jsoup.Jsoup;
import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;
import org.jsoup.select.Elements;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.charset.StandardCharsets;
import java.time.Duration;
import java.util.*;
import java.util.concurrent.ThreadLocalRandom;
import java.util.logging.Level;
import java.util.logging.Logger;

public class QuoteScraper {

    private static final Logger LOGGER = Logger.getLogger(QuoteScraper.class.getName());
    private static final String BASE_URL = "https://quotes.toscrape.com";
    private static final String START_URL = BASE_URL + "/page/1/";
    private static final String OUTPUT_CSV = "quotes.csv";

    private static final HttpClient client = HttpClient.newBuilder()
            .connectTimeout(Duration.ofSeconds(10))
            .build();

    public static Document getPage(String url) {
        try {
            HttpRequest request = HttpRequest.newBuilder()
                    .uri(URI.create(url))
                    .timeout(Duration.ofSeconds(10))
                    .GET()
                    .build();

            HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());

            if (response.statusCode() == 200) {
                return Jsoup.parse(response.body());
            } else {
                LOGGER.severe("Error fetching page " + url + ": HTTP " + response.statusCode());
            }
        } catch (IOException | InterruptedException e) {
            LOGGER.log(Level.SEVERE, "Error fetching page " + url, e);
        }
        return null;
    }

    public static List<Map<String, String>> parseQuotes(Document doc) {
        List<Map<String, String>> quotesData = new ArrayList<>();
        Elements quoteBlocks = doc.select("div.quote");

        for (Element quote : quoteBlocks) {
            String text = quote.select("span.text").text();
            String author = quote.select("small.author").text();
            List<String> tags = quote.select("a.tag").eachText();

            Map<String, String> quoteMap = new LinkedHashMap<>();
            quoteMap.put("text", text);
            quoteMap.put("author", author);
            quoteMap.put("tags", String.join(", ", tags));

            quotesData.add(quoteMap);
        }
        return quotesData;
    }

    public static String getNextPage(Document doc) {
        Element nextBtn = doc.selectFirst("li.next a");
        if (nextBtn != null) {
            return BASE_URL + nextBtn.attr("href");
        }
        return null;
    }

    public static void saveToCsv(List<Map<String, String>> quotes, String filename) {
        if (quotes.isEmpty()) return;

        try (BufferedWriter writer = new BufferedWriter(
                new FileWriter(filename, StandardCharsets.UTF_8))) {

            // Write header
            Set<String> keys = quotes.get(0).keySet();
            writer.write(String.join(",", keys));
            writer.newLine();

            // Write rows
            for (Map<String, String> quote : quotes) {
                List<String> row = new ArrayList<>();
                for (String key : keys) {
                    // Escape quotes for CSV
                    String value = quote.get(key).replace("\"", "\"\"");
                    row.add("\"" + value + "\"");
                }
                writer.write(String.join(",", row));
                writer.newLine();
            }

        } catch (IOException e) {
            LOGGER.log(Level.SEVERE, "Error saving to CSV", e);
        }
    }

    public static void main(String[] args) {
        String url = START_URL;
        List<Map<String, String>> allQuotes = new ArrayList<>();

        while (url != null) {
            LOGGER.info("Fetching: " + url);
            Document doc = getPage(url);
            if (doc == null) break;

            List<Map<String, String>> quotes = parseQuotes(doc);
            allQuotes.addAll(quotes);

            url = getNextPage(doc);

            // Random delay between 1 and 3 seconds
            try {
                double delay = ThreadLocalRandom.current().nextDouble(1.0, 3.0);
                Thread.sleep((long) (delay * 1000));
            } catch (InterruptedException ignored) {
            }
        }

        if (!allQuotes.isEmpty()) {
            saveToCsv(allQuotes, OUTPUT_CSV);
            LOGGER.info("Saved " + allQuotes.size() + " quotes to file: " + OUTPUT_CSV);
        } else {
            LOGGER.warning("No quotes found.");
        }
    }
}
