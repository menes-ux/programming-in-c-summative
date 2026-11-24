# Occupancy Monitor

An Arduino Uno system simulated in Tinkercad. Two buttons represent people entering and leaving a room with a capacity of 5.

[Open the circuit in Tinkercad](https://www.tinkercad.com/things/fgf4fVQzK0d/editel?returnTo=%2Fdashboard)

The LCD shows the number inside out of 5, remaining spaces, and OPEN or FULL. Green means space is available; red means the room is full. The Serial Monitor welcomes people entering, says goodbye when they leave, and reports totals and rejected attempts.

For example, with two people inside:

```text
Inside: 2/5
Spaces: 3   OPEN
```

## Circuit

| Component | Arduino connection |
| --- | --- |
| Entry button | A0 and GND when pressed |
| Exit button | A1 and GND when pressed |
| Green LED | A2 through a current-limiting resistor to GND |
| Red LED | A3 through a current-limiting resistor to GND |
| I2C LCD | 5V, GND, SDA, and SCL |

The buttons use `INPUT_PULLUP`, so a press reads LOW. Each LED needs its own series resistor, such as 220 or 330 ohms. The LCD must be compatible with `Adafruit_LiquidCrystal`.

## Run

1. Open the circuit in Tinkercad on your laptop.
2. Open **Code** and select **Text**.
3. Replace the current sketch with the contents of `occupancy_monitor.ino`.
4. Start the simulation and open the Serial Monitor. The sketch uses 9600 baud.
5. Press and release the entry and exit buttons to change the count.

No physical Arduino is needed to run the Tinkercad simulation.

## Code

`Occupancy` stores the current count and accepted entry and exit totals. `room` is a pointer to one record allocated with `calloc()`. Allocation is checked, and the record stays allocated for the powered-on lifetime of the monitor.

`Button` stores each button's reading and state. A reading must remain unchanged for 20 ms before it is accepted. Holding a button produces one event; it must be released before another press can be counted.

The LCD labels are written once. After an accepted press, the LEDs, counts, and OPEN/FULL status update before the Serial message is printed. `millis()` also schedules a refresh of these values after one second without a display update. There are no `delay()` calls in the sketch.

Entry is rejected when the room is full, and exit is rejected when it is empty. Rejected attempts do not change the count or accepted totals. If both presses are detected together, exit is handled first.

## Testing

All tests below passed during manual testing in Tinkercad.

| Test | Result | Status |
| --- | --- | --- |
| Start the simulation | Inside 0/5, spaces 5, OPEN, green on, red off, Ready message | Passed |
| Press entry once | Inside 1/5, spaces 4; entries increases once | Passed |
| Hold the entry button | Only the first press is counted | Passed |
| Release entry, then press again | One more person is counted | Passed |
| Reach 5 people | Inside 5/5, spaces 0, FULL, green off, red on | Passed |
| Try entering when full | Entry rejected; count and totals unchanged | Passed |
| Press exit once from full | Inside 4/5, spaces 1, OPEN, green on, red off | Passed |
| Return to 0, then try exiting | Exit rejected; count stays 0 and totals unchanged | Passed |

## Screenshots

| Screenshot | What it shows |
| --- | --- |
| [Available](screenshots/available.png) | Inside 2/5, spaces 3, OPEN, green LED, and accepted entry/exit messages |
| [Full](screenshots/full.png) | Inside 5/5, spaces 0, FULL, red LED, and a rejected entry with unchanged totals |
| [Empty](screenshots/empty.png) | Inside 0/5, spaces 5, OPEN, green LED, and a rejected exit with unchanged totals |
