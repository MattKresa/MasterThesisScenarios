#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <random>
#include <sstream>
#include <curl/curl.h>
#include <gumbo.h>
#include <regex>

// Struct to hold quote data
struct Quote {
    std::string text;
    std::string author;
    std::string tags;
};

// Global constants
const std::string BASE_URL = "https://quotes.toscrape.com";
const std::string START_URL = BASE_URL + "/page/1/";
const std::string OUTPUT_CSV = "quotes.csv";

// Callback function for curl to write response data
struct MemoryStruct {
    char* memory;
    size_t size;
};

static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct* mem = (struct MemoryStruct*)userp;

    char* ptr = (char*)realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        std::cerr << "Not enough memory (realloc returned NULL)" << std::endl;
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// Logging function
void log_info(const std::string& message) {
    std::cout << "[INFO] " << message << std::endl;
}

void log_error(const std::string& message) {
    std::cerr << "[ERROR] " << message << std::endl;
}

void log_warning(const std::string& message) {
    std::cout << "[WARNING] " << message << std::endl;
}

// Get page content using curl
std::string get_page(const std::string& url) {
    CURL* curl_handle;
    CURLcode res;

    struct MemoryStruct chunk;
    chunk.memory = (char*)malloc(1);
    chunk.size = 0;

    curl_handle = curl_easy_init();
    if (!curl_handle) {
        log_error("Failed to initialize curl");
        free(chunk.memory);
        return "";
    }

    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

    res = curl_easy_perform(curl_handle);

    std::string result;
    if (res != CURLE_OK) {
        log_error("Error fetching page " + url + ": " + curl_easy_strerror(res));
    }
    else {
        result = std::string(chunk.memory);
    }

    curl_easy_cleanup(curl_handle);
    free(chunk.memory);

    return result;
}

// Helper function to get text content from a node
std::string get_text_content(GumboNode* node) {
    if (node->type == GUMBO_NODE_TEXT) {
        return std::string(node->v.text.text);
    }
    else if (node->type == GUMBO_NODE_ELEMENT) {
        std::string result;
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            result += get_text_content((GumboNode*)children->data[i]);
        }
        return result;
    }
    return "";
}

// Helper function to find element by class
GumboNode* find_element_by_class(GumboNode* node, const std::string& tag_name, const std::string& class_name) {
    if (node->type != GUMBO_NODE_ELEMENT) {
        return nullptr;
    }

    GumboElement* element = &node->v.element;

    if (element->tag == gumbo_tag_enum(tag_name.c_str())) {
        GumboAttribute* class_attr = gumbo_get_attribute(&element->attributes, "class");
        if (class_attr && std::string(class_attr->value).find(class_name) != std::string::npos) {
            return node;
        }
    }

    // Recursively search children
    for (unsigned int i = 0; i < element->children.length; ++i) {
        GumboNode* result = find_element_by_class((GumboNode*)element->children.data[i], tag_name, class_name);
        if (result) {
            return result;
        }
    }

    return nullptr;
}

// Helper function to find all elements by class
void find_all_elements_by_class(GumboNode* node, const std::string& tag_name, const std::string& class_name, std::vector<GumboNode*>& results) {
    if (node->type != GUMBO_NODE_ELEMENT) {
        return;
    }

    GumboElement* element = &node->v.element;

    if (element->tag == gumbo_tag_enum(tag_name.c_str())) {
        GumboAttribute* class_attr = gumbo_get_attribute(&element->attributes, "class");
        if (class_attr && std::string(class_attr->value).find(class_name) != std::string::npos) {
            results.push_back(node);
        }
    }

    // Recursively search children
    for (unsigned int i = 0; i < element->children.length; ++i) {
        find_all_elements_by_class((GumboNode*)element->children.data[i], tag_name, class_name, results);
    }
}

// Trim whitespace from string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first) {
        return str;
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

// Parse quotes from HTML
std::vector<Quote> parse_quotes(const std::string& html) {
    std::vector<Quote> quotes_data;

    GumboOutput* output = gumbo_parse(html.c_str());

    std::vector<GumboNode*> quote_blocks;
    find_all_elements_by_class(output->root, "div", "quote", quote_blocks);

    for (GumboNode* quote_node : quote_blocks) {
        Quote quote;

        // Find text
        GumboNode* text_node = find_element_by_class(quote_node, "span", "text");
        if (text_node) {
            quote.text = trim(get_text_content(text_node));
        }

        // Find author
        GumboNode* author_node = find_element_by_class(quote_node, "small", "author");
        if (author_node) {
            quote.author = trim(get_text_content(author_node));
        }

        // Find tags
        std::vector<GumboNode*> tag_nodes;
        find_all_elements_by_class(quote_node, "a", "tag", tag_nodes);
        std::vector<std::string> tags;
        for (GumboNode* tag_node : tag_nodes) {
            tags.push_back(trim(get_text_content(tag_node)));
        }

        // Join tags with comma
        std::ostringstream tag_stream;
        for (size_t i = 0; i < tags.size(); ++i) {
            if (i != 0) tag_stream << ", ";
            tag_stream << tags[i];
        }
        quote.tags = tag_stream.str();

        quotes_data.push_back(quote);
    }

    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return quotes_data;
}

// Get next page URL
std::string get_next_page(const std::string& html) {
    GumboOutput* output = gumbo_parse(html.c_str());

    GumboNode* next_li = find_element_by_class(output->root, "li", "next");
    std::string next_url;

    if (next_li) {
        GumboNode* next_a = find_element_by_class(next_li, "a", "");
        if (next_a && next_a->type == GUMBO_NODE_ELEMENT) {
            GumboAttribute* href = gumbo_get_attribute(&next_a->v.element.attributes, "href");
            if (href) {
                next_url = BASE_URL + std::string(href->value);
            }
        }
    }

    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return next_url;
}

// Save quotes to CSV file
void save_to_csv(const std::vector<Quote>& quotes, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        log_error("Failed to open file: " + filename);
        return;
    }

    // Write header
    file << "text,author,tags\n";

    // Write data
    for (const Quote& quote : quotes) {
        // Escape quotes and commas in CSV
        std::string escaped_text = quote.text;
        std::string escaped_author = quote.author;
        std::string escaped_tags = quote.tags;

        // Simple CSV escaping (wrap in quotes if contains comma or quote)
        if (escaped_text.find(',') != std::string::npos || escaped_text.find('"') != std::string::npos) {
            std::regex quote_regex("\"");
            escaped_text = std::regex_replace(escaped_text, quote_regex, "\"\"");
            escaped_text = "\"" + escaped_text + "\"";
        }
        if (escaped_author.find(',') != std::string::npos || escaped_author.find('"') != std::string::npos) {
            std::regex quote_regex("\"");
            escaped_author = std::regex_replace(escaped_author, quote_regex, "\"\"");
            escaped_author = "\"" + escaped_author + "\"";
        }
        if (escaped_tags.find(',') != std::string::npos || escaped_tags.find('"') != std::string::npos) {
            std::regex quote_regex("\"");
            escaped_tags = std::regex_replace(escaped_tags, quote_regex, "\"\"");
            escaped_tags = "\"" + escaped_tags + "\"";
        }

        file << escaped_text << "," << escaped_author << "," << escaped_tags << "\n";
    }

    file.close();
}

int main() {
    // Initialize curl globally
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Random number generator for delays
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(1.0, 3.0);

    std::string url = START_URL;
    std::vector<Quote> all_quotes;

    while (!url.empty()) {
        log_info("Fetching: " + url);

        std::string html = get_page(url);
        if (html.empty()) {
            break;
        }

        std::vector<Quote> quotes = parse_quotes(html);
        all_quotes.insert(all_quotes.end(), quotes.begin(), quotes.end());

        url = get_next_page(html);

        // Random delay of 1-3 seconds
        double delay = dis(gen);
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(delay * 1000)));
    }

    if (!all_quotes.empty()) {
        save_to_csv(all_quotes, OUTPUT_CSV);
        log_info("Saved " + std::to_string(all_quotes.size()) + " quotes to file: " + OUTPUT_CSV);
    }
    else {
        log_warning("No quotes found.");
    }

    // Cleanup curl
    curl_global_cleanup();

    return 0;
}