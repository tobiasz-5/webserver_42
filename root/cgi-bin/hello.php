<?php
header("Content-Type: text/plain");

echo "Hello CGI via PHP!\n";
echo "Method: " . $_SERVER['REQUEST_METHOD'] . "\n";
echo "Query: " . (isset($_SERVER['QUERY_STRING']) ? $_SERVER['QUERY_STRING'] : '') . "\n";
echo "Body-Len: " . strlen(file_get_contents("php://input")) . "\n";
?>