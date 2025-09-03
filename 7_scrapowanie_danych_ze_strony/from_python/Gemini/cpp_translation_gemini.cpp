#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <random>
#include <chrono>
#include <thread>
#include <curl/curl.h>
#include <gumbo.h>

// A struct to hold quote data
struct Quote {
    std::string text;
    std::string author;
    std::string tags;
};

// Callback function for cURL to write received data to a string
size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
    }
    catch (std::bad_alloc& e) {
        // Handle memory problem
        return 0;
    }
    return newLength;
}

// Function to fetch a page using cURL
std::string getPage(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // Set timeout to 10 seconds
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Error fetching page " << url << ": " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

// Helper function to find a specific element with a class
GumboNode* findElementByClass(GumboNode* node, const std::string& class_name) {
    if (!node || node->type != GUMBO_NODE_ELEMENT) {
        return nullptr;
    }

    if (node->v.element.tag == GUMBO_TAG_UNKNOWN) {
        GumboAttribute* class_attr = gumbo_get_attribute(&node->v.element.attributes, "class");
        if (class_attr && class_attr->value == class_name) {
            return node;
        }
    }

    if (node->v.element.tag == GUMBO_TAG_DIV || node->v.element.tag == GUMBO_TAG_SPAN || node->v.element.tag == GUMBO_TAG_SMALL || node->v.element.tag == GUMBO_TAG_A) {
        GumboAttribute* class_attr = gumbo_get_attribute(&node->v.element.attributes, "class");
        if (class_attr && class_attr->value == class_name) {
            return node;
        }
    }

    const GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        GumboNode* result = findElementByClass(static_cast<GumboNode*>(children->data[i]), class_name);
        if (result) {
            return result;
        }
    }
    return nullptr;
}

// Helper function to find all elements with a specific class
void findAllElementsByClass(GumboNode* node, const std::string& class_name, std::vector<GumboNode*>& results) {
    if (!node || node->type != GUMBO_NODE_ELEMENT) {
        return;
    }

    if (node->v.element.tag == GUMBO_TAG_DIV || node->v.element.tag == GUMBO_TAG_SPAN || node->v.element.tag == GUMBO_TAG_SMALL || node->v.element.tag == GUMBO_TAG_A) {
        GumboAttribute* class_attr = gumbo_get_attribute(&node->v.element.attributes, "class");
        if (class_attr && class_attr->value == class_name) {
            results.push_back(node);
        }
    }

    const GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        findAllElementsByClass(static_cast<GumboNode*>(children->data[i]), class_name, results);
    }
}

// Function to get the text content of a Gumbo node
std::string getTextContent(GumboNode* node) {
    if (!node || node->type != GUMBO_NODE_ELEMENT) {
        return "";
    }
    std::string text = "";
    if (node->v.element.tag == GUMBO_TAG_TEXT) {
        return std::string(node->v.text.text);
    }
    const GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        GumboNode* child = static_cast<GumboNode*>(children->data[i]);
        if (child->type == GUMBO_NODE_TEXT) {
            text += child->v.text.text;
        }
        else if (child->type == GUMBO_NODE_ELEMENT) {
            text += getTextContent(child);
        }
    }
    return text;
}

// Function to parse quotes from an HTML document
std::vector<Quote> parseQuotes(GumboNode* root) {
    std::vector<Quote> quotes;
    std::vector<GumboNode*> quote_blocks;
    findAllElementsByClass(root, "quote", quote_blocks);

    for (GumboNode* quote_block : quote_blocks) {
        GumboNode* text_node = findElementByClass(quote_block, "text");
        GumboNode* author_node = findElementByClass(quote_block, "author");

        Quote quote;
        if (text_node) {
            quote.text = getTextContent(text_node);
        }
        if (author_node) {
            quote.author = getTextContent(author_node);
        }

        std::vector<GumboNode*> tag_nodes;
        findAllElementsByClass(quote_block, "tag", tag_nodes);
        for (size_t i = 0; i < tag_nodes.size(); ++i) {
            quote.tags += getTextContent(tag_nodes[i]);
            if (i < tag_nodes.size() - 1) {
                quote.tags += ", ";
            }
        }
        quotes.push_back(quote);
    }
    return quotes;
}

// Function to get the URL of the next page
std::string getNextPage(GumboNode* root) {
    GumboNode* next_li = findElementByClass(root, "next");
    if (next_li) {
        GumboNode* next_a = findElementByClass(next_li, "");
        if (next_a && next_a->v.element.tag == GUMBO_TAG_A) {
            GumboAttribute* href = gumbo_get_attribute(&next_a->v.element.attributes, "href");
            if (href) {
                return "https://quotes.toscrape.com" + std::string(href->value);
            }
        }
    }
    return "";
}

// Function to save quotes to a CSV file
void saveToCsv(const std::vector<Quote>& quotes, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    // Write header
    file << "text,author,tags\n";

    // Write data
    for (const auto& quote : quotes) {
        file << "\"" << quote.text << "\",\"" << quote.author << "\",\"" << quote.tags << "\"\n";
    }

    file.close();
}

int main() {
    std::string url = "https://quotes.toscrape.com/page/1/";
    std::vector<Quote> all_quotes;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distrib(1.0, 3.0);

    while (!url.empty()) {
        std::cout << "Fetching: " << url << std::endl;
        std::string html_content = getPage(url);
        if (html_content.empty()) {
            break;
        }

        GumboOutput* output = gumbo_parse(html_content.c_str());
        std::vector<Quote> quotes = parseQuotes(output->root);
        all_quotes.insert(all_quotes.end(), quotes.begin(), quotes.end());

        url = getNextPage(output->root);
        gumbo_destroy_output(&kGumboDefaultOptions, output);

        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(distrib(gen) * 1000)));
    }

    if (!all_quotes.empty()) {
        saveToCsv(all_quotes, "quotes.csv");
        std::cout << "Saved " << all_quotes.size() << " quotes to file: quotes.csv" << std::endl;
    }
    else {
        std::cerr << "No quotes found." << std::endl;
    }

    return 0;
}