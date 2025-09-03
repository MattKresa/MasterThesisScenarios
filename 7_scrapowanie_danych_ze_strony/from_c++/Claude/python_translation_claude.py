import requests
from bs4 import BeautifulSoup
import csv
import time
import random
import logging

# Configure logging
logging.basicConfig(level=logging.INFO, format='[%(levelname)s] %(message)s')

BASE_URL = "https://quotes.toscrape.com"
START_URL = f"{BASE_URL}/page/1/"
OUTPUT_CSV = "quotes.csv"

def get_page(url):
    """Fetch page content using requests"""
    try:
        response = requests.get(url, timeout=10)
        response.raise_for_status()
        return response.text
    except requests.RequestException as e:
        logging.error(f"Failed to fetch: {url} - {e}")
        return None

def parse_quotes(html):
    """Parse quotes from HTML content"""
    soup = BeautifulSoup(html, 'html.parser')
    quotes = []
    
    # Find all quote blocks
    quote_blocks = soup.find_all('div', class_='quote')
    
    for quote_block in quote_blocks:
        text = ""
        author = ""
        tags = []
        
        # Extract quote text
        text_element = quote_block.find('span', class_='text')
        if text_element:
            text = text_element.get_text().strip()
        
        # Extract author
        author_element = quote_block.find('small', class_='author')
        if author_element:
            author = author_element.get_text().strip()
        
        # Extract tags
        tag_elements = quote_block.find_all('a', class_='tag')
        for tag_element in tag_elements:
            tags.append(tag_element.get_text().strip())
        
        # Join tags with comma
        tags_str = ", ".join(tags)
        
        quotes.append((text, author, tags_str))
    
    return quotes

def get_next_page(html):
    """Extract next page URL from HTML"""
    soup = BeautifulSoup(html, 'html.parser')
    
    # Find the next button
    next_li = soup.find('li', class_='next')
    if next_li:
        next_link = next_li.find('a')
        if next_link and next_link.get('href'):
            return BASE_URL + next_link['href']
    
    return None

def save_to_csv(quotes, filename):
    """Save quotes to CSV file"""
    try:
        with open(filename, 'w', newline='', encoding='utf-8') as file:
            writer = csv.writer(file)
            # Write header
            writer.writerow(['text', 'author', 'tags'])
            # Write data
            for text, author, tags in quotes:
                writer.writerow([text, author, tags])
        logging.info(f"Successfully saved {len(quotes)} quotes to {filename}")
    except IOError as e:
        logging.error(f"Error writing to CSV file: {e}")

def main():
    """Main scraping logic"""
    url = START_URL
    all_quotes = []
    
    while url:
        logging.info(f"Fetching: {url}")
        
        # Fetch page content
        html = get_page(url)
        if not html:
            break
        
        # Parse quotes from current page
        quotes = parse_quotes(html)
        all_quotes.extend(quotes)
        
        # Get next page URL
        url = get_next_page(html)
        
        # Random delay between 1-3 seconds (matching C++ milliseconds range)
        if url:  # Only delay if there's another page to fetch
            delay = random.uniform(1.0, 3.0)
            time.sleep(delay)
    
    # Save results
    if all_quotes:
        save_to_csv(all_quotes, OUTPUT_CSV)
        logging.info(f"Saved {len(all_quotes)} quotes to file: {OUTPUT_CSV}")
    else:
        logging.warning("No quotes found.")

if __name__ == "__main__":
    main()