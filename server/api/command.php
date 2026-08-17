<?php

require __DIR__ . '/config.php';

const BUZZER_DEFAULT_MELODY = 'musicBox';
const BUZZER_DEFAULT_SPEED = 100;
const BUZZER_MIN_SPEED = 50;
const BUZZER_MAX_SPEED = 200;
const BUZZER_ALLOWED_MELODIES = ['musicBox', 'toreador', 'mega', 'siren118', 'doom'];

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    json_response(['ok' => false, 'error' => 'Method not allowed'], 405);
}

require_api_token();

$payload = read_json_payload();

$state = load_state($stateFile);
$actionValue = isset($payload['action']) ? $payload['action'] : null;
$action = is_string($actionValue) ? $actionValue : '';
$params = isset($payload['params']) && is_array($payload['params'])
    ? $payload['params']
    : [];
$locks = isset($state['_commandLocks']) ? $state['_commandLocks'] : [];

function clamp_int($value, $default, $min, $max)
{
    if (is_int($value) || is_float($value) || (is_string($value) && is_numeric($value))) {
        $value = (int)$value;
    } else {
        $value = $default;
    }

    return max($min, min($max, $value));
}

function normalize_buzzer_melody($value)
{
    if (is_string($value) && in_array($value, BUZZER_ALLOWED_MELODIES, true)) {
        return $value;
    }

    return BUZZER_DEFAULT_MELODY;
}

function queue_buzzer(&$state, $melody, $speed = BUZZER_DEFAULT_SPEED)
{
    $state['buzzerMelody'] = normalize_buzzer_melody($melody);
    $state['buzzerSpeed'] = clamp_int(
        $speed,
        BUZZER_DEFAULT_SPEED,
        BUZZER_MIN_SPEED,
        BUZZER_MAX_SPEED
    );
    $state['buzzerEnabled'] = true;
    // Legacy field still consumed by the ESP32-to-Mega bridge.
    $state['doomBuzzerEnabled'] = true;
    $state['buzzerRequestId'] = ((int)(isset($state['buzzerRequestId']) ? $state['buzzerRequestId'] : 0)) + 1;
}

function stop_buzzer(&$state)
{
    $state['buzzerEnabled'] = false;
    $state['doomBuzzerEnabled'] = false;
    $state['buzzerRequestId'] = ((int)(isset($state['buzzerRequestId']) ? $state['buzzerRequestId'] : 0)) + 1;
}

$lockCommandKeys = function ($keys) use (&$state, &$locks) {
    $until = time() + 30;
    foreach ($keys as $key) {
        $locks[$key] = [
            'value' => isset($state[$key]) ? $state[$key] : null,
            'until' => $until,
        ];
    }
    $state['_commandLocks'] = $locks;
};

switch ($action) {
    case 'openInternalDoor':
        $state['internalDoorUnlocked'] = true;
        $lockCommandKeys(['internalDoorUnlocked']);
        break;
    case 'toggleAlarm':
        $state['intrusionAlarmArmed'] = !$state['intrusionAlarmArmed'];
        $lockCommandKeys(['intrusionAlarmArmed']);
        break;
    case 'toggleRfid':
        $state['rfidReaderEnabled'] = !$state['rfidReaderEnabled'];
        $lockCommandKeys(['rfidReaderEnabled']);
        break;
    case 'openParkingBarrier':
        $state['parkingBarrierOpen'] = true;
        $lockCommandKeys(['parkingBarrierOpen']);
        break;
    case 'vehicleEntered':
        $state['occupiedSpots'] = min(
            $state['parkingCapacity'],
            $state['occupiedSpots'] + 1
        );
        $state['parkingBarrierOpen'] = false;
        $state['vehicleDetected'] = false;
        break;
    case 'vehicleExited':
        $state['occupiedSpots'] = max(0, $state['occupiedSpots'] - 1);
        $state['parkingBarrierOpen'] = false;
        $state['vehicleDetected'] = false;
        break;
    case 'toggleExteriorLights':
        $state['exteriorLightsOn'] = !$state['exteriorLightsOn'];
        $lockCommandKeys(['exteriorLightsOn']);
        break;
    case 'toggleAwning':
        $state['awningOpen'] = !$state['awningOpen'];
        $lockCommandKeys(['awningOpen']);
        break;
    case 'syncExteriorAutomation':
        $state['exteriorLightsOn'] = (bool)$state['twilightDetected'];
        $state['awningOpen'] = !(bool)$state['twilightDetected'];
        $lockCommandKeys(['exteriorLightsOn', 'awningOpen']);
        break;
    case 'toggleFan':
        $state['fanOn'] = !$state['fanOn'];
        $lockCommandKeys(['fanOn']);
        break;
    case 'toggleWindows':
        $state['windowsOpen'] = !$state['windowsOpen'];
        $lockCommandKeys(['windowsOpen']);
        break;
    case 'playBuzzer':
    case 'playDoomBuzzer':
        queue_buzzer($state, 'doom');
        break;
    case 'playFnafMusicBox':
        queue_buzzer($state, 'musicBox');
        break;
    case 'playToreadorMarch':
        queue_buzzer($state, 'toreador');
        break;
    case 'playMegaBoss':
        queue_buzzer($state, 'mega');
        break;
    case 'playAmbulanceSiren':
        queue_buzzer($state, 'siren118');
        break;
    case 'playSelectedBuzzer':
        queue_buzzer(
            $state,
            isset($params['melody']) ? $params['melody'] : BUZZER_DEFAULT_MELODY,
            isset($params['speed']) ? $params['speed'] : BUZZER_DEFAULT_SPEED
        );
        break;
    case 'stopBuzzer':
        stop_buzzer($state);
        break;
    case 'syncClimateAutomation':
        $state['fanOn'] = $state['temperature'] >= 28 || $state['humidity'] >= 68;
        $state['windowsOpen'] = $state['temperature'] >= 29;
        $state['lcdEnabled'] = true;
        $lockCommandKeys(['fanOn', 'windowsOpen', 'lcdEnabled']);
        break;
    case 'toggleIndoorLights':
        $state['indoorLightsOn'] = !$state['indoorLightsOn'];
        $lockCommandKeys(['indoorLightsOn']);
        break;
    case 'syncPresenceLighting':
        $state['indoorLightsOn'] = (bool)$state['motionDetected'];
        $lockCommandKeys(['indoorLightsOn']);
        break;
    default:
        json_response(['ok' => false, 'error' => 'Unknown action'], 400);
}

save_state($stateFile, $state);

json_response([
    'ok' => true,
    'state' => load_state($stateFile),
]);
