#include "cgi_utils.hpp"
#include "Client.hpp"
#include "config.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <vector>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <limits.h> 
#include <stdlib.h> 

static std::vector<char*> buildEnv(const Client& cli,
                                   const std::string& scriptPath,
                                   const std::string& body)
{
    const Request& req = cli.getRequest(); 
    std::vector<std::string> temp;

    //costruire l ambiente (temporaneo) per ogni processo/esecuzione-di-cgi
    //ovvero alcune variabili cgi standard da passare all'interprete
    temp.push_back("REQUEST_METHOD="  + req.getMethod());
    temp.push_back("SCRIPT_FILENAME=" + scriptPath);
    temp.push_back("PATH_INFO="       + scriptPath);

    std::string uri = req.getUri();
    size_t q = uri.find('?');
    temp.push_back("QUERY_STRING=" + (q == std::string::npos ? "" : uri.substr(q + 1))); //se c'e estrae la query string altrimenti ""

    if (!body.empty())
        temp.push_back("CONTENT_LENGTH=" + to_stringgg(body.size())); //se c'e un body lo trasforma in numero e lo aggiunge a content legnth
    std::string ctype = req.getHeader("Content-Type");
    if (!ctype.empty())
        temp.push_back("CONTENT_TYPE=" + ctype);//aggiunge il content type

    temp.push_back("SERVER_PROTOCOL=HTTP/1.1");
    temp.push_back("SERVER_NAME=" + req.getHeader("Host"));

    std::vector<char*> env;
    for (size_t i = 0; i < temp.size(); ++i)
        env.push_back(strdup(temp[i].c_str())); //riempi il vettore di strnighe
    env.push_back(strdup("REDIRECT_STATUS=200")); //aggiunge in fondo quest ultiam variabile d ambienete per il cgi
    env.push_back(NULL);//null termina
    return env; //ritorna il vettore di stringhe da passare ad execvue
}

/* ---------- utilità interne ---------- */
// static void pushEnv(std::vector<std::string>& v,
//                     const std::string& k,
//                     const std::string& val)
// {
//     v.push_back(k + "=" + val);
// }

// static std::string ultoa(unsigned long n)
// {
//     std::ostringstream oss;
//     oss << n;
//     return oss.str();
// }


bool isCgiRequest(const std::string& uri, const route& r)
{
    std::size_t dot = uri.rfind('.'); //cerca il . partendo da destra
    if (dot == std::string::npos) 
        return false;
    std::string ext = uri.substr(dot); //estrae l estensione
    return std::find(r.cgi_extensions.begin(), r.cgi_extensions.end(), ext) != r.cgi_extensions.end(); //ritorna true se trova ext nelle cgi_extensions , altrimenti find trova end e ritorna false
}

//Il server (padre) scrive nel inPipe[1] → va nello STDIN del CGI.
//Il CGI (figlio) scrive nel STDOUT_FILENO, che è stato duplicato su outPipe[1].
//Il server (padre) legge da outPipe[0].
//inPipe serve per scrivere il body della richiesta HTTP nello stdin del processo CGI.
//outPipe serve per leggere l’output del CGI e restituirlo come risposta HTTP.
std::string runCgi(const Client& cli,
                   const route&  r,
                   const std::string& scriptPathRel,
                   const std::string& body)
{
    char absBuf[PATH_MAX];
    if (!realpath(scriptPathRel.c_str(), absBuf)) //restituisce percorso pulito ad es toglie .
        return "Status: 404 Not Found\r\n"
               "Content-Type: text/plain\r\n\r\n"
               "CGI script not found";

    std::string scriptAbs(absBuf); 

    int inPipe[2], outPipe[2]; //per inviare e ricevere dati al/dal cgi
    if (pipe(inPipe) == -1 || pipe(outPipe) == -1)
        throw std::runtime_error("pipe failed");

    pid_t pid = fork();//crea processo figlio, padre ha pid del figlio, figlio ha pid 0
    if (pid == -1)
        throw std::runtime_error("fork failed");
    if (pid == 0)
    {
        if (chdir(r.root_directory.c_str()) == -1) //se non riesce a spostarsi nella root degli script esce - richiesat di alcuni cgi, essere esugiti li
            _exit(1);

        dup2(inPipe[0],  STDIN_FILENO);//duplica stdin in lato lettura pipe
        dup2(outPipe[1], STDOUT_FILENO);//duplica stdout in lato scrittura pipe 
        close(inPipe[1]);//chiude le estremita non necessarie
        close(outPipe[0]);

        const char* dot = strrchr(scriptAbs.c_str(), '.'); //cerca il . ritorna puntatore
        bool isPhp = (dot && std::strcmp(dot, ".php") == 0); //se php imposta var a true

        char* argvPhp[] = 
        {
            const_cast<char*>(r.cgi_path.c_str()),   //path all interprete
            // const_cast<char*>("-f"), //flag opzionale per forzare
            const_cast<char*>(scriptAbs.c_str()), //path allo script
            NULL 
        };
        char* argvOther[] = {
            const_cast<char*>(r.cgi_path.c_str()),   
            const_cast<char*>(scriptAbs.c_str()),
            NULL
        };
        char** argv = isPhp ? argvPhp : argvOther; //costurisce argv con php o con py da passare poi a execve

        std::vector<char*> env = buildEnv(cli, scriptAbs, body);//ritorna il vettore costruito con tutte le var d ambiente per il cgi
        // env.push_back(strdup("REDIRECT_STATUS=200")); 
        // env.push_back(NULL);

        //esegue prendono il percorso dell interprete , ovvero il programma da eseguire. es: /usr/bin/php-cgi
        //l'array argv di argomenti argv[0] e' il programma ovvero per es /usr/bin/php-cgi . argv[1] e' lo script . argv[2] e' null
        //infine prende un puntatore alla prima stringa d ambiente
        execve(r.cgi_path.c_str(), argv, &env[0]); 
        _exit(1);//in caso di errore esce dal figlio          
    }

    close(inPipe[0]);  
    close(outPipe[1]);

    //se il body non e' vuoto, scrive nel lato di scrittura della pipe, ovvero invia i dati al cgi
    if (!body.empty())
        write(inPipe[1], body.data(), body.size());
    close(inPipe[1]);

    std::string output;  //qui viene slavato l intero output del cgi
    char buf[4096]; //buffer temporaneo per leggere i dati a blocchi
    ssize_t n; //numero di byte letti ad ogni iterazione

    //legge l'output del cgi, lo aggiunge al buffer, via via riempie output tramite append, quando i byte da leggere sono finiti termina
    while ((n = read(outPipe[0], buf, sizeof(buf))) > 0) 
        output.append(buf, n);
    close(outPipe[0]);

    
    int status; //variabile per stato di uscita del figlio

    //pid, aspettiamo il figlio, un unico processo, il cgi
    //status non lo usiamo effetivamente
    //0 server bloccato finche il figlio non termina
    waitpid(pid, &status, 0); 


    return output;   
}