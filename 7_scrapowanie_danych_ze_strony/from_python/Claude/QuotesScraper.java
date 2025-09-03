// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/QuotesScraper.java) ---
package org.example;

import java.io.*;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.net.URI;
import java.time.Duration;
import java.util.*;
import java.util.logging.Logger;
import java.util.logging.Level;
import java.util.logging.ConsoleHandler;
import java.util.logging.SimpleFormatter;
import java.util.concurrent.ThreadLocalRandom;

import org.jsoup.Jsoup;
import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;
import org.jsoup.select.Elements;
import com.opencsv.CSVWriter;

/**
 * Quote class to hold quote data
 */
class Quote {
    private String text;
    private String author;
    private String tags;

    public Quote(String text, String author, String tags) {
        this.text = text;
        this.author = author;
        this.tags = tags;
    }

    // Getters
    public String getText() { return text; }
    public String getAuthor() { return author; }
    public String getTags() { return tags; }

    // Convert to string array for CSV writing
    public String[] toArray() {
        return new String[]{text, author, tags};
    }
}

/**
 * Web scraper for quotes.toscrape.com
 */
public class QuotesScraper {
    private static final Logger logger = Logger.getLogger(QuotesScraper.class.getName());
    private static final String BASE_URL = "https://quotes.toscrape.com";
    private static final String START_URL = BASE_URL + "/page/1/";
    private static final String OUTPUT_CSV = "quotes.csv";

    private final HttpClient httpClient;

    public QuotesScraper() {
        // Configure logging
        configureLogging();

        // Create HTTP client with timeout
        this.httpClient = HttpClient.newBuilder()
                .connectTimeout(Duration.ofSeconds(10))
                .build();
    }

    /**
     * Configure logging to match Python's format
     */
    private void configureLogging() {
        logger.setLevel(Level.INFO);
        logger.setUseParentHandlers(false);

        ConsoleHandler handler = new ConsoleHandler();
        handler.setFormatter(new SimpleFormatter() {
            @Override
            public String format(java.util.logging.LogRecord record) {
                return String.format("[%s] %s%n", record.getLevel(), record.getMessage());
            }
        });

        logger.addHandler(handler);
    }

    /**
     * Fetch a web page and return parsed Document
     */
    private Document getPage(String url) {
        try {
            HttpRequest request = HttpRequest.newBuilder()
                    .uri(URI.create(url))
                    .timeout(Duration.ofSeconds(10))
                    .GET()
                    .build();

            HttpResponse<String> response = httpClient.send(request,
                    HttpResponse.BodyHandlers.ofString());

            if (response.statusCode() >= 200 && response.statusCode() < 300) {
                return Jsoup.parse(response.body());
            } else {
                logger.severe("HTTP error " + response.statusCode() + " for URL: " + url);
                return null;
            }
        } catch (Exception e) {
            logger.severe("Error fetching page " + url + ": " + e.getMessage());
            return null;
        }
    }

    /**
     * Parse quotes from the HTML document
     */
    private List<Quote> parseQuotes(Document doc) {
        List<Quote> quotesData = new ArrayList<>();
        Elements quoteBlocks = doc.select("div.quote");

        for (Element quote : quoteBlocks) {
            try {
                // Extract text
                Element textElement = quote.selectFirst("span.text");
                String text = textElement != null ? textElement.text().trim() : "";

                // Extract author
                Element authorElement = quote.selectFirst("small.author");
                String author = authorElement != null ? authorElement.text().trim() : "";

                // Extract tags
                Elements tagElements = quote.select("a.tag");
                List<String> tagList = new ArrayList<>();
                for (Element tag : tagElements) {
                    tagList.add(tag.text().trim());
                }
                String tags = String.join(", ", tagList);

                quotesData.add(new Quote(text, author, tags));
            } catch (Exception e) {
                logger.warning("Error parsing quote: " + e.getMessage());
            }
        }

        return quotesData;
    }

    /**
     * Get the URL of the next page
     */
    private String getNextPage(Document doc) {
        Element nextBtn = doc.selectFirst("li.next");
        if (nextBtn != null) {
            Element nextLink = nextBtn.selectFirst("a");
            if (nextLink != null) {
                String href = nextLink.attr("href");
                return BASE_URL + href;
            }
        }
        return null;
    }

    /**
     * Save quotes to CSV file
     */
    private void saveToCSV(List<Quote> quotes, String filename) {
        try (FileWriter fileWriter = new FileWriter(filename, java.nio.charset.StandardCharsets.UTF_8);
             CSVWriter writer = new CSVWriter(fileWriter)) {

            // Write header
            String[] header = {"text", "author", "tags"};
            writer.writeNext(header);

            // Write data
            for (Quote quote : quotes) {
                writer.writeNext(quote.toArray());
            }

        } catch (IOException e) {
            logger.severe("Error writing to CSV file: " + e.getMessage());
        }
    }

    /**
     * Main scraping logic
     */
    public void scrape() {
        String url = START_URL;
        List<Quote> allQuotes = new ArrayList<>();

        while (url != null) {
            logger.info("Fetching: " + url);

            Document doc = getPage(url);
            if (doc == null) {
                break;
            }

            List<Quote> quotes = parseQuotes(doc);
            allQuotes.addAll(quotes);

            url = getNextPage(doc);

            // Random delay of 1-3 seconds
            if (url != null) {
                try {
                    double delay = ThreadLocalRandom.current().nextDouble(1.0, 3.0);
                    Thread.sleep((long) (delay * 1000));
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    logger.warning("Sleep interrupted: " + e.getMessage());
                    break;
                }
            }
        }

        if (!allQuotes.isEmpty()) {
            saveToCSV(allQuotes, OUTPUT_CSV);
            logger.info("Saved " + allQuotes.size() + " quotes to file: " + OUTPUT_CSV);
        } else {
            logger.warning("No quotes found.");
        }
    }

    /**
     * Main method
     */
    public static void main(String[] args) {
        QuotesScraper scraper = new QuotesScraper();
        scraper.scrape();
    }
}