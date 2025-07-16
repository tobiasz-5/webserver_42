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


//per recuperare il valore di un header HTTP specifico dalla richiesta (Content-Type, Host, Content-lenght...)
std::string Request::getHeader(const std::string& k) const
{
    std::map<std::string,std::string>::const_iterator it = headers.find(k);
    return (it != headers.end() ? it->second : ""); //se trovata, ritorna il valore associato it->second, altrimenti una stringa vuota ""
}

//indica se la richiesta è completamente ricevuta e parsata
void Request::setComplete(bool b) { complete = b; }

//azzerare/reset l’oggetto Request prima di riutilizzarlo per una nuova richiesta.
void Request::clearData()
{
    buffer.clear(); 
    method.clear(); 
    uri.clear(); 
    http_version.clear();
    headers.clear(); 
    body.clear();
    complete = false; 
    chunked = false;
}

//riceve i dati dalla socket (fd) e li aggiunge al buffer (request line, header, body)
int Request::receiveData(int fd)
{
    char tmp[2048];
    int n = recv(fd, tmp, sizeof(tmp)-1, 0);
    if (n <= 0) 
    { 
        if (n < 0) 
            perror("recv failed"); 
        return n; 
    }

    tmp[n] = '\0';
    buffer.append(tmp, n);

    std::cout << "\033[32m---REQUEST RECEIVED---\n" << buffer
              << "\n---END REQUEST----- (byte read:" << n << ")\033[0m\n";
    return n;
}

//body chunked servono a ottimizzare la risposta del server, quando non conosciamo content-lenght
std::string Request::unchunkBody(const std::string& raw)
{
    std::string out; //per il body un-chunked
    size_t pos = 0;
    while (true)
    {
        size_t crlf = raw.find("\r\n", pos);//cerca fine riga
        if (crlf == std::string::npos) 
            break;//se non la trova esce
        std::string lenHex = raw.substr(pos, crlf-pos); //estrae la stringa
        unsigned long len = strtoul(lenHex.c_str(), NULL, 16); //la rende un long in hex
        if (len == 0) //se le e' 0 siamo nell ultimo blocco, fine trasmissione
            break;   
        pos = crlf + 2; //altrimenti avanza al blocco successivo
        if (pos + len > raw.size()) //evita overread len non puo essere maggiore di body
            break;   
        out.append(raw, pos, len); //ricostruisce body via via unchunked
        pos += len + 2;// passa al chunk successivo 
    }
    return out;
}


void Request::parseRequest()
{
    size_t headerEnd = buffer.find("\r\n\r\n"); //cerca fine degli header HTTP
    if (headerEnd == std::string::npos) 
        return;//se non trova, richiesta incompleta       

    //estrae il blocco degli header
    std::istringstream first(buffer.substr(0, headerEnd));
    std::string line;
    std::getline(first, line);//legge la prima riga della richiesta
    std::istringstream ls(line);
    ls >> method >> uri >> http_version;//estrae metodo, uri, versione http

    //legge il resto degli header (chiave: valore)
    while (std::getline(first, line))
    {
        if (line=="\r" || line.empty()) 
            break; //Righe vuote → fine header
        size_t colon = line.find(':'); // trova : per separare chiave e valore
        if (colon != std::string::npos)
        {
            //// estrae chiave e valore
            std::string key = line.substr(0, colon);
            std::string val = line.substr(colon+1);

            // rimuove eventuali spazi o tab iniziali nel valore
            val.erase(0, val.find_first_not_of(" \t"));
            if (!val.empty() && val[val.size() - 1] == '\r')//rimuove \r finale da una riga, se presente.
                val.erase(val.size() - 1);
            headers[key] = val; // inserisce coppia chiave-valore nella mappa headers
        }
    }

    //ottiene la parte di body (dopo "\r\n\r\n")
    std::string rawBody = buffer.substr(headerEnd+4);

    //controlla se è presente l'header "Transfer-Encoding: chunked"
    std::string te = getHeader("Transfer-Encoding");
    chunked = (te == "chunked"); //imposta a true o false
    body = chunked ? unchunkBody(rawBody) : rawBody; //se true chiamo unchunckBody altrimenti body e' rawbody

    std::cout << "\n\033[38;5;22m--- Parsed Request ---\n"
              << "Method       : " << method << "\n"
              << "URI          : " << uri    << "\n"
              << "HTTP Version : " << http_version << "\n"
              << "Headers:\n";
    //stampa gli header
    for (std::map<std::string,std::string>::const_iterator it = headers.begin(); it!=headers.end(); ++it)
        std::cout << "  " << it->first << ": " << it->second << "\n";
    //stampa info sul corpo
    std::cout << "Chunked      : " << (chunked?"yes":"no") << "\n"
              << "Body Length  : " << body.length() << " bytes\n"
              << "Body Content : " << body
              << "\n-----------------------\033[0m\n";
}
