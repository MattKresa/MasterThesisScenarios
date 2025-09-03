// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/QuoteScraper.java) ---
package org.example;

import org.jsoup.Jsoup;
import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;
import org.jsoup.select.Elements;

import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.*;
import java.util.logging.Logger;
import java.util.logging.Level;

public class QuoteScraper {
    private static final String BASE_URL = "https://quotes.toscrape.com";
    private static final String START_URL = BASE_URL + "/page/1/";
    private static final String OUTPUT_CSV = "quotes.csv";
    private static final Logger logger = Logger.getLogger(QuoteScraper.class.getName());

    public static void main(String[] args) {
        List<Map<String, String>> allQuotes = new ArrayList<>();
        String url = START_URL;

        while (url != null) {
            logger.info("Fetching: " + url);
            Document doc = getPage(url);
            if (doc == null) break;

            allQuotes.addAll(parseQuotes(doc));
            url = getNextPage(doc);

            try {
                Thread.sleep((long) (Math.random() * 2000 + 1000)); // sleep 1–3s
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                logger.warning("Sleep interrupted");
            }
        }

        if (!allQuotes.isEmpty()) {
            saveToCsv(allQuotes, OUTPUT_CSV);
            logger.info("Saved " + allQuotes.size() + " quotes to file: " + OUTPUT_CSV);
        } else {
            logger.warning("No quotes found.");
        }
    }

    private static Document getPage(String url) {
        try {
            return Jsoup.connect(url).timeout(10000).get();
        } catch (IOException e) {
            logger.log(Level.SEVERE, "Error fetching page " + url, e);
            return null;
        }
    }

    private static List<Map<String, String>> parseQuotes(Document doc) {
        List<Map<String, String>> quotes = new ArrayList<>();
        Elements quoteBlocks = doc.select("div.quote");

        for (Element quote : quoteBlocks) {
            String text = quote.selectFirst("span.text").text();
            String author = quote.selectFirst("small.author").text();
            Elements tagElements = quote.select("a.tag");

            List<String> tags = new ArrayList<>();
            for (Element tag : tagElements) {
                tags.add(tag.text());
            }

            Map<String, String> quoteData = new LinkedHashMap<>();
            quoteData.put("text", text);
            quoteData.put("author", author);
            quoteData.put("tags", String.join(", ", tags));

            quotes.add(quoteData);
        }

        return quotes;
    }

    private static String getNextPage(Document doc) {
        Element nextButton = doc.selectFirst("li.next > a");
        if (nextButton != null) {
            return BASE_URL + nextButton.attr("href");
        }
        return null;
    }

    private static void saveToCsv(List<Map<String, String>> quotes, String filename) {
        try (PrintWriter writer = new PrintWriter(new FileWriter(filename, false))) {
            // Write header
            writer.println("text,author,tags");

            for (Map<String, String> quote : quotes) {
                writer.printf("\"%s\",\"%s\",\"%s\"%n",
                        escapeCsv(quote.get("text")),
                        escapeCsv(quote.get("author")),
                        escapeCsv(quote.get("tags")));
            }
        } catch (IOException e) {
            logger.log(Level.SEVERE, "Error writing to CSV file", e);
        }
    }

    private static String escapeCsv(String value) {
        if (value.contains("\"") || value.contains(",") || value.contains("\n")) {
            value = value.replace("\"", "\"\"");
            return "\"" + value + "\"";
        }
        return value;
    }
}
