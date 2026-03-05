<?php
$name = isset($_GET['name']) ? $_GET['name'] : 'world';
echo "<h1>Hello, " . htmlspecialchars($name, ENT_QUOTES, 'UTF-8') . "!</h1>\n";
