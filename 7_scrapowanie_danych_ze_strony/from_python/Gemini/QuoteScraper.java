// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/QuoteScraper.java) ---
package org.example;

import org.jsoup.Jsoup;
import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;
import org.jsoup.select.Elements;

import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.TimeUnit;
import java.util.logging.Logger;
import java.util.logging.Level;
import java.util.logging.ConsoleHandler;
import java.util.logging.Formatter;
import java.util.logging.LogRecord;
import java.util.stream.Collectors;

public class QuoteScraper {

    private static final String BASE_URL = "https://quotes.toscrape.com";
    private static final String START_URL = BASE_URL + "/page/1/";
    private static final String OUTPUT_CSV = "quotes.csv";
    private static final Logger LOGGER = Logger.getLogger(QuoteScraper.class.getName());

    static {
        // Configure logging to match Python's simple format
        LOGGER.setUseParentHandlers(false);
        ConsoleHandler handler = new ConsoleHandler();
        handler.setFormatter(new Formatter() {
            @Override
            public String format(LogRecord record) {
                return String.format("[%s] %s%n", record.getLevel(), record.getMessage());
            }
        });
        LOGGER.addHandler(handler);
    }

    public static void main(String[] args) {
        String url = START_URL;
        List<Quote> allQuotes = new ArrayList<>();
        Random random = new Random();

        while (url != null) {
            LOGGER.log(Level.INFO, "Fetching: " + url);
            try {
                Document doc = Jsoup.connect(url).timeout(10000).get();
                List<Quote> quotes = parseQuotes(doc);
                allQuotes.addAll(quotes);
                url = getNextPage(doc);

                // Random delay of 1-3 seconds
                long delay = (long) (1000 + random.nextFloat() * 2000);
                TimeUnit.MILLISECONDS.sleep(delay);
            } catch (IOException e) {
                LOGGER.log(Level.SEVERE, "Error fetching page " + url, e);
                break;
            } catch (InterruptedException e) {
                LOGGER.log(Level.SEVERE, "Thread interrupted.", e);
                Thread.currentThread().interrupt();
                break;
            }
        }

        if (!allQuotes.isEmpty()) {
            saveToCsv(allQuotes, OUTPUT_CSV);
            LOGGER.log(Level.INFO, "Saved " + allQuotes.size() + " quotes to file: " + OUTPUT_CSV);
        } else {
            LOGGER.log(Level.WARNING, "No quotes found.");
        }
    }

    private static List<Quote> parseQuotes(Document doc) {
        List<Quote> quotesData = new ArrayList<>();
        Elements quoteBlocks = doc.select("div.quote");

        for (Element quote : quoteBlocks) {
            String text = quote.select("span.text").first().text();
            String author = quote.select("small.author").first().text();
            String tags = quote.select("div.tags a.tag")
                    .stream()
                    .map(Element::text)
                    .collect(Collectors.joining(", "));

            quotesData.add(new Quote(text, author, tags));
        }
        return quotesData;
    }

    private static String getNextPage(Document doc) {
        Element nextBtn = doc.selectFirst("li.next a");
        if (nextBtn != null) {
            return BASE_URL + nextBtn.attr("href");
        }
        return null;
    }

    private static void saveToCsv(List<Quote> quotes, String filename) {
        try (FileWriter writer = new FileWriter(filename)) {
            // Write header
            writer.append("text,author,tags\n");
            // Write data
            for (Quote quote : quotes) {
                writer.append("\"")
                        .append(quote.getText().replace("\"", "\"\""))
                        .append("\",\"")
                        .append(quote.getAuthor().replace("\"", "\"\""))
                        .append("\",\"")
                        .append(quote.getTags().replace("\"", "\"\""))
                        .append("\"\n");
            }
        } catch (IOException e) {
            LOGGER.log(Level.SEVERE, "Error saving to CSV file.", e);
        }
    }

    private static class Quote {
        private final String text;
        private final String author;
        private final String tags;

        public Quote(String text, String author, String tags) {
            this.text = text;
            this.author = author;
            this.tags = tags;
        }

        public String getText() {
            return text;
        }

        public String getAuthor() {
            return author;
        }

        public String getTags() {
            return tags;
        }
    }
}