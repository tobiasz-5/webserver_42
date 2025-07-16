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

    temp.push_back("REQUEST_METHOD="  + req.getMethod());
    temp.push_back("SCRIPT_FILENAME=" + scriptPath);
    temp.push_back("PATH_INFO="       + scriptPath);

    std::string uri = req.getUri();
    size_t q = uri.find('?');
    temp.push_back("QUERY_STRING=" + (q == std::string::npos ? "" : uri.substr(q + 1)));

    if (!body.empty())
        temp.push_back("CONTENT_LENGTH=" + to_stringgg(body.size()));
    std::string ctype = req.getHeader("Content-Type");
    if (!ctype.empty())
        temp.push_back("CONTENT_TYPE=" + ctype);

    temp.push_back("SERVER_PROTOCOL=HTTP/1.1");
    temp.push_back("SERVER_NAME=" + req.getHeader("Host"));

    std::vector<char*> env;
    for (size_t i = 0; i < temp.size(); ++i)
        env.push_back(strdup(temp[i].c_str()));
    env.push_back(strdup("REDIRECT_STATUS=200"));
    env.push_back(NULL);
    return env;  
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
    std::size_t dot = uri.rfind('.');
    if (dot == std::string::npos)
        return false;
    std::string ext = uri.substr(dot);
    return std::find(r.cgi_extensions.begin(),
                     r.cgi_extensions.end(),
                     ext) != r.cgi_extensions.end();
}

std::string runCgi(const Client& cli,
                   const route&  r,
                   const std::string& scriptPathRel,
                   const std::string& body)
{
    char absBuf[PATH_MAX];
    if (!realpath(scriptPathRel.c_str(), absBuf))
        return "Status: 404 Not Found\r\n"
               "Content-Type: text/plain\r\n\r\n"
               "CGI script not found";

    std::string scriptAbs(absBuf);

    int inPipe[2], outPipe[2];
    if (pipe(inPipe) == -1 || pipe(outPipe) == -1)
        throw std::runtime_error("pipe failed");

    pid_t pid = fork();
    if (pid == -1)
        throw std::runtime_error("fork failed");
    if (pid == 0)
    {
        if (chdir(r.root_directory.c_str()) == -1)
            _exit(1);

        dup2(inPipe[0],  STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);
        close(inPipe[1]);
        close(outPipe[0]);

        const char* dot = strrchr(scriptAbs.c_str(), '.');
        bool isPhp = (dot && std::strcmp(dot, ".php") == 0);

        char* argvPhp[] = 
        {
            const_cast<char*>(r.cgi_path.c_str()),   
            const_cast<char*>("-f"),
            const_cast<char*>(scriptAbs.c_str()),
            NULL
        };
        char* argvOther[] = {
            const_cast<char*>(r.cgi_path.c_str()),   
            const_cast<char*>(scriptAbs.c_str()),
            NULL
        };
        char** argv = isPhp ? argvPhp : argvOther;

        std::vector<char*> env = buildEnv(cli, scriptAbs, body);
        env.push_back(strdup("REDIRECT_STATUS=200"));
        env.push_back(NULL);

        execve(r.cgi_path.c_str(), argv, &env[0]);
        _exit(1);          
    }

    close(inPipe[0]);  
    close(outPipe[1]);

    if (!body.empty())
        write(inPipe[1], body.data(), body.size());
    close(inPipe[1]);

    std::string output;  
    char buf[4096]; 
    ssize_t n;

    while ((n = read(outPipe[0], buf, sizeof(buf))) > 0)
        output.append(buf, n);
    close(outPipe[0]);

    int status; 
    waitpid(pid, &status, 0);
    return output;   
}