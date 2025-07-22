#!/usr/bin/env python3
import os, pathlib, cgi, datetime

SCRIPT_DIR = pathlib.Path(__file__).resolve().parent          
UPLOAD_DIR = SCRIPT_DIR.parent / "uploads"                     
UPLOAD_DIR.mkdir(exist_ok=True)

form = cgi.FieldStorage()

if "file" in form:                      
    fileitem  = form["file"]            
    filename  = os.path.basename(fileitem.filename or "")
    filedata  = fileitem.file.read() if fileitem.file else b""

    if filename and filedata:
        dest = UPLOAD_DIR / filename
        with open(dest, "wb") as f:
            f.write(filedata)

        status  = "OK"
        message = f"Saved as <code>{filename}</code>"
    else:
        status  = "FAIL"
        message = "Empty file or missing filename."
else:
    status  = "FAIL"
    message = "No file field found."

print("Content-Type: text/html\r\n\r\n")
now = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
print(f"""<!DOCTYPE html>
<html><head><title>Upload {status}</title></head>
<body>
  <h1>Upload {status}</h1>
  <p>{message}</p>
  <p><a href="/">Back</a></p>
  <small>{now}</small>
</body></html>""")
