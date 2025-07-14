#!/usr/bin/env python3
import os, datetime

# Header *completo* (terminato da CRLF + CRLF)
# print("Content-Type: text/html\r\n")
print("Content-Type: text/html\r\n\r\n", end="")

# Corpo HTML
now = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
print(f"""\
<!DOCTYPE html>
<html>
<head><title>CGI Demo</title></head>
<body>
  <h1>Hello from CGI</h1>
  <p>Current server time: <strong>{now}</strong></p>
</body>
</html>""")
