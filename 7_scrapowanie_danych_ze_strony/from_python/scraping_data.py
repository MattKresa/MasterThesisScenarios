import requests
from bs4 import BeautifulSoup
import csv
import time
import logging
from random import uniform

# Configure logging
logging.basicConfig(level=logging.INFO, format='[%(levelname)s] %(message)s')

BASE_URL = "https://quotes.toscrape.com"
START_URL = f"{BASE_URL}/page/1/"
OUTPUT_CSV = "quotes.csv"

def get_page(url):
    try:
        response = requests.get(url, timeout=10)
        response.raise_for_status()
        return BeautifulSoup(response.text, 'html.parser')
    except requests.RequestException as e:
        logging.error(f"Error fetching page {url}: {e}")
        return None

def parse_quotes(soup):
    quotes_data = []
    quote_blocks = soup.find_all('div', class_='quote')

    for quote in quote_blocks:
        text = quote.find('span', class_='text').get_text(strip=True)
        author = quote.find('small', class_='author').get_text(strip=True)
        tags = [tag.get_text(strip=True) for tag in quote.find_all('a', class_='tag')]

        quotes_data.append({
            "text": text,
            "author": author,
            "tags": ", ".join(tags)
        })
    return quotes_data

def get_next_page(soup):
    next_btn = soup.find('li', class_='next')
    if next_btn:
        next_link = next_btn.find('a')['href']
        return BASE_URL + next_link
    return None

def save_to_csv(quotes, filename):
    keys = quotes[0].keys()
    with open(filename, mode='w', newline='', encoding='utf-8') as file:
        writer = csv.DictWriter(file, fieldnames=keys)
        writer.writeheader()
        writer.writerows(quotes)

def main():
    url = START_URL
    all_quotes = []

    while url:
        logging.info(f"Fetching: {url}")
        soup = get_page(url)
        if soup is None:
            break

        quotes = parse_quotes(soup)
        all_quotes.extend(quotes)

        url = get_next_page(soup)
        time.sleep(uniform(1, 3))  # Random delay of 1–3 seconds

    if all_quotes:
        save_to_csv(all_quotes, OUTPUT_CSV)
        logging.info(f"Saved {len(all_quotes)} quotes to file: {OUTPUT_CSV}")
    else:
        logging.warning("No quotes found.")

if __name__ == "__main__":
    main()
