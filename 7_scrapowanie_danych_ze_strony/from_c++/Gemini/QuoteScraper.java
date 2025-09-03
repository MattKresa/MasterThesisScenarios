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

public class QuoteScraper {

    private static final String BASE_URL = "https://quotes.toscrape.com";
    private static final String START_URL = BASE_URL + "/page/1/";
    private static final String OUTPUT_CSV = "quotes.csv";

    public static void main(String[] args) {
        String url = START_URL;
        List<String[]> allQuotes = new ArrayList<>();
        Random random = new Random();

        while (url != null && !url.isEmpty()) {
            System.out.println("[INFO] Fetching: " + url);
            try {
                // Use Jsoup to fetch and parse the page
                Document doc = Jsoup.connect(url)
                        .timeout(10000)
                        .get();

                // Parse the quotes from the current page
                List<String[]> quotes = parseQuotes(doc);
                allQuotes.addAll(quotes);

                // Find the next page link
                url = getNextPage(doc);

                // Introduce a random delay to be polite
                long delay = 1000 + random.nextInt(2001); // 1000 to 3000 ms
                Thread.sleep(delay);

            } catch (IOException e) {
                System.err.println("[ERROR] Failed to fetch or parse: " + url + " - " + e.getMessage());
                url = null; // Stop the loop on error
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                url = null; // Stop the loop if interrupted
            }
        }

        if (!allQuotes.isEmpty()) {
            saveToCsv(allQuotes, OUTPUT_CSV);
            System.out.println("[INFO] Saved " + allQuotes.size() + " quotes to file: " + OUTPUT_CSV);
        } else {
            System.err.println("[WARNING] No quotes found.");
        }
    }

    private static List<String[]> parseQuotes(Document doc) {
        List<String[]> quotes = new ArrayList<>();
        Elements quoteElements = doc.select(".quote");

        for (Element quoteElement : quoteElements) {
            String text = quoteElement.select(".text").text();
            String author = quoteElement.select(".author").text();

            Elements tagElements = quoteElement.select(".tag");
            List<String> tags = new ArrayList<>();
            for (Element tagElement : tagElements) {
                tags.add(tagElement.text());
            }
            String tagsString = String.join(", ", tags);

            quotes.add(new String[]{text, author, tagsString});
        }
        return quotes;
    }

    private static String getNextPage(Document doc) {
        Element nextButton = doc.selectFirst(".next a");
        if (nextButton != null) {
            return BASE_URL + nextButton.attr("href");
        }
        return null;
    }

    private static void saveToCsv(List<String[]> quotes, String filename) {
        try (FileWriter writer = new FileWriter(filename)) {
            // Write header
            writer.append("text,author,tags\n");

            // Write data
            for (String[] quote : quotes) {
                writer.append("\"" + escapeCsv(quote[0]) + "\"");
                writer.append(",");
                writer.append("\"" + escapeCsv(quote[1]) + "\"");
                writer.append(",");
                writer.append("\"" + escapeCsv(quote[2]) + "\"");
                writer.append("\n");
            }
        } catch (IOException e) {
            System.err.println("[ERROR] Failed to save to CSV: " + e.getMessage());
        }
    }

    private static String escapeCsv(String value) {
        // Escape double quotes by doubling them
        return value.replace("\"", "\"\"");
    }
}