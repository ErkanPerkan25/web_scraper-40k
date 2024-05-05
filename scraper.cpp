#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <curl/curl.h>
#include <string>
#include <vector>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "libxml/HTMLparser.h"
#include "libxml/xpath.h"

using namespace std;

struct PokemonProduct{
    string url;
    string image;
    string name;
    string price;
};

struct MemoryStruct{
    char *memory;
    size_t size;
};

static size_t mem_cb(void *contents, size_t size, size_t nmemb, void *userp){
    size_t real_size = size * nmemb;

    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    // have to explicity cast as C++ does not automatically cast to (char *)
    mem->memory = static_cast<char *>(realloc(mem->memory, mem->size + real_size +1));
    
    if (mem->memory == NULL) {
        // out of memory 
        printf("not enough memory (realloc returned NULL)");
        return 0;
    }
    
    memcpy(&(mem->memory[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->memory[mem->size] = 0;

    return real_size;
}

string get_request(string url){
    CURL *curl;
    CURLcode res;

    struct MemoryStruct chunk;

    chunk.memory = static_cast<char *>(malloc(1)); // grown as needed by the realloc 
    chunk.size = 0; // no data at this point
   
    // init curl session
    curl = curl_easy_init();

    // specify the URL to get
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // send all the data to the function
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mem_cb);

    // pass the "chunk" struct to the callback function
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    /* some servers do not like requests that are made without a user-agent
     field, so we provide one */
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    // get it!
    res = curl_easy_perform(curl);

    // check for errors
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res)); 
    }
    else {
             /*
         * Now, our chunk.memory points to a memory block that is chunk.size
         * bytes big and contains the remote file.
         *
         * Do something nice with it
         */

        printf("%lu bytes retrieved\n", (long)chunk.size);
    }
    
    // cleanup
    curl_easy_cleanup(curl);

    return chunk.memory;
}

int main(int argc, char *argv[]){
    string url;
    if (argc != 2) {
        cerr << "Expected: <url> " << endl;
        return 1;
    }
    // initialize curl globally
    curl_global_init(CURL_GLOBAL_ALL);

    // download the target HTML document 
    // and print it
    string html_doc = get_request("https://scrapeme.live/shop/");

    // parses html string and builds a tree, which XPath selectors can be used
    htmlDocPtr doc = htmlReadMemory(html_doc.c_str(), html_doc.size(), nullptr, nullptr, HTML_PARSE_NOERROR);

    vector<PokemonProduct> pokemon_products;
    // Retrives HTML products
    xmlXPathContextPtr context = xmlXPathNewContext(doc);

    xmlXPathObjectPtr product_html_elms = xmlXPathEvalExpression((xmlChar *) "//li[contains(@class, 'product')]", context);

    for (int i=0; i < product_html_elms->nodesetval->nodeNr; ++i) {
        xmlNodePtr product_html_elm = product_html_elms->nodesetval->nodeTab[i];

        xmlXPathSetContextNode(product_html_elm, context);

        xmlNodePtr url_html_elm = xmlXPathEvalExpression((xmlChar *) ".//a", context)->nodesetval->nodeTab[0];

        string url = string(reinterpret_cast<char *>(xmlGetProp(url_html_elm, (xmlChar *) "href")));

        xmlNodePtr image_html_elm = xmlXPathEvalExpression((xmlChar *) ".//a/img", context)->nodesetval->nodeTab[0];

        string image = string(reinterpret_cast<char *>(xmlGetProp(image_html_elm, (xmlChar *) "src")));

        xmlNodePtr name_html_elm = xmlXPathEvalExpression((xmlChar *) ".//a/h2", context)->nodesetval->nodeTab[0];

        string name = string(reinterpret_cast<char *>(xmlNodeGetContent(name_html_elm)));

        xmlNodePtr price_html_elm = xmlXPathEvalExpression((xmlChar *) ".//a/span", context)->nodesetval->nodeTab[0];

        string price = string(reinterpret_cast<char *>(xmlNodeGetContent(price_html_elm)));

        PokemonProduct pokemon_product = {url, image, name, price};
        pokemon_products.push_back(pokemon_product);
    }

    xmlXPathFreeContext(context);
    xmlFreeDoc(doc);

    ofstream csv_file("products.csv");

    csv_file << "url,image,name,price" << endl;

    for (int i=0; i < pokemon_products.size(); i++) {
        PokemonProduct p = pokemon_products[i];

        string csv_record = p.url + "," + p.image + "," + p.name + "," + p.price;

        csv_file << csv_record << endl;
    }

    csv_file.close();
    
    // free up the global curl resources
    curl_global_cleanup();

    return 0;
}
