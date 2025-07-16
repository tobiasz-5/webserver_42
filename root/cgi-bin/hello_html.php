<?php
/* ---------- hello_html.php (CGI) ----------
 *  HTML di prova per PHP‑CGI
 *  - ora lato server
 *  - metodo HTTP
 *  - query‑string
 *  - pulsante per ricaricare con parametro
 */

date_default_timezone_set('Europe/Rome');          // cambia fuso se serve

$method = $_SERVER['REQUEST_METHOD'];
$query  = isset($_SERVER['QUERY_STRING']) ? $_SERVER['QUERY_STRING'] : '';
$time   = date('Y-m-d H:i:s');

header("Content-Type: text/html\r\n\r\n");         // header + CRLF CRLF
?>
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>CGI HTML demo (PHP)</title>
  <style>
    body { font-family:sans-serif; margin:2rem; }
    code { background:#f4f4f4; padding:2px 4px; }
  </style>
</head>
<body>
  <h1>Hello from PHP‑CGI</h1>
  <p>Current server time: <strong><?= $time ?></strong></p>
  <p>Request method: <code><?= htmlspecialchars($method) ?></code></p>
  <p>Query string: <code><?= htmlspecialchars($query) ?></code></p>

  <!-- <a class="btn" href="?name=42">Try with query <code>?name=42</code></a> -->
</body>
</html>