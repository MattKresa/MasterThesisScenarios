// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/QuoteScraper.java) ---
package org.example;

import org.jsoup.Jsoup;
import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;
import org.jsoup.select.Elements;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.HashMap;
import java.util.Random;
import java.util.logging.Level;
import java.util.logging.Logger;
import com.opencsv.CSVWriter;

public class QuoteScraper {
    private static final Logger logger = Logger.getLogger(QuoteScraper.class.getName());
    private static final String BASE_URL = "https://quotes.toscrape.com";
    private static final String START_URL = BASE_URL + "/page/1/";
    private static final String OUTPUT_CSV = "quotes.csv";
    private static final HttpClient httpClient = HttpClient.newHttpClient();
    private static final Random random = new Random();

    static {
        // Configure logger
        System.setProperty("java.util.logging.SimpleFormatter.format",
                "[%1$tF %1$tT] [%4$-7s] %5$s %n");
    }

    private static Document getPage(String url) {
        try {
            HttpRequest request = HttpRequest.newBuilder()
                    .uri(URI.create(url))
                    .timeout(java.time.Duration.ofSeconds(10))
                    .build();

            HttpResponse<String> response = httpClient.send(
                    request, HttpResponse.BodyHandlers.ofString());

            if (response.statusCode() != 200) {
                logger.log(Level.SEVERE, "HTTP Error " + response.statusCode() + " fetching: " + url);
                return null;
            }

            return Jsoup.parse(response.body());
        } catch (Exception e) {
            logger.log(Level.SEVERE, "Error fetching page " + url + ": " + e.getMessage());
            return null;
        }
    }

    private static List<Map<String, String>> parseQuotes(Document doc) {
        List<Map<String, String>> quotes = new ArrayList<>();
        Elements quoteBlocks = doc.select("div.quote");

        for (Element quote : quoteBlocks) {
            Map<String, String> quoteData = new HashMap<>();

            String text = quote.selectFirst("span.text").text();
            String author = quote.selectFirst("small.author").text();
            Elements tagElements = quote.select("a.tag");
            String tags = String.join(", ", tagElements.eachText());

            quoteData.put("text", text);
            quoteData.put("author", author);
            quoteData.put("tags", tags);

            quotes.add(quoteData);
        }

        return quotes;
    }

    private static String getNextPage(Document doc) {
        Element nextBtn = doc.selectFirst("li.next > a");
        if (nextBtn != null) {
            return BASE_URL + nextBtn.attr("href");
        }
        return null;
    }

    private static void saveToCsv(List<Map<String, String>> quotes, String filename) throws Exception {
        if (quotes.isEmpty()) {
            logger.warning("No quotes to save");
            return;
        }

        Path path = Paths.get(filename);
        try (CSVWriter writer = new CSVWriter(Files.newBufferedWriter(path))) {
            // Write header
            String[] header = quotes.get(0).keySet().toArray(new String[0]);
            writer.writeNext(header);

            // Write data
            for (Map<String, String> quote : quotes) {
                String[] row = quote.values().toArray(new String[0]);
                writer.writeNext(row);
            }
        }

        logger.info("Saved " + quotes.size() + " quotes to file: " + filename);
    }

    private static void randomDelay() {
        try {
            // Random delay between 1-3 seconds
            int delay = 1000 + random.nextInt(2000);
            Thread.sleep(delay);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }

    public static void main(String[] args) {
        String url = START_URL;
        List<Map<String, String>> allQuotes = new ArrayList<>();

        try {
            while (url != null) {
                logger.info("Fetching: " + url);
                Document doc = getPage(url);
                if (doc == null) {
                    break;
                }

                List<Map<String, String>> quotes = parseQuotes(doc);
                allQuotes.addAll(quotes);

                url = getNextPage(doc);
                if (url != null) {
                    randomDelay();
                }
            }

            if (!allQuotes.isEmpty()) {
                saveToCsv(allQuotes, OUTPUT_CSV);
            } else {
                logger.warning("No quotes found.");
            }
        } catch (Exception e) {
            logger.log(Level.SEVERE, "An error occurred: " + e.getMessage(), e);
        }
    }
}