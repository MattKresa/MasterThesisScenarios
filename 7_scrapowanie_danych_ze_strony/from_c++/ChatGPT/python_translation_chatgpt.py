import requests
from bs4 import BeautifulSoup
import csv
import time
import random

BASE_URL = "https://quotes.toscrape.com"
START_URL = f"{BASE_URL}/page/1/"
OUTPUT_CSV = "quotes.csv"

def get_page(url):
    try:
        response = requests.get(url, timeout=10)
        response.raise_for_status()
        return response.text
    except requests.RequestException as e:
        print(f"[ERROR] Failed to fetch: {url} - {e}")
        return ""

def parse_quotes(soup):
    quotes = []
    for quote_div in soup.find_all("div", class_="quote"):
        text = quote_div.find("span", class_="text").get_text(strip=True)
        author = quote_div.find("small", class_="author").get_text(strip=True)
        tags = [tag.get_text(strip=True) for tag in quote_div.find_all("a", class_="tag")]
        quotes.append((text, author, ", ".join(tags)))
    return quotes

def get_next_page(soup):
    next_btn = soup.find("li", class_="next")
    if next_btn:
        a_tag = next_btn.find("a")
        if a_tag and "href" in a_tag.attrs:
            return BASE_URL + a_tag["href"]
    return ""

def save_to_csv(quotes, filename):
    with open(filename, mode="w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["text", "author", "tags"])
        writer.writerows(quotes)

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
        time.sleep(random.uniform(1.0, 3.0))  # random delay

    if all_quotes:
        save_to_csv(all_quotes, OUTPUT_CSV)
        print(f"[INFO] Saved {len(all_quotes)} quotes to file: {OUTPUT_CSV}")
    else:
        print("[WARNING] No quotes found.")

if __name__ == "__main__":
    main()
