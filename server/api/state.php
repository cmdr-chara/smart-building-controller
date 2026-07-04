<?php

require __DIR__ . '/config.php';

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

json_response([
    'ok' => true,
    'state' => load_state($stateFile),
]);
