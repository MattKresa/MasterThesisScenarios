import requests
from bs4 import BeautifulSoup
import csv
import time
import random
import logging


class QuoteScraper:
    BASE_URL = "https://quotes.toscrape.com"
    START_URL = BASE_URL + "/page/1/"
    OUTPUT_CSV = "quotes.csv"

    def __init__(self):
        logging.basicConfig(level=logging.INFO)
        self.logger = logging.getLogger(__name__)

    def run(self):
        all_quotes = []
        url = self.START_URL

        while url:
            self.logger.info(f"Fetching: {url}")
            doc = self.get_page(url)
            if not doc:
                break

            all_quotes.extend(self.parse_quotes(doc))
            url = self.get_next_page(doc)

            time.sleep(random.uniform(1, 3))  # sleep 1-3s

        if all_quotes:
            self.save_to_csv(all_quotes, self.OUTPUT_CSV)
            self.logger.info(f"Saved {len(all_quotes)} quotes to file: {self.OUTPUT_CSV}")
        else:
            self.logger.warning("No quotes found.")

    def get_page(self, url):
        try:
            response = requests.get(url, timeout=10)
            response.raise_for_status()
            return BeautifulSoup(response.text, 'html.parser')
        except requests.RequestException as e:
            self.logger.error(f"Error fetching page {url}: {e}")
            return None

    def parse_quotes(self, doc):
        quotes = []
        quote_blocks = doc.select("div.quote")

        for quote in quote_blocks:
            text = quote.select_one("span.text").get_text()
            author = quote.select_one("small.author").get_text()
            tag_elements = quote.select("a.tag")

            tags = [tag.get_text() for tag in tag_elements]

            quote_data = {
                "text": text,
                "author": author,
                "tags": ", ".join(tags)
            }

            quotes.append(quote_data)

        return quotes

    def get_next_page(self, doc):
        next_button = doc.select_one("li.next > a")
        if next_button:
            return self.BASE_URL + next_button["href"]
        return None

    def save_to_csv(self, quotes, filename):
        try:
            with open(filename, 'w', newline='', encoding='utf-8') as csvfile:
                fieldnames = ['text', 'author', 'tags']
                writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

                writer.writeheader()
                for quote in quotes:
                    writer.writerow({
                        'text': quote['text'],
                        'author': quote['author'],
                        'tags': quote['tags']
                    })
        except IOError as e:
            self.logger.error(f"Error writing to CSV file: {e}")

    @staticmethod
    def escape_csv(value):
        if '"' in value or ',' in value or '\n' in value:
            value = value.replace('"', '""')
            return f'"{value}"'
        return value


if __name__ == "__main__":
    scraper = QuoteScraper()
    scraper.run()