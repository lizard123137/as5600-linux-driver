import curses

HEADER_PADDING = 50

outputs = {
        "in_angle_raw" : "/sys/bus/iio/devices/iio:device0/in_angl_raw",
        "in_angle_input" : "/sys/bus/iio/devices/iio:device0/in_angl_input",
}

attributes = {
        "automatic_gain_control" : "/sys/bus/iio/devices/iio:device0/automatic_gain_control",
        "burn_counter" : "/sys/bus/iio/devices/iio:device0/burn_counter",
        "start_position" : "/sys/bus/iio/devices/iio:device0/start_position",
        "stop_position" : "/sys/bus/iio/devices/iio:device0/stop_position",
        "magnitude" : "/sys/bus/iio/devices/iio:device0/magnitude",
        "max_angle" : "/sys/bus/iio/devices/iio:device0/max_angle",
}

status_attributes = {
        "magnet_detected" : "/sys/bus/iio/devices/iio:device0/magnet_detected",
        "magnet_too_weak" : "/sys/bus/iio/devices/iio:device0/magnet_too_weak",
        "magnet_too_strong" : "/sys/bus/iio/devices/iio:device0/magnet_too_strong",
}

conf_attributes = {
        "conf_fast_filter" : "/sys/bus/iio/devices/iio:device0/conf_fast_filter",
        "conf_hysteresis" : "/sys/bus/iio/devices/iio:device0/conf_hysteresis",
        "conf_output_stage" : "/sys/bus/iio/devices/iio:device0/conf_output_stage",
        "conf_power_mode" : "/sys/bus/iio/devices/iio:device0/conf_power_mode",
        "conf_pwm_freq" : "/sys/bus/iio/devices/iio:device0/conf_pwm_freq",
        "conf_slow_filter" : "/sys/bus/iio/devices/iio:device0/conf_slow_filter",
        "conf_watchdog" : "/sys/bus/iio/devices/iio:device0/conf_watchdog",
}

def main(stdscr):
    curses.curs_set(0)

    while True:
        stdscr.erase()

        i = 0
        stdscr.addstr(i, 0, "AS5600 Outputs".center(HEADER_PADDING, "="))
        for (key, value) in outputs.items():
            with open(value, "r") as f:
                i += 1
                text = f.read()
                stdscr.addstr(i, 0, f"{key:<25}: {text:>10}")
        i += 1

        stdscr.addstr(i, 0, "AS5600 Attributes".center(HEADER_PADDING, "="))
        for (key, value) in attributes.items():
            with open(value, "r") as f:
                i += 1
                text = f.read()
                stdscr.addstr(i, 0, f"{key:<25}: {text:>10}")
        i += 1

        stdscr.addstr(i, 0, "AS5600 Status Attributes".center(HEADER_PADDING, "="))
        for (key, value) in status_attributes.items():
            with open(value, "r") as f:
                i += 1
                text = f.read()
                stdscr.addstr(i, 0, f"{key:<25}: {text:>10}")
        i += 1

        stdscr.addstr(i, 0, "AS5600 Config Attributes".center(HEADER_PADDING, "="))
        for (key, value) in conf_attributes.items():
            with open(value, "r") as f:
                i += 1
                text = f.read()
                stdscr.addstr(i, 0, f"{key:<25}: {text:>10}")
        stdscr.refresh()

curses.wrapper(main)
