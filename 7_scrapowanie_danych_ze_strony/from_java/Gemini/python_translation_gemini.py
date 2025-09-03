import requests
from bs4 import BeautifulSoup
import csv
import time
import random
import logging

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

BASE_URL = "https://quotes.toscrape.com"
START_URL = f"{BASE_URL}/page/1/"
OUTPUT_CSV = "quotes.csv"


def get_page(url):
    """Fetches a web page and returns its BeautifulSoup object."""
    try:
        response = requests.get(url, timeout=10)
        response.raise_for_status()  # Raises an HTTPError for bad responses
        return BeautifulSoup(response.text, 'html.parser')
    except requests.exceptions.RequestException as e:
        logger.error(f"Error fetching page {url}: {e}")
        return None


def parse_quotes(soup):
    """Parses a BeautifulSoup object for quotes and returns a list of dictionaries."""
    quotes = []
    quote_blocks = soup.find_all('div', class_='quote')

    for quote_block in quote_blocks:
        text = quote_block.find('span', class_='text').get_text(strip=True)
        author = quote_block.find('small', class_='author').get_text(strip=True)
        tags = [tag.get_text(strip=True) for tag in quote_block.find_all('a', class_='tag')]

        quotes.append({
            'text': text,
            'author': author,
            'tags': ', '.join(tags)
        })
    return quotes


def get_next_page(soup):
    """Finds the URL for the next page of quotes."""
    next_button = soup.find('li', class_='next')
    if next_button:
        return BASE_URL + next_button.find('a')['href']
    return None


def save_to_csv(quotes, filename):
    """Saves a list of quote dictionaries to a CSV file."""
    if not quotes:
        logger.warning("No quotes to save.")
        return

    try:
        with open(filename, 'w', newline='', encoding='utf-8') as csvfile:
            fieldnames = ['text', 'author', 'tags']
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

            writer.writeheader()
            writer.writerows(quotes)
        logger.info(f"Saved {len(quotes)} quotes to file: {filename}")
    except IOError as e:
        logger.error(f"Error writing to CSV file: {e}")


def main():
    """Main function to run the web scraper."""
    all_quotes = []
    url = START_URL

    while url:
        logger.info(f"Fetching: {url}")
        soup = get_page(url)
        if not soup:
            break

        all_quotes.extend(parse_quotes(soup))
        url = get_next_page(soup)

        # Pause to avoid overwhelming the server
        sleep_time = random.uniform(1, 3)
        logger.info(f"Sleeping for {sleep_time:.2f} seconds...")
        time.sleep(sleep_time)

    save_to_csv(all_quotes, OUTPUT_CSV)


if __name__ == "__main__":
    main()