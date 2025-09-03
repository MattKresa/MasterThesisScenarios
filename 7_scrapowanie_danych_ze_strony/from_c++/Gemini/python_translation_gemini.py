import requests
from bs4 import BeautifulSoup
import csv
import time
import random

BASE_URL = "https://quotes.toscrape.com"
START_URL = BASE_URL + "/page/1/"
OUTPUT_CSV = "quotes.csv"


def get_page(url):
    """
    Fetches the content of a given URL.
    """
    try:
        response = requests.get(url, timeout=10)
        response.raise_for_status()  # Raise an exception for bad status codes (4xx or 5xx)
        return response.text
    except requests.exceptions.RequestException as e:
        print(f"[ERROR] Failed to fetch: {url} - {e}")
        return None


def parse_quotes(html):
    """
    Parses HTML content to find and extract quotes, authors, and tags.
    """
    soup = BeautifulSoup(html, 'html.parser')
    quotes = []

    quote_blocks = soup.find_all('div', class_='quote')

    for block in quote_blocks:
        text = block.find('span', class_='text').get_text(strip=True)
        author = block.find('small', class_='author').get_text(strip=True)

        tags = [tag.get_text(strip=True) for tag in block.find('div', class_='tags').find_all('a', class_='tag')]
        tags_str = ', '.join(tags)

        quotes.append((text, author, tags_str))

    return quotes


def get_next_page(html):
    """
    Finds the URL for the next page.
    """
    soup = BeautifulSoup(html, 'html.parser')
    next_link = soup.find('li', class_='next')

    if next_link:
        href = next_link.find('a')['href']
        return BASE_URL + href

    return None


def save_to_csv(quotes, filename):
    """
    Saves the list of quotes to a CSV file.
    """
    with open(filename, 'w', newline='', encoding='utf-8') as file:
        writer = csv.writer(file, quoting=csv.QUOTE_MINIMAL)
        writer.writerow(["text", "author", "tags"])
        writer.writerows(quotes)


def main():
    """
    Main function to orchestrate the scraping process.
    """
    url = START_URL
    all_quotes = []

    while url:
        print(f"[INFO] Fetching: {url}")

        html = get_page(url)
        if not html:
            break

        quotes = parse_quotes(html)
        all_quotes.extend(quotes)

        url = get_next_page(html)

        # Random delay to be polite to the server
        delay = random.uniform(1, 3)
        time.sleep(delay)

    if all_quotes:
        save_to_csv(all_quotes, OUTPUT_CSV)
        print(f"[INFO] Saved {len(all_quotes)} quotes to file: {OUTPUT_CSV}")
    else:
        print("[WARNING] No quotes found.")


if __name__ == "__main__":
    main()