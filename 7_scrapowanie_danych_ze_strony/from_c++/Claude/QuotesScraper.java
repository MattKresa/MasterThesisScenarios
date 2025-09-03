// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/QuotesScraper.java) ---
package org.example;

import java.io.FileWriter;
import java.io.IOException;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.ThreadLocalRandom;

import org.jsoup.Jsoup;
import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;
import org.jsoup.select.Elements;

import com.opencsv.CSVWriter;

/**
 * Quote data structure to hold text, author, and tags
 */
class Quote {
    private final String text;
    private final String author;
    private final String tags;

    public Quote(String text, String author, String tags) {
        this.text = text;
        this.author = author;
        this.tags = tags;
    }

    public String getText() { return text; }
    public String getAuthor() { return author; }
    public String getTags() { return tags; }

    public String[] toArray() {
        return new String[]{text, author, tags};
    }

    @Override
    public String toString() {
        return String.format("Quote{text='%s', author='%s', tags='%s'}", text, author, tags);
    }
}

/**
 * Web scraper for quotes.toscrape.com - Java translation of C++ code
 */
public class QuotesScraper {
    private static final String BASE_URL = "https://quotes.toscrape.com";
    private static final String START_URL = BASE_URL + "/page/1/";
    private static final String OUTPUT_CSV = "quotes.csv";

    private final HttpClient httpClient;
    private final Random random;

    public QuotesScraper() {
        this.httpClient = HttpClient.newBuilder()
                .connectTimeout(Duration.ofSeconds(10))
                .build();
        this.random = new Random();
    }

    /**
     * Fetch page content using HttpClient
     * Equivalent to C++ get_page function
     */
    private String getPage(String url) {
        try {
            HttpRequest request = HttpRequest.newBuilder()
                    .uri(URI.create(url))
                    .timeout(Duration.ofSeconds(10))
                    .GET()
                    .build();

            HttpResponse<String> response = httpClient.send(request,
                    HttpResponse.BodyHandlers.ofString());

            if (response.statusCode() >= 200 && response.statusCode() < 300) {
                return response.body();
            } else {
                System.err.println("[ERROR] Failed to fetch: " + url + " - HTTP " + response.statusCode());
                return "";
            }
        } catch (Exception e) {
            System.err.println("[ERROR] Failed to fetch: " + url + " - " + e.getMessage());
            return "";
        }
    }

    /**
     * Parse quotes from HTML content
     * Equivalent to C++ parse_quotes function
     */
    private List<Quote> parseQuotes(String html) {
        List<Quote> quotes = new ArrayList<>();

        try {
            Document doc = Jsoup.parse(html);
            Elements quoteBlocks = doc.select("div.quote");

            for (Element quoteBlock : quoteBlocks) {
                String text = "";
                String author = "";
                List<String> tagsList = new ArrayList<>();

                // Extract quote text
                Element textElement = quoteBlock.selectFirst("span.text");
                if (textElement != null) {
                    text = textElement.text().trim();
                }

                // Extract author
                Element authorElement = quoteBlock.selectFirst("small.author");
                if (authorElement != null) {
                    author = authorElement.text().trim();
                }

                // Extract tags
                Elements tagElements = quoteBlock.select("a.tag");
                for (Element tagElement : tagElements) {
                    tagsList.add(tagElement.text().trim());
                }

                // Join tags with comma
                String tags = String.join(", ", tagsList);

                quotes.add(new Quote(text, author, tags));
            }
        } catch (Exception e) {
            System.err.println("[ERROR] Error parsing quotes: " + e.getMessage());
        }

        return quotes;
    }

    /**
     * Extract next page URL from HTML
     * Equivalent to C++ get_next_page function
     */
    private String getNextPage(String html) {
        try {
            Document doc = Jsoup.parse(html);
            Element nextLi = doc.selectFirst("li.next");

            if (nextLi != null) {
                Element nextLink = nextLi.selectFirst("a");
                if (nextLink != null && nextLink.hasAttr("href")) {
                    return BASE_URL + nextLink.attr("href");
                }
            }
        } catch (Exception e) {
            System.err.println("[ERROR] Error finding next page: " + e.getMessage());
        }

        return "";
    }

    /**
     * Save quotes to CSV file
     * Equivalent to C++ save_to_csv function
     */
    private void saveToCsv(List<Quote> quotes, String filename) {
        try (FileWriter fileWriter = new FileWriter(filename, java.nio.charset.StandardCharsets.UTF_8);
             CSVWriter writer = new CSVWriter(fileWriter)) {

            // Write header
            String[] header = {"text", "author", "tags"};
            writer.writeNext(header);

            // Write data
            for (Quote quote : quotes) {
                writer.writeNext(quote.toArray());
            }

            System.out.println("[INFO] Successfully saved " + quotes.size() + " quotes to " + filename);
        } catch (IOException e) {
            System.err.println("[ERROR] Error writing to CSV file: " + e.getMessage());
        }
    }

    /**
     * Main scraping logic
     * Equivalent to C++ main function logic
     */
    public void scrape() {
        String url = START_URL;
        List<Quote> allQuotes = new ArrayList<>();

        while (!url.isEmpty()) {
            System.out.println("[INFO] Fetching: " + url);

            String html = getPage(url);
            if (html.isEmpty()) {
                break;
            }

            List<Quote> quotes = parseQuotes(html);
            allQuotes.addAll(quotes);

            url = getNextPage(html);

            // Random delay between 1000-3000 milliseconds (matching C++ logic)
            if (!url.isEmpty()) {
                try {
                    int delay = ThreadLocalRandom.current().nextInt(1000, 3001);
                    Thread.sleep(delay);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    System.err.println("[ERROR] Sleep interrupted: " + e.getMessage());
                    break;
                }
            }
        }

        // Save results
        if (!allQuotes.isEmpty()) {
            saveToCsv(allQuotes, OUTPUT_CSV);
            System.out.println("[INFO] Saved " + allQuotes.size() + " quotes to file: " + OUTPUT_CSV);
        } else {
            System.err.println("[WARNING] No quotes found.");
        }
    }

    /**
     * Main method - entry point
     */
    public static void main(String[] args) {
        QuotesScraper scraper = new QuotesScraper();
        scraper.scrape();
    }
}