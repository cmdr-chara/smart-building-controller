<?php

require __DIR__ . '/config.php';

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    json_response(['ok' => false, 'error' => 'Method not allowed'], 405);
}

$payload = read_json_payload();

$allowedKeys = [
    'internalDoorUnlocked',
    'intrusionAlarmArmed',
    'rfidReaderEnabled',
    'parkingBarrierOpen',
    'vehicleDetected',
    'parkingCapacity',
    'occupiedSpots',
    'twilightDetected',
    'exteriorLightsOn',
    'awningOpen',
    'temperature',
    'humidity',
    'sensorOk',
    'fanOn',
    'fanPower',
    'fanPwm',
    'windowsOpen',
    'remoteOverride',
    'motionDetected',
    'indoorLightsOn',
    'lcdEnabled',
];

$state = load_state($stateFile);
$locks = isset($state['_commandLocks']) ? $state['_commandLocks'] : [];
$now = time();

foreach ($allowedKeys as $key) {
    if (array_key_exists($key, $payload)) {
        if (isset($locks[$key]) && is_array($locks[$key])) {
            $until = (int)(isset($locks[$key]['until']) ? $locks[$key]['until'] : 0);
            $desired = isset($locks[$key]['value']) ? $locks[$key]['value'] : null;

            if ($until > $now && $payload[$key] != $desired) {
                continue;
            }

            unset($locks[$key]);
        }

        $state[$key] = $payload[$key];
    }
}

foreach ($locks as $key => $lock) {
    if (!is_array($lock) || (int)(isset($lock['until']) ? $lock['until'] : 0) <= $now) {
        unset($locks[$key]);
    }
}

$state['_commandLocks'] = $locks;

save_state($stateFile, $state);

json_response([
    'ok' => true,
    'state' => load_state($stateFile),
]);
