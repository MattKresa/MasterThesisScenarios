#!/usr/bin/env python3
"""
Quote Scraper - Python version
Scrapes quotes from quotes.toscrape.com and saves them to a CSV file.
"""

import requests
from bs4 import BeautifulSoup
import csv
import time
import random
import logging
from typing import List, Dict, Optional
from urllib.parse import urljoin

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


class QuoteScraper:
    BASE_URL = "https://quotes.toscrape.com"
    START_URL = f"{BASE_URL}/page/1/"
    OUTPUT_CSV = "quotes.csv"

    def __init__(self):
        self.session = requests.Session()
        self.session.headers.update({
            'User-Agent': 'Mozilla/5.0 (compatible; QuoteScraper/1.0)'
        })

    def get_page(self, url: str) -> Optional[BeautifulSoup]:
        """
        Fetch a page and return a BeautifulSoup object.

        Args:
            url: The URL to fetch

        Returns:
            BeautifulSoup object or None if error occurred
        """
        try:
            response = self.session.get(url, timeout=10)
            response.raise_for_status()
            return BeautifulSoup(response.content, 'html.parser')
        except requests.RequestException as e:
            logger.error(f"Error fetching page {url}: {e}")
            return None

    def parse_quotes(self, soup: BeautifulSoup) -> List[Dict[str, str]]:
        """
        Parse quotes from a BeautifulSoup object.

        Args:
            soup: BeautifulSoup object containing the HTML

        Returns:
            List of quote dictionaries
        """
        quotes = []
        quote_blocks = soup.select("div.quote")

        for quote_block in quote_blocks:
            # Extract quote text
            text_element = quote_block.select_one("span.text")
            text = text_element.get_text(strip=True) if text_element else ""

            # Remove surrounding quotes if present
            if text.startswith('"') and text.endswith('"'):
                text = text[1:-1]

            # Extract author
            author_element = quote_block.select_one("small.author")
            author = author_element.get_text(strip=True) if author_element else ""

            # Extract tags
            tag_elements = quote_block.select("a.tag")
            tags = [tag.get_text(strip=True) for tag in tag_elements]

            quote_data = {
                "text": text,
                "author": author,
                "tags": ", ".join(tags)
            }

            quotes.append(quote_data)

        return quotes

    def get_next_page_url(self, soup: BeautifulSoup) -> Optional[str]:
        """
        Extract the next page URL from the current page.

        Args:
            soup: BeautifulSoup object containing the HTML

        Returns:
            Next page URL or None if no next page
        """
        next_button = soup.select_one("li.next > a")
        if next_button:
            href = next_button.get("href")
            if href:
                return urljoin(self.BASE_URL, href)
        return None

    def escape_csv_value(self, value: str) -> str:
        """
        Escape a value for CSV format.

        Args:
            value: The value to escape

        Returns:
            Escaped value
        """
        if not value:
            return ""

        # Check if escaping is needed
        if '"' in value or ',' in value or '\n' in value:
            # Replace " with ""
            escaped = value.replace('"', '""')
            # Wrap in quotes
            return f'"{escaped}"'

        return value

    def save_to_csv(self, quotes: List[Dict[str, str]], filename: str) -> None:
        """
        Save quotes to a CSV file.

        Args:
            quotes: List of quote dictionaries
            filename: Output filename
        """
        try:
            with open(filename, 'w', newline='', encoding='utf-8') as csvfile:
                fieldnames = ['text', 'author', 'tags']
                writer = csv.DictWriter(csvfile, fieldnames=fieldnames, quoting=csv.QUOTE_MINIMAL)

                # Write header
                writer.writeheader()

                # Write quotes
                for quote in quotes:
                    writer.writerow(quote)

            logger.info(f"Successfully saved {len(quotes)} quotes to {filename}")

        except IOError as e:
            logger.error(f"Error writing to CSV file: {e}")

    def run(self) -> None:
        """
        Main method to run the scraper.
        """
        all_quotes = []
        url = self.START_URL

        while url:
            logger.info(f"Fetching: {url}")

            soup = self.get_page(url)
            if soup is None:
                logger.error("Failed to fetch page, stopping.")
                break

            # Parse quotes from current page
            page_quotes = self.parse_quotes(soup)
            all_quotes.extend(page_quotes)
            logger.info(f"Found {len(page_quotes)} quotes on this page")

            # Get next page URL
            url = self.get_next_page_url(soup)

            # Sleep between requests (1-3 seconds)
            if url:
                sleep_time = random.uniform(1, 3)
                logger.info(f"Sleeping for {sleep_time:.1f} seconds...")
                time.sleep(sleep_time)

        if all_quotes:
            self.save_to_csv(all_quotes, self.OUTPUT_CSV)
            logger.info(f"Saved {len(all_quotes)} quotes to file: {self.OUTPUT_CSV}")
        else:
            logger.warning("No quotes found.")


def main():
    """Main entry point."""
    scraper = QuoteScraper()
    try:
        scraper.run()
    except KeyboardInterrupt:
        logger.info("Scraping interrupted by user")
    except Exception as e:
        logger.error(f"Unexpected error occurred: {e}")


if __name__ == "__main__":
    main()