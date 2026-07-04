<?php

header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Headers: Content-Type, Accept');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Content-Type: application/json; charset=utf-8');

$storageDir = dirname(__DIR__) . DIRECTORY_SEPARATOR . 'storage';
$stateFile = $storageDir . DIRECTORY_SEPARATOR . 'state.json';

if (!is_dir($storageDir)) {
    mkdir($storageDir, 0777, true);
}

function write_json_file($path, $payload)
{
    $tmpPath = $path . '.tmp';
    $encoded = json_encode($payload, JSON_PRETTY_PRINT);

    if ($encoded === false) {
        json_response(['ok' => false, 'error' => 'JSON encode failed'], 500);
    }

    $handle = fopen($tmpPath, 'wb');
    if ($handle === false) {
        json_response(['ok' => false, 'error' => 'Storage write failed'], 500);
    }

    try {
        if (!flock($handle, LOCK_EX)) {
            json_response(['ok' => false, 'error' => 'Storage lock failed'], 500);
        }

        fwrite($handle, $encoded);
        fflush($handle);
        flock($handle, LOCK_UN);
    } finally {
        fclose($handle);
    }

    rename($tmpPath, $path);
}

function json_response($payload, $status = 200)
{
    http_response_code($status);
    echo json_encode($payload, JSON_PRETTY_PRINT);
    exit;
}

function read_json_payload()
{
    $payload = json_decode(file_get_contents('php://input') ?: '', true);
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
