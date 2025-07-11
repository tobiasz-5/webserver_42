
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
