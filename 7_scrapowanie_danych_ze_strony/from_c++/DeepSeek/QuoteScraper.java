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

public class QuoteScraper {
    private static final String BASE_URL = "https://quotes.toscrape.com";
    private static final String START_URL = BASE_URL + "/page/1/";
    private static final String OUTPUT_CSV = "quotes.csv";

    private static String getPage(String url) throws IOException {
        return Jsoup.connect(url)
                .timeout(10000)
                .get()
                .html();
    }

    private static List<Quote> parseQuotes(Document doc) {
        List<Quote> quotes = new ArrayList<>();
        Elements quoteBlocks = doc.select("div.quote");

        for (Element block : quoteBlocks) {
            String text = block.select("span.text").text();
            String author = block.select("small.author").text();
            Elements tagElements = block.select("a.tag");
            List<String> tags = new ArrayList<>();
            for (Element tag : tagElements) {
                tags.add(tag.text());
            }
            quotes.add(new Quote(text, author, String.join(", ", tags)));
        }

        return quotes;
    }

    private static String getNextPage(Document doc) {
        Element nextButton = doc.selectFirst("li.next > a");
        if (nextButton != null) {
            String href = nextButton.attr("href");
            return BASE_URL + href;
        }
        return null;
    }

    private static void saveToCsv(List<Quote> quotes, String filename) throws IOException {
        try (FileWriter writer = new FileWriter(filename)) {
            writer.write("text,author,tags\n");
            for (Quote quote : quotes) {
                writer.write(String.format("\"%s\",\"%s\",\"%s\"\n",
                        quote.getText().replace("\"", "\"\""),
                        quote.getAuthor().replace("\"", "\"\""),
                        quote.getTags().replace("\"", "\"\"")));
            }
        }
    }

    public static void main(String[] args) {
        String url = START_URL;
        List<Quote> allQuotes = new ArrayList<>();
        Random random = new Random();

        try {
            while (url != null) {
                System.out.println("[INFO] Fetching: " + url);

                Document doc = Jsoup.connect(url).timeout(10000).get();
                List<Quote> quotes = parseQuotes(doc);
                allQuotes.addAll(quotes);

                url = getNextPage(doc);

                // Random delay between 1 and 3 seconds
                TimeUnit.MILLISECONDS.sleep(1000 + random.nextInt(2000));
            }

            if (!allQuotes.isEmpty()) {
                saveToCsv(allQuotes, OUTPUT_CSV);
                System.out.printf("[INFO] Saved %d quotes to file: %s%n", allQuotes.size(), OUTPUT_CSV);
            } else {
                System.out.println("[WARNING] No quotes found.");
            }
        } catch (IOException | InterruptedException e) {
            System.err.println("[ERROR] " + e.getMessage());
        }
    }
}

class Quote {
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