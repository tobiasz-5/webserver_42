#!/usr/bin/env python3
import os, sys
print("Hello CGI! Method:", os.environ.get("REQUEST_METHOD"))
print("Query:", os.environ.get("QUERY_STRING", ""))
print("Body-len:", os.environ.get("CONTENT_LENGTH", "0"))
open("local.txt", "w").write("ok")
