#include "Request.hpp"
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <cstdlib>         


Request::Request() : buffer(""), chunked(false) {}

Request::Request(Request const &other) { *this = other; }

Request &Request::operator=(Request const &other)
{
    if (this != &other)
    {
        buffer   = other.buffer;
        method   = other.method;
        uri      = other.uri;
        http_version = other.http_version;
        headers  = other.headers;
        body     = other.body;
        complete = other.complete;
        chunked  = other.chunked;
    }
    return *this;
}

Request::~Request() {}


const std::string &Request::getMethod()        const { return method; }
const std::string &Request::getUri()           const { return uri; }
const std::string &Request::getBody()          const { return body; }
const std::string &Request::gethttp_version()  const { return http_version; }
const std::map<std::string,std::string>& Request::getHeaders() const { return headers; }
const std::string &Request::getBuffer()        const { return buffer; }
const bool        &Request::getComplete()      const { return complete; }

std::string Request::getHeader(const std::string& k) const
{
    std::map<std::string,std::string>::const_iterator it = headers.find(k);
    return (it != headers.end() ? it->second : "");
}


void Request::setComplete(bool b) { complete = b; }

void Request::clearData()
{
    buffer.clear(); method.clear(); uri.clear(); http_version.clear();
    headers.clear(); body.clear();
    complete = false; chunked = false;
}


int Request::receiveData(int fd)
{
    char tmp[2048];
    int n = recv(fd, tmp, sizeof(tmp)-1, 0);
    if (n <= 0) { if (n < 0) perror("recv failed"); return n; }

    tmp[n] = '\0';
    buffer.append(tmp, n);

    std::cout << "\033[32m---REQUEST RECEIVED---\n" << buffer
              << "\n---END REQUEST----- (byte read:" << n << ")\033[0m\n";
    return n;
}

/* ================== UNCHUNK HELPER ================== */

std::string Request::unchunkBody(const std::string& raw)
{
    std::string out; size_t pos = 0;
    while (true)
    {
        size_t crlf = raw.find("\r\n", pos);
        if (crlf == std::string::npos) break;
        std::string lenHex = raw.substr(pos, crlf-pos);
        unsigned long len = strtoul(lenHex.c_str(), NULL, 16);
        if (len == 0) break;                     // trailer
        pos = crlf + 2;
        if (pos + len > raw.size()) break;       // malformed
        out.append(raw, pos, len);
        pos += len + 2;                          // skip chunk + CRLF
    }
    return out;
}


void Request::parseRequest()
{
    size_t headerEnd = buffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos) return;       

    std::istringstream first(buffer.substr(0, headerEnd));
    std::string line;
    std::getline(first, line);
    std::istringstream ls(line);
    ls >> method >> uri >> http_version;

    while (std::getline(first, line))
    {
        if (line=="\r" || line.empty()) break;
        size_t colon = line.find(':');
        if (colon != std::string::npos)
        {
            std::string key = line.substr(0, colon);
            std::string val = line.substr(colon+1);
            val.erase(0, val.find_first_not_of(" \t"));
            // if (!val.empty() && val.back() == '\r') val.pop_back();
            if (!val.empty() && val[val.size() - 1] == '\r')
                val.erase(val.size() - 1);
            headers[key] = val;
        }
    }

    std::string rawBody = buffer.substr(headerEnd+4);
    std::string te = getHeader("Transfer-Encoding");
    chunked = (te == "chunked" || te == "Chunked");
    body = chunked ? unchunkBody(rawBody) : rawBody;

    std::cout << "\n\033[38;5;22m--- Parsed Request ---\n"
              << "Method       : " << method << "\n"
              << "URI          : " << uri    << "\n"
              << "HTTP Version : " << http_version << "\n"
              << "Headers:\n";
    for (std::map<std::string,std::string>::const_iterator it = headers.begin(); it!=headers.end(); ++it)
        std::cout << "  " << it->first << ": " << it->second << "\n";
    std::cout << "Chunked      : " << (chunked?"yes":"no") << "\n"
              << "Body Length  : " << body.length() << " bytes\n"
              << "Body Content : " << body
              << "\n-----------------------\033[0m\n";
}
