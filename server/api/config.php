<?php

header('Content-Type: application/json; charset=utf-8');
header('Cache-Control: no-store');
header('X-Content-Type-Options: nosniff');
header('Referrer-Policy: no-referrer');
header('Access-Control-Allow-Headers: Content-Type, Accept, Authorization');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');

$allowedOrigin = getenv('SMART_CONTROLLER_ALLOWED_ORIGIN');
$requestOrigin = isset($_SERVER['HTTP_ORIGIN']) ? trim((string)$_SERVER['HTTP_ORIGIN']) : '';
if (is_string($allowedOrigin) && $allowedOrigin !== '' && $requestOrigin !== '' && hash_equals($allowedOrigin, $requestOrigin)) {
    header('Access-Control-Allow-Origin: ' . $allowedOrigin);
    header('Vary: Origin');
}

$storageDir = dirname(__DIR__) . DIRECTORY_SEPARATOR . 'storage';
$stateFile = $storageDir . DIRECTORY_SEPARATOR . 'state.json';

if (!is_dir($storageDir) && !mkdir($storageDir, 0700, true) && !is_dir($storageDir)) {
    http_response_code(500);
    echo json_encode(['ok' => false, 'error' => 'Storage initialization failed']);
    exit;
}
@chmod($storageDir, 0700);

function json_response($payload, $status = 200)
{
    http_response_code($status);
    echo json_encode($payload, JSON_PRETTY_PRINT);
    exit;
}

function configured_api_token()
{
    $token = getenv('SMART_CONTROLLER_API_TOKEN');
    if (!is_string($token)) {
        return null;
    }

    $token = trim($token);
    return strlen($token) >= 32 ? $token : null;
}

function require_api_token()
{
    $expected = configured_api_token();
    if ($expected === null) {
        json_response([
            'ok' => false,
            'error' => 'Server API token is not configured',
        ], 503);
    }

    $authorization = isset($_SERVER['HTTP_AUTHORIZATION'])
        ? trim((string)$_SERVER['HTTP_AUTHORIZATION'])
        : '';
    if (strncmp($authorization, 'Bearer ', 7) !== 0) {
        header('WWW-Authenticate: Bearer');
        json_response(['ok' => false, 'error' => 'Unauthorized'], 401);
    }

    $provided = substr($authorization, 7);
    if ($provided === '' || !hash_equals($expected, $provided)) {
        header('WWW-Authenticate: Bearer');
        json_response(['ok' => false, 'error' => 'Unauthorized'], 401);
    }
}

function write_json_file($path, $payload)
{
    $encoded = json_encode($payload, JSON_PRETTY_PRINT);

    if ($encoded === false) {
        json_response(['ok' => false, 'error' => 'JSON encode failed'], 500);
    }

    $tmpPath = tempnam(dirname($path), basename($path) . '.tmp.');
    if ($tmpPath === false) {
        json_response(['ok' => false, 'error' => 'Storage temp file failed'], 500);
    }
    @chmod($tmpPath, 0600);

    $handle = fopen($tmpPath, 'wb');
    if ($handle === false) {
        @unlink($tmpPath);
        json_response(['ok' => false, 'error' => 'Storage write failed'], 500);
    }

    try {
        if (!flock($handle, LOCK_EX)) {
            json_response(['ok' => false, 'error' => 'Storage lock failed'], 500);
        }
        if (fwrite($handle, $encoded) === false || !fflush($handle)) {
            json_response(['ok' => false, 'error' => 'Storage write failed'], 500);
        }
        flock($handle, LOCK_UN);
    } finally {
        fclose($handle);
    }

    if (!rename($tmpPath, $path)) {
        @unlink($tmpPath);
        json_response(['ok' => false, 'error' => 'Storage replace failed'], 500);
    }
    @chmod($path, 0600);
}

function read_json_payload()
{
    $contentLength = isset($_SERVER['CONTENT_LENGTH']) ? (int)$_SERVER['CONTENT_LENGTH'] : 0;
    if ($contentLength > 32768) {
        json_response(['ok' => false, 'error' => 'Request body too large'], 413);
    }

    $raw = file_get_contents('php://input') ?: '';
    if (strlen($raw) > 32768) {
        json_response(['ok' => false, 'error' => 'Request body too large'], 413);
    }

    $payload = json_decode($raw, true);
    if (!is_array($payload)) {
        json_response(['ok' => false, 'error' => 'Invalid JSON'], 400);
    }

    return $payload;
}

function default_state()
{
    return [
        'internalDoorUnlocked' => false,
        'intrusionAlarmArmed' => true,
        'rfidReaderEnabled' => true,
        'parkingBarrierOpen' => false,
        'vehicleDetected' => true,
        'parkingCapacity' => 32,
        'occupiedSpots' => 21,
        'twilightDetected' => false,
        'exteriorLightsOn' => false,
        'awningOpen' => true,
        'temperature' => 27.2,
        'humidity' => 62.0,
        'sensorOk' => true,
        'fanOn' => true,
        'fanPower' => 0,
        'fanPwm' => 0,
        'windowsOpen' => false,
        'remoteOverride' => false,
        'buzzerRequestId' => 0,
        'buzzerMelody' => 'musicBox',
        'buzzerSpeed' => 100,
        'buzzerEnabled' => false,
        'doomBuzzerEnabled' => false,
        'motionDetected' => true,
        'indoorLightsOn' => true,
        'lcdEnabled' => true,
        'lastUpdated' => date(DATE_ATOM),
    ];
}

function normalize_state($state)
{
    $defaults = default_state();
    $state = array_merge($defaults, $state);

    $boolKeys = [
        'internalDoorUnlocked',
        'intrusionAlarmArmed',
        'rfidReaderEnabled',
        'parkingBarrierOpen',
        'vehicleDetected',
        'twilightDetected',
        'exteriorLightsOn',
        'awningOpen',
        'sensorOk',
        'fanOn',
        'windowsOpen',
        'remoteOverride',
        'buzzerEnabled',
        'doomBuzzerEnabled',
        'motionDetected',
        'indoorLightsOn',
        'lcdEnabled',
    ];

    foreach ($boolKeys as $key) {
        $state[$key] = filter_var($state[$key], FILTER_VALIDATE_BOOL);
    }

    $state['parkingCapacity'] = max(0, min(999, (int)$state['parkingCapacity']));
    $state['occupiedSpots'] = max(0, min($state['parkingCapacity'], (int)$state['occupiedSpots']));
    $state['buzzerRequestId'] = max(0, (int)$state['buzzerRequestId']);
    $state['buzzerSpeed'] = max(50, min(200, (int)$state['buzzerSpeed']));
    $state['fanPower'] = max(0, min(255, (int)$state['fanPower']));
    $state['fanPwm'] = max(0, min(255, (int)$state['fanPwm']));
    $state['temperature'] = (float)$state['temperature'];
    $state['humidity'] = (float)$state['humidity'];

    if (!is_string($state['buzzerMelody']) || $state['buzzerMelody'] === '') {
        $state['buzzerMelody'] = $defaults['buzzerMelody'];
    }

    return $state;
}

function load_state($stateFile)
{
    if (!file_exists($stateFile)) {
        $initial = default_state();
        write_json_file($stateFile, $initial);
        return $initial;
    }

    $raw = file_get_contents($stateFile);
    $decoded = json_decode($raw ?: '', true);

    if (!is_array($decoded)) {
        $decoded = default_state();
    }

    return normalize_state($decoded);
}

function save_state($stateFile, $state)
{
    $state = normalize_state($state);
    $state['lastUpdated'] = date(DATE_ATOM);
    write_json_file($stateFile, $state);
}
