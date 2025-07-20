
#include "config.hpp"
#include "Server.hpp"

std::string trim_space(const std::string& s) // Funzione trim: rimuove spazi iniziali e finali
{
    size_t start = 0;
    while (start < s.size() && std::isspace(s[start]))
        ++start;
    size_t end = s.size();
    while (end > start && std::isspace(s[end-1]))
        --end;
    std::string r = s.substr(start, end - start);
    return r;
}

std::vector<std::string> divide_words(const std::string &str) // Funzione split: divide una stringa in parole separate da spazi
{
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string word;
    while (iss >> word)
    {
        if (!word.empty() && word[word.size() - 1] == ';')
            word.erase(word.size() - 1); // rimuove ';' finale
        tokens.push_back(word);
    }
    return tokens;
}

int to_int(const std::string &s)
{
    int val = 0;
    std::istringstream iss(s);
    iss >> val;
    if (iss.fail())
        throw std::runtime_error("Conversion to int failed: " + s);
    return val;
}

unsigned long to_long(const std::string &s)
{
    unsigned long val = 0;
    std::istringstream iss(s);
    iss >> val;
    if (iss.fail())
        throw std::runtime_error("Conversion to long failed: " + s);
    return val;
}

std::string to_stringgg(int num)
{
	std::stringstream ss;
	ss << num;
	std::string ris = ss.str();
	return(ris);
}


std::string get_type(const std::string& path)
{
    static std::map<std::string, std::string> mime_types;
    if (mime_types.empty()) {
        mime_types[".html"] = "text/html";
        mime_types[".htm"] = "text/html";
        mime_types[".css"] = "text/css";
        mime_types[".js"] = "application/javascript";
        mime_types[".json"] = "application/json";
        mime_types[".jpg"] = "image/jpeg";
        mime_types[".jpeg"] = "image/jpeg";
        mime_types[".png"] = "image/png";
        mime_types[".gif"] = "image/gif";
        mime_types[".svg"] = "image/svg+xml";
        mime_types[".ico"] = "image/x-icon";
        mime_types[".txt"] = "text/plain";
        mime_types[".pdf"] = "application/pdf";
        mime_types[".zip"] = "application/zip";
        mime_types[".tar"] = "application/x-tar";
        mime_types[".mp3"] = "audio/mpeg";
        mime_types[".mp4"] = "video/mp4";
    }
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos != std::string::npos)
    {
        std::string ext = path.substr(dot_pos);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (mime_types.count(ext))
            return mime_types[ext];
    }
    return "application/octet-stream"; // fallback per file binari sconosciuti
}
