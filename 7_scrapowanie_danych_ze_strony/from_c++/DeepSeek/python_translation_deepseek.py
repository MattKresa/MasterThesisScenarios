import csv
import random
import time
import requests
from bs4 import BeautifulSoup

BASE_URL = "https://quotes.toscrape.com"
START_URL = BASE_URL + "/page/1/"
OUTPUT_CSV = "quotes.csv"


def get_page(url):
    try:
        response = requests.get(url, timeout=10)
        response.raise_for_status()
        return response.text
    except requests.exceptions.RequestException as e:
        print(f"[ERROR] Failed to fetch: {url} - {str(e)}")
        return ""


def parse_quotes(soup):
    quotes = []
    quote_blocks = soup.find_all("div", class_="quote")

    for block in quote_blocks:
        text = block.find("span", class_="text").get_text(strip=True)
        author = block.find("small", class_="author").get_text(strip=True)
        tags = [tag.get_text(strip=True) for tag in block.find_all("a", class_="tag")]
        quotes.append((text, author, ", ".join(tags)))

    return quotes


def get_next_page(soup):
    next_button = soup.find("li", class_="next")
    if next_button:
        next_link = next_button.find("a")
        if next_link and "href" in next_link.attrs:
            return BASE_URL + next_link["href"]
    return None


def save_to_csv(quotes, filename):
    with open(filename, "w", newline="", encoding="utf-8") as file:
        writer = csv.writer(file)
        writer.writerow(["text", "author", "tags"])
        for text, author, tags in quotes:
            writer.writerow([text, author, tags])


def main():
    url = START_URL
    all_quotes = []

    while url:
        print(f"[INFO] Fetching: {url}")

        html = get_page(url)
        if not html:
            break

        soup = BeautifulSoup(html, "html.parser")
        quotes = parse_quotes(soup)
        all_quotes.extend(quotes)

        url = get_next_page(soup)

        # Random delay between 1 and 3 seconds
        time.sleep(random.uniform(1, 3))

    if all_quotes:
        save_to_csv(all_quotes, OUTPUT_CSV)
        print(f"[INFO] Saved {len(all_quotes)} quotes to file: {OUTPUT_CSV}")
    else:
        print("[WARNING] No quotes found.")


if __name__ == "__main__":
    main()