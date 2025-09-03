import requests
from bs4 import BeautifulSoup
import csv
import time
import random
import logging

BASE_URL = "https://quotes.toscrape.com"
START_URL = f"{BASE_URL}/page/1/"
OUTPUT_CSV = "quotes.csv"

logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")
logger = logging.getLogger("QuoteScraper")


def main():
    all_quotes = []
    url = START_URL

    while url:
        logger.info(f"Fetching: {url}")
        doc = get_page(url)
        if doc is None:
            break

        all_quotes.extend(parse_quotes(doc))
        url = get_next_page(doc)

        time.sleep(random.uniform(1, 3))  # sleep 1–3 seconds

    if all_quotes:
        save_to_csv(all_quotes, OUTPUT_CSV)
        logger.info(f"Saved {len(all_quotes)} quotes to file: {OUTPUT_CSV}")
    else:
        logger.warning("No quotes found.")


def get_page(url):
    try:
        response = requests.get(url, timeout=10)
        response.raise_for_status()
        return BeautifulSoup(response.text, "html.parser")
    except requests.RequestException as e:
        logger.error(f"Error fetching page {url}: {e}")
        return None


def parse_quotes(doc):
    quotes = []
    quote_blocks = doc.select("div.quote")

    for quote in quote_blocks:
        text = quote.select_one("span.text").get_text(strip=True)
        author = quote.select_one("small.author").get_text(strip=True)
        tags = [tag.get_text(strip=True) for tag in quote.select("a.tag")]

        quotes.append({
            "text": text,
            "author": author,
            "tags": ", ".join(tags)
        })

    return quotes


def get_next_page(doc):
    next_button = doc.select_one("li.next > a")
    if next_button:
        return BASE_URL + next_button["href"]
    return None


def save_to_csv(quotes, filename):
    try:
        with open(filename, "w", newline="", encoding="utf-8") as csvfile:
            writer = csv.DictWriter(csvfile, fieldnames=["text", "author", "tags"])
            writer.writeheader()
            for quote in quotes:
                writer.writerow({
                    "text": escape_csv(quote["text"]),
                    "author": escape_csv(quote["author"]),
                    "tags": escape_csv(quote["tags"])
                })
    except IOError as e:
        logger.error(f"Error writing to CSV file: {e}")


def escape_csv(value):
    # CSV module already handles quoting, but keeping method for consistency
    return value.replace('"', '""')


if __name__ == "__main__":
    main()
